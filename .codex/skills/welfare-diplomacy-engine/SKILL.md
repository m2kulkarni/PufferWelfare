---
name: welfare-diplomacy-engine
description: Modify or debug PufferWelfare's Diplomacy C engine, map, Python binding/wrapper, rule-bearing tests, or RL observation/action/reward contract. Do not use for unrelated PufferLib environments or research prose.
---

# Welfare Diplomacy engine work

Read `docs/rules/welfare-diplomacy.md` completely before changing behavior. Use
the pinned sources and local file map there; do not infer rules from historical
status documents or from what the current implementation happens to do.

## Establish the contract

Classify the change before editing:

1. Standard Diplomacy adjudication or map behavior.
2. A Welfare Diplomacy rule: adjustment choice, Winter scoring, time horizon, or
   utility.
3. The RL interface: observations, legal actions, rewards, termination, seeding,
   or buffers.

State the expected behavior and its source. When sources conflict or are silent,
stop the behavioral change, preserve a minimal reproduction, and ask for a human
decision. Do not silently select a convenient interpretation.

## Implement narrowly

- Trace the path through `diplomacy.py` -> `binding.c` -> `diplomacy.c` and, for
  map behavior, `diplomacy_map.c`.
- Write the smallest failing regression test before the fix. Put port-specific
  unit/integration cases in `tests/diplomacy/c_tests/`. Change the copied DATC
  expectations only when translating a cited upstream correction.
- Keep standard and welfare modes separate. Exercise both modes when shared code
  changes.
- If observation or action sizes change, update the C header, encoder/mask logic,
  Python spaces, Torch policy input/output assumptions, config, and shape tests in
  the same change.
- Preserve deterministic seeds and avoid wall-clock, network, or model calls in
  simulator tests.

## Verify

Build only the focused extension:

```bash
python scripts/build_diplomacy_ext.py build_ext --inplace
```

Run the new test first. Then run:

```bash
python -m pytest tests/diplomacy/c_tests tests/diplomacy/original/test_datc.py tests/test_diplomacy.py -q
```

Report the exact command, commit, Python version, platform, and counts for passed,
failed, errored, and skipped tests. A pre-existing failure is not permission to
add another one or to describe the suite as passing.
