# PufferLib 4 migration

The owner selected PufferLib 4, not 5. The imported source is pinned to upstream
commit `42f70d6932c30ac977736f861006809c50168ba9`.
See [source provenance](upstream-4-source.md) for imported files and local changes.

## Current state

The Python trainer and CPU vector backend now live in this repository. Edit
these files directly. Builds and tests no longer reconstruct a temporary
upstream package or apply patch files.

The source initializer reports `4.0.0.dev0`. Package installation metadata and
the old general setup are not migrated by this source-only slice. This is not a
completed package migration, GPU port, or usable Welfare Diplomacy benchmark.

The legacy C engine, regression binding and wrapper remain available for rule
tests. The initializer lazily provides their `PufferEnv` compatibility class
without importing Gym into the native trainer.

## CPU architecture

- `src/bindings_cpu.cpp`, `vecenv.h` and `tensor.h` are local upstream 4 source.
- `native/pufferlib4/binding.c` adapts the existing Diplomacy engine to that
  vector interface. Actions are floats at the vector boundary and integers in
  the engine; terminal flags are converted from bytes to floats.
- Shapes remain 1641 observations and one action chosen from 1088 values.
- Powers remain adjacent in complete seven-power games. The checked
  `DiplomacyVec` accepts a multiple of seven agents up to 28,672, exactly one
  CPU buffer, and 1–1000 years. The cap is a resource/integer-size guard, not a
  game rule. Raw pointer bindings are still unsafe if callers bypass validation.
- Terminal games reset immediately. Returned observations belong to the new
  game; rewards and terminal flags belong to the completed transition.
  The learner must mask bootstrap values at terminals.
- The Python wrapper copies returned arrays and rejects use after close.
  Logs aggregate each completed game once.

## Build and check

Install Clang and OpenMP first. Ubuntu needs `clang libomp-dev`. Apple Silicon
uses Homebrew libomp at `/opt/homebrew/opt/libomp` by default; another prefix can
be supplied with `PUFFERWELFARE_LIBOMP`.

The native-only regression adapter can still build with Python 3.9. The actual
upstream 4 trainer needs Python >=3.10 and Torch >=2.9. Test those in separate
processes and report which Python version ran each check.

```bash
python -m pip install -r requirements/pufferlib4-test.txt
python scripts/build_diplomacy_ext.py build_ext --inplace
python scripts/build_diplomacy_puffer4.py
python -m pytest tests/diplomacy/c_tests tests/diplomacy/original/test_datc.py tests/test_diplomacy.py tests/test_agent_worktree.py -q
```

For actual training checks, use the training requirements in a Python >=3.10
environment. On macOS the build must share Torch's OpenMP runtime; otherwise
loading both runtimes caused a real OpenMP error 15. The flag changes and signs
only the generated extension, not Torch or Homebrew. No duplicate-runtime
override is used.

```bash
python -m pip install -r requirements/pufferlib4-train.txt
# On Linux omit --torch-openmp.
python scripts/build_diplomacy_puffer4.py --torch-openmp
python tests/test_pufferlib4_rollout_boundaries.py
python tests/test_pufferlib4_reward_clipping.py
python tests/test_pufferlib4_final_action_bootstrap.py
python scripts/smoke_pufferlib4_training.py
```

The extension is imported normally as `pufferlib._C`. Neither sequence needs
another checkout, Git history at runtime, a source download, or a patch step.
Compiler warnings and the upstream Torch TF32 warning have not been suppressed.

## Preserved fixes and their limits

Winter rewards are emitted on the transition that processes Winter, including
the final turn (issue #11). Engine scoring and adjudication rules did not change.

The trainer preserves pending reward/terminal buffers between rollout batches
(issue #15). Reward clipping is a strict boolean setting, enabled by default;
the welfare smoke disables it so per-power magnitudes are retained (issue #16).

The final sampled action now supplies its transition and next-state value
through an extra advantage-source row (issue #19). This row samples no action
and does not increase batch size or step count. It is clipped under the same
setting as other rewards. Its next-state value is masked by the compiled
terminal flag. Snapshots and the extra query's recurrent state are cloned;
the returned state is discarded.

Regressions delegate to the real compiled CPU advantage function and compare
numerical results with an explicit oracle. They test terminal and nonterminal
endings, a one-action horizon, snapshot ownership, and state-query isolation.
Split/uninterrupted equality is established only with feedforward state and
lambda zero. Nonzero lambda cuts a trace at the batch boundary; general equality
is not claimed. Issue #24 separately tracks inconsistent importance-weight
formulas when ratios differ from one.

The smoke policy is deliberately restricted to HOLD, or disband unit zero
during Winter, with an initial disband bias. It applies the same mask during
rollout and update and does not exploit illegal non-Winter disbands. It is an
integration test, not a learned baseline or a complete legal-order policy.
The local direct-source run has 336 steps, 125 stored reward points and 149
reward points belonging to sampled actions. Losses are finite, eight parameter
tensors change, and saved-weight predictions reload exactly. That is not full
optimizer/RNG resume or proof of long-run learning.

## Remaining migration work

1. Fix native lifecycle leaks and unsafe repeated close/use-after-close behavior.
2. Resolve issue #24 against an authoritative algorithm source and test nonunit
   weights; do not claim GPU parity from a CPU test.
3. Replace the old setup and verify a fresh wheel/install and default entrypoint.
   Do not merely change a version number or require Git during installation.
4. Validate recurrent episode handling and checkpoint/resume before enabling
   those paths. Unsupported options should fail clearly, not run silently.
5. Port and test GPU training on approved hardware before advertising support.
   The CPU build supplies upstream's own unused CUDA stubs; it is not GPU code.
   Historical opponents assume two teams and are not a seven-power welfare
   evaluation system. They require a separate design and tests.
6. Obtain human approval and merge reviewed changes in dependency order.

Limited action encoding, ownership/map/observation defects, the paper/reference
rule conflict, no private press, and 46 existing skipped tests remain simulator
work. The migration must not hide them or claim game correctness it has not
tested. Current-commit independent review and CI are required for every PR;
passing a previous head is not evidence for a changed source import.
