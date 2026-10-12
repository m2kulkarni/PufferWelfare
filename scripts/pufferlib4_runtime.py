"""Create an isolated, explicitly patched PufferLib 4 Python runtime.

The upstream checkout is evidence and stays untouched. Runtime files come from
the pinned Git object, and every local change is supplied as a reviewable patch.
"""
from contextlib import contextmanager
from io import BytesIO
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile


PIN = "42f70d6932c30ac977736f861006809c50168ba9"
ROOT = Path(__file__).resolve().parents[1]
ROLLOUT_BOUNDARY_PATCH = ROOT / "patches/pufferlib4/preserve-rollout-boundaries.patch"
REWARD_CLIPPING_PATCH = ROOT / "patches/pufferlib4/configurable-reward-clipping.patch"


def _pinned_archive(upstream):
    upstream = Path(upstream).resolve()
    resolved = subprocess.check_output(
        ["git", "-C", str(upstream), "rev-parse", f"{PIN}^{{commit}}"],
        text=True,
    ).strip()
    if resolved != PIN:
        raise RuntimeError(f"upstream repository does not contain pinned commit {PIN}")
    return subprocess.check_output([
        "git", "-C", str(upstream), "archive", "--format=tar", PIN,
        "pufferlib", "config/default.ini",
    ])


def _extract_archive(data, destination):
    destination = destination.resolve()
    with tarfile.open(fileobj=BytesIO(data), mode="r:") as archive:
        for member in archive.getmembers():
            target = (destination / member.name).resolve()
            if target != destination and destination not in target.parents:
                raise RuntimeError(f"unsafe path in pinned archive: {member.name}")
            if member.issym() or member.islnk():
                raise RuntimeError(f"links are not allowed in pinned runtime: {member.name}")
        if sys.version_info >= (3, 12):
            archive.extractall(destination, filter="data")
        else:
            archive.extractall(destination)


@contextmanager
def isolated_runtime(upstream, patches=()):
    """Yield a temporary runtime copied from PIN with exact patches applied."""
    with tempfile.TemporaryDirectory(prefix="pufferlib4-runtime-") as scratch:
        runtime = Path(scratch).resolve()
        _extract_archive(_pinned_archive(upstream), runtime)
        for patch in patches:
            patch = Path(patch).resolve()
            if not patch.is_file():
                raise FileNotFoundError(patch)
            command = [
                "git", "apply", "--no-index", "--unidiff-zero",
                "--whitespace=error-all", str(patch),
            ]
            subprocess.run(command[:2] + ["--check"] + command[2:],
                           cwd=runtime, check=True)
            subprocess.run(command, cwd=runtime, check=True)
        yield runtime
