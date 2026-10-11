#!/usr/bin/env python3
"""Build the pinned PufferLib 4 CPU vector backend, outside the old package.

No downloads, GPU stubs of our own, rendering libraries, or upstream edits.
Pass an existing checkout at the pinned commit. This is not a trainer build.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import sysconfig

import pybind11

PIN = "42f70d6932c30ac977736f861006809c50168ba9"
ROOT = Path(__file__).resolve().parents[1]
SOURCES = ["src/vecenv.h", "src/tensor.h", "src/bindings_cpu.cpp"]


def run(command):
    print(shlex.join([str(part) for part in command]), flush=True)
    subprocess.run(command, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", required=True, type=Path)
    args = parser.parse_args()
    upstream = args.upstream.resolve()
    for name in SOURCES:
        pinned = subprocess.check_output(["git", "-C", str(upstream), "show", f"{PIN}:{name}"])
        if (upstream / name).read_bytes() != pinned:
            parser.error(f"{name} does not match pinned PufferLib 4 commit {PIN}")

    out = ROOT / "build" / "pufferlib4"
    out.mkdir(parents=True, exist_ok=True)
    includes = [ROOT / "native/pufferlib4", ROOT / "pufferlib/ocean/diplomacy", upstream / "src"]
    common = ["-O2", "-fPIC", "-fvisibility=hidden"]
    omp_link = ["-fopenmp"]
    if sys.platform == "darwin":
        omp = Path(os.environ.get("PUFFERWELFARE_LIBOMP", "/opt/homebrew/opt/libomp"))
        if not (omp / "include/omp.h").is_file():
            parser.error("libomp is required; set PUFFERWELFARE_LIBOMP to its prefix")
        common += ["-Xpreprocessor", "-fopenmp", f"-I{omp / 'include'}"]
        omp_link = [f"-L{omp / 'lib'}", f"-Wl,-rpath,{omp / 'lib'}", "-lomp"]
    else:
        common += ["-fopenmp"]
    cc = shlex.split(os.environ.get("CC", "clang"))
    cxx = shlex.split(os.environ.get("CXX", "clang++"))
    definitions = ["-DEnv=DiplomacyEngine", "-DLog=DiplomacyEngineLog"]
    for callback in ["init", "reset", "step", "render", "close", "configure"]:
        definitions.append(f"-Dc_{callback}=diplomacy_engine_{callback}")
    objects = []
    for name, source, defines in [
        ("engine", ROOT / "pufferlib/ocean/diplomacy/diplomacy.c", definitions),
        ("map", ROOT / "pufferlib/ocean/diplomacy/diplomacy_map.c", definitions),
        ("adapter", ROOT / "native/pufferlib4/binding.c", []),
    ]:
        obj = out / f"{name}.o"
        run(cc + common + ["-std=c11", "-Werror=implicit-function-declaration"]
            + [f"-I{path}" for path in includes] + defines + ["-c", str(source), "-o", str(obj)])
        objects.append(str(obj))
    extension = out / ("_C" + sysconfig.get_config_var("EXT_SUFFIX"))
    link = ["-undefined", "dynamic_lookup"] if sys.platform == "darwin" else []
    run(cxx + common + ["-std=c++17", "-shared", "-DENV_NAME=diplomacy"]
        + [f"-I{path}" for path in includes + [Path(pybind11.get_include()), Path(sysconfig.get_path("include"))]]
        + [str(upstream / "src/bindings_cpu.cpp")] + objects + omp_link + link + ["-o", str(extension)])
    print(f"Built CPU environment backend: {extension}")


if __name__ == "__main__":
    main()
