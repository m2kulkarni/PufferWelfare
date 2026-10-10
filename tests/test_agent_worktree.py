"""Check worktree creation without touching the developer's repository."""

from pathlib import Path
import subprocess

import pytest


SCRIPT = Path(__file__).resolve().parents[1] / "scripts/new_agent_worktree.sh"


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()


@pytest.fixture
def repo(tmp_path):
    remote = tmp_path / "remote.git"
    subprocess.run(["git", "init", "--bare", str(remote)], check=True, capture_output=True)
    checkout = tmp_path / "project"
    subprocess.run(["git", "init", "-b", "welfare", str(checkout)], check=True,
                   capture_output=True)
    git(checkout, "config", "user.name", "Worktree test")
    git(checkout, "config", "user.email", "test@example.invalid")
    git(checkout, "commit", "--allow-empty", "-m", "base")
    git(checkout, "remote", "add", "origin", str(remote))
    git(checkout, "push", "origin", "welfare")
    git(checkout, "switch", "-c", "agent/parent")
    git(checkout, "commit", "--allow-empty", "-m", "parent")
    git(checkout, "switch", "welfare")
    return checkout


def run(repo, *args):
    return subprocess.run(["bash", str(SCRIPT), *args], cwd=repo,
                          text=True, capture_output=True)


def test_default_base(repo):
    result = run(repo, "10", "default")
    assert result.returncode == 0, result.stderr
    target = Path(result.stdout.strip().splitlines()[-1])
    assert git(target, "rev-parse", "HEAD") == git(repo, "rev-parse", "welfare")
    assert git(target, "branch", "--show-current") == "agent/10-default"


def test_explicit_parent_and_linked_checkout(repo):
    first = run(repo, "11", "parent", "agent/parent")
    assert first.returncode == 0, first.stderr
    linked = Path(first.stdout.strip().splitlines()[-1])
    assert git(linked, "rev-parse", "HEAD") == git(repo, "rev-parse", "agent/parent")
    second = run(linked, "12", "child", "agent/11-parent")
    assert second.returncode == 0, second.stderr
    child = Path(second.stdout.strip().splitlines()[-1])
    assert child.parent == linked.parent
    assert git(child, "rev-parse", "HEAD") == git(linked, "rev-parse", "HEAD")
    assert git(repo, "branch", "--show-current") == "welfare"


@pytest.mark.parametrize("args", [(), ("x", "slug"), ("13", "Bad slug"),
                                  ("13", "slug", "missing"),
                                  ("13", "slug", "--help"),
                                  ("13", "slug", "welfare", "extra")])
def test_rejects_invalid_input_without_creating_worktree(repo, args):
    result = run(repo, *args)
    assert result.returncode != 0
    assert not (repo.parent / "worktrees").exists()
    assert git(repo, "branch", "--show-current") == "welfare"


def test_duplicate_target_preserves_existing_checkout(repo):
    first = run(repo, "14", "duplicate")
    assert first.returncode == 0, first.stderr
    target = Path(first.stdout.strip().splitlines()[-1])
    marker = target / "uncommitted.txt"
    marker.write_text("keep this")
    second = run(repo, "14", "duplicate")
    assert second.returncode != 0
    assert marker.read_text() == "keep this"
