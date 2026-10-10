---
name: welfare-diplomacy-engine
description: Change or debug PufferWelfare's Diplomacy C engine, map, Python binding/wrapper, tests, or observations/actions/rewards. Does not cover other PufferLib games or research writing.
---

# Working on the simulator

This is shared guidance for any coding agent. Use the same procedure regardless
of which tool runs the session.

Follow the plain-language and honesty rules in `AGENTS.md`. Read
`docs/rules/welfare-diplomacy.md` before changing behavior.

## Understand the problem

Say whether the change affects standard adjudication, Welfare Diplomacy rules,
or the interface used by learning agents. Explain the expected result and cite
its source. Do not invent rules from memory or treat existing bugs as intended
behavior. If the sources disagree or do not settle a rule, describe the problem,
keep a small reproduction, and get a human decision before choosing a rule.

## Make the fix

- Follow the code through `diplomacy.py`, `binding.c`, and `diplomacy.c`.
  Check `diplomacy_map.c` for map changes.
- Add a small test that fails for the original bug. Put local tests in
  `tests/diplomacy/c_tests/`. Do not change copied DATC expectations without a
  cited correction to the source test or rule.
- Keep welfare-only behavior behind `welfare_mode`. Test both modes when shared
  code changes.
- If observation or action sizes change, update the C header, encoding and masks,
  Python spaces, Torch policy, config, and tests together.
- Keep tests repeatable with fixed seeds. Simulator tests must not depend on
  time, network services, or model calls.
- Report other defects you find. Do not hide failures or add skips to get a
  passing result. Explain and independently review any test correction.

## Check the result

Build with `python scripts/build_diplomacy_ext.py build_ext --inplace`.
Run the new test first, then the full test command in `AGENTS.md`.
Report the command, commit, Python version, platform, failures, errors, and skips.
If the fix is incomplete or a check cannot run, say so. A passing test suite
does not establish that skipped or untested game behavior is correct.
