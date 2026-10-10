# Working on Diplomacy

- Follow the root `AGENTS.md`: write clearly, report problems, and do not hide
  failures or change tests merely to make them pass.
- The C engine, map, header, binding, and Python wrapper work together. Follow
  state through these files before changing behavior.
- Read `docs/rules/welfare-diplomacy.md` and use the engine skill for game or
  learning-agent interface changes. Cite rules rather than guessing them.
- Standard adjudication tests use `welfare_mode=False`. Keep welfare-only
  behavior behind `welfare_mode`; test both modes when shared code changes.
- Keep `OBS_*` and `ACTION_*` constants consistent with encoding, Python spaces,
  model inputs in `pufferlib/ocean/torch.py`, and tests.
- If the paper, reference code, and tests disagree, describe the disagreement
  and keep a small reproduction. Get a human rule decision before choosing.
- Use small, repeatable game states to test orders, ownership, scoring, and
  rewards. Report missing coverage and checks you could not run.
