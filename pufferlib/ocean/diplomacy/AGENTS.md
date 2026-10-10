# Diplomacy engine instructions

- Treat `diplomacy.c`, `diplomacy_map.c`, `diplomacy.h`, `binding.c`, and
  `diplomacy.py` as one subsystem. Trace state across the C/Python boundary before
  editing any one layer.
- Read `docs/rules/welfare-diplomacy.md` and use the repository's
  `welfare-diplomacy-engine` skill for rule-bearing or RL-interface work.
- Standard Diplomacy compatibility runs with `welfare_mode=False`. Welfare-only
  behavior must be isolated behind `welfare_mode` and tested in both modes when it
  touches shared adjudication or adjustment code.
- Keep `OBS_*` and `ACTION_*` constants synchronized with the Python spaces,
  encoding code, model inputs in `pufferlib/ocean/torch.py`, and tests.
- Do not add a rule from memory. If the paper, pinned reference implementation,
  and regression tests disagree, preserve the conflict in a failing test or issue
  and request a human rule decision.
- Prefer a small deterministic state built through the adapter/binding over a
  long simulated game when testing adjudication, ownership, scoring, or rewards.
