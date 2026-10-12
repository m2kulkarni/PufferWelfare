# Instructions for every agent

These instructions apply to every collaborator, including Codex and Claude Code.
Neither tool has a special role or authority. Either can implement or review a
change, but the reviewer must be a different session from the author.
Keep shared rules here; tool-specific entry files must only point to them.

## Write clearly and report honestly

- Write issues, PRs, reviews, comments, and instructions in plain language.
  Say what is wrong, what changed, how you checked it, and what remains.
  Explain technical terms when needed. Avoid filler and vague claims.
- Report problems you find, including existing bugs, failed or skipped tests,
  checks you could not run, and disagreements between rule sources. Separate
  what you observed from what you suspect. Link code or a reproduction.
- Do not hide failures, disable checks, add skips to avoid failures, or change
  expected results just to make tests pass. If a test is outdated or wrong,
  explain why with evidence and have another agent review the correction.
- Do not claim tests passed unless you ran them. Passing tests do not prove
  behavior the tests never exercise. Do not call unfinished work complete.
- Fix the assigned problem. Report unrelated problems in the issue or a separate
  issue; do not silently ignore them or expand the change without explaining it.

## Work on one issue at a time

- The development branch is `welfare`. Do not commit directly to it.
- Each coding agent gets its own branch and worktree (a separate checkout).
  Name branches `agent/<issue>-<short-slug>`. Use only your assigned checkout.
- Record the tool and a unique session name in the issue/PR. Before handing off,
  record the branch, commit, changes, test results, and remaining problems.
  Another collaborator must be able to continue without your private chat history.
- The issue must explain the intended result, how to check it, and what is outside
  the change. Cite a rule source when changing game behavior.
- Old plans, status documents, comments, and commit messages can be wrong.
  Current code describes what happens, not necessarily what should happen.

## Review and approval

- A different agent must review the change. Record author and reviewer names.
- If both agents use the same GitHub account, the reviewer leaves a `COMMENT`
  review. Do not describe it as an independent GitHub approval.
- Keep a PR draft while writing it or resolving review findings. Before marking
  it ready for the owner's review, check that a different agent reviewed the
  current commit, current CI passes, no blocking finding or unresolved review
  thread remains, and the description accurately reports scope and test evidence.
  Record the checked commit in the PR, mark it ready for review, and notify the
  owner. Disclose existing defects, skipped tests, and checks that were not run.
- If the PR receives a new commit after it was marked ready, return it to draft
  until the changed commit passes that review check again.
- Ready for review is not approval to merge. The owner will read the entire PR
  and discuss it before approving. Do not merge without explicit human approval.
- Human review is required for game rules, rewards, actions, observations, CI,
  dependencies, agent instructions, permissions, and branch protection.

## Simulator changes

- Read `.agents/skills/welfare-diplomacy-engine/SKILL.md` for Diplomacy engine,
  binding, wrapper, model/config, or test changes. Read
  `docs/rules/welfare-diplomacy.md` first.
- Reviewers read `.agents/skills/welfare-diplomacy-review/SKILL.md`.
- Read applicable `AGENTS.md` files in the directories you change. If your tool
  does not load these files or skills automatically, open them explicitly.
- Follow state through the Python wrapper, binding, and C engine. When changing
  observation or action sizes, update every dependent layer together.
- Before fixing behavior, add a small test that fails for the original bug.
  Do not change expected adjudication results without a cited rule correction.

## Build and test

```bash
python -m pip install -r requirements/diplomacy-test.txt
python scripts/build_diplomacy_ext.py build_ext --inplace
python -m pytest tests/diplomacy/c_tests tests/diplomacy/original/test_datc.py tests/test_diplomacy.py -q
```

Run the relevant test first, then the full command above. Report the command,
commit, Python version, platform, and passed/failed/errored/skipped counts.
If a check cannot run, say why and leave it listed as unfinished.
