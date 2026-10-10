#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "usage: $0 <issue-number> <lowercase-hyphenated-slug> [parent-branch]" >&2
  exit 2
fi

issue_number="$1"
slug="$2"
base="${3:-origin/welfare}"

if [[ ! "$issue_number" =~ ^[0-9]+$ ]]; then
  echo "issue number must contain digits only" >&2
  exit 2
fi

if [[ ! "$slug" =~ ^[a-z0-9]+(-[a-z0-9]+)*$ ]]; then
  echo "slug must be lowercase letters/digits separated by single hyphens" >&2
  exit 2
fi

repo_root="$(git rev-parse --show-toplevel)"
common_git_dir="$(git rev-parse --path-format=absolute --git-common-dir)"
primary_root="$(dirname "$common_git_dir")"
worktree_parent="$(dirname "$primary_root")/worktrees"
branch="agent/${issue_number}-${slug}"
worktree_path="${worktree_parent}/${issue_number}-${slug}"

if [[ -e "$worktree_path" ]]; then
  echo "worktree path already exists: $worktree_path" >&2
  exit 1
fi

if [[ "$base" == "origin/welfare" ]]; then
  git -C "$repo_root" fetch origin welfare
fi
base_commit="$(git -C "$repo_root" rev-parse --verify --end-of-options "${base}^{commit}")"
mkdir -p "$worktree_parent"
git -C "$repo_root" worktree add -b "$branch" "$worktree_path" "$base_commit"

printf '%s\n' "$worktree_path"
