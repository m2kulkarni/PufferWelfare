#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <issue-number> <lowercase-hyphenated-slug>" >&2
  exit 2
fi

issue_number="$1"
slug="$2"

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

mkdir -p "$worktree_parent"
git -C "$repo_root" fetch origin welfare
git -C "$repo_root" worktree add -b "$branch" "$worktree_path" origin/welfare

printf '%s\n' "$worktree_path"
