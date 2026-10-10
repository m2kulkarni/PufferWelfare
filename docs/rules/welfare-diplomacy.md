# Welfare Diplomacy rule sources and local contract

This document prevents implementation history from quietly becoming a game rule.
It describes source priority and the behavior that is currently safe to assume.

## Source priority

1. **Welfare variant semantics:** Section 2.2 of *Welfare Diplomacy:
   Benchmarking Language Model Cooperation* (arXiv:2310.08901v1).
2. **Reference behavior:** the authors' implementation at
   `mukobi/welfare-diplomacy`, pinned for comparison at commit
   `60520aaa4661e4c95abfd30ce6483990bb127567`.
3. **Standard adjudication regression oracle:**
   `tests/diplomacy/original/test_datc.py`, run with `welfare_mode=False`, plus the
   pinned reference engine when a copied case is ambiguous.
4. **PufferWelfare interface decisions:** executable tests for the C binding,
   wrapper, observation/action encoding, rewards, termination, and seeding.

The paper and reference implementation answer different questions. The paper is
the semantic source for the variant. The pinned implementation is the compatibility
source for operational details not fully specified in the paper. Record conflicts
instead of hiding them.

`docs/PLAN.md`, `docs/STATUS.md`, `docs/ORIGINAL_CODE.md`, comments, commit
messages, and the current C behavior are useful audit evidence but are not
normative rule sources.

## Paper-level Welfare Diplomacy changes

Relative to Standard Diplomacy:

- After each Winter, a power gains Welfare Points equal to its controlled supply
  centers minus its units on the board.
- Welfare Points accumulate over the game.
- The game ends after a fixed number of years rather than at 18 supply centers.
- Each power maximizes its own accumulated Welfare Points; there is no single
  winner.

The paper also describes players choosing fewer units than their supply-center
count during build turns. It does not license voluntary disbands in movement or
retreat phases.

## Pinned reference details

The reference engine implements the choice during the Winter adjustment phase:

- A power may build fewer units, waive builds, or disband existing units.
- A single adjustment set cannot mix build and disband order types.
- The adjustment processing path accumulates welfare. The reference also appears
  to score through `_other_results` during retreat processing; this conflicts
  with the paper's Winter-only description and needs a minimal reproduction and
  human decision before claiming exact reference parity.
- The implementation asserts that `centers - units` is non-negative at scoring
  time.

Treat adjustment-only voluntary disbanding as the compatibility target unless a
research issue explicitly proposes and justifies another variant.

## Local architecture

- `pufferlib/ocean/diplomacy/diplomacy.c`: state transitions, adjudication,
  adjustments, scoring, reward production, termination, and encoding.
- `pufferlib/ocean/diplomacy/diplomacy_map.c`: standard map and ownership metadata.
- `pufferlib/ocean/diplomacy/diplomacy.h`: shared state and interface constants.
- `pufferlib/ocean/diplomacy/binding.c`: Python access and test adapter hooks.
- `pufferlib/ocean/diplomacy/diplomacy.py`: PufferEnv spaces and lifecycle.
- `tests/diplomacy/original/test_datc.py`: standard-mode adjudication regression
  suite.
- `tests/diplomacy/c_tests/`: binding, map, adjudication, and welfare-specific
  tests.

Private press/messaging from the paper's experiments is not implemented by the
current local simulator. Do not make communication assumptions part of engine or
baseline work without a separate design issue.

## Required evidence for a rule change

A rule-bearing issue and pull request must include:

- the exact source and passage, reference-code location, or named DATC case;
- a minimal state, orders, phase, and expected transition;
- the standard- and welfare-mode impact;
- a regression test and the exact focused/full test results; and
- an explicit human decision when the sources conflict or define a new variant.
