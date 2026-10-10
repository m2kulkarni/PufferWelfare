# PufferWelfare agent instructions

## Repository contract

- The active development branch is `welfare`. Do not commit directly to it.
- Use one issue, branch, and worktree per coding agent. Name branches
  `agent/<issue>-<short-slug>` and give each agent only its own worktree path.
- Keep non-trivial changes narrowly scoped to an issue with explicit acceptance
  criteria, non-goals, and a rule or interface source when behavior is involved.
- Do not treat `docs/PLAN.md`, `docs/STATUS.md`, commit messages, or current code
  as authoritative game rules. They are implementation history and may be stale.

## Review and merge

- The author agent must not review its own change. Use a fresh agent context for
  review and record both agent identities in the pull request.
- Agents sharing one GitHub account cannot provide an independently attributable
  GitHub approval. In that case, the reviewer leaves a `COMMENT` review and the
  orchestration record provides the separation.
- Agents do not merge pull requests during the pilot. A human owns every merge.
- Human review is mandatory for game-rule or adjudication changes, reward/action/
  observation changes, CI and dependency changes, agent instructions, repository
  permissions, and branch-protection settings.

## Diplomacy changes

- For changes under `pufferlib/ocean/diplomacy/`, its bindings, its model/config,
  or `tests/diplomacy/`, use the `welfare-diplomacy-engine` skill in
  `.codex/skills/` and read `docs/rules/welfare-diplomacy.md` first.
- For reviews of those changes, use the `welfare-diplomacy-review` skill.
- Preserve the boundary between the C engine, Python binding/wrapper, and the RL
  observation/action/reward interface. Coupled constants must change together.
- Add a focused regression test before fixing behavior. Never weaken or rewrite a
  golden adjudication expectation merely to make the implementation pass.

## Validation

Set up the focused test environment with:

```bash
python -m pip install -r requirements/diplomacy-test.txt
python scripts/build_diplomacy_ext.py build_ext --inplace
```

Then run the narrowest relevant test, followed by the full Diplomacy gate:

```bash
python -m pytest tests/diplomacy/c_tests tests/diplomacy/original/test_datc.py tests/test_diplomacy.py -q
```

Do not update pass counts in documentation unless that exact command was run and
the Python version, platform, commit, failures, errors, and skips are recorded.
