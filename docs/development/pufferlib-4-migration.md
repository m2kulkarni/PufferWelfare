# PufferLib 4 migration

The owner selected PufferLib 4 instead of 5. This supersedes the target proposed
in the 5.0 audit; that audit remains useful evidence, not an instruction to port
to 5. The current package is still based on 3.0. Do not change its version number
until the package and trainer actually use 4.0.

Author: Codex, session `pufferlib-4-port`. Tracking: issue #9. Parent: PR #7,
which depends on PR #6. No parent or migration PR may merge without human approval.

## Target and first slice

Pin [PufferLib 4.0 at 42f70d69](https://github.com/PufferAI/PufferLib/tree/42f70d6932c30ac977736f861006809c50168ba9).
Do not follow the moving branch during this port. The first slice compiles the
actual upstream `src/vecenv.h`, `src/tensor.h`, and `src/bindings_cpu.cpp` with a
local adapter. The build verifies their bytes against Git's pinned objects.
There are no copied upstream training files, downloads in the build script, or
changes to the old adjudication engine.

- `native/pufferlib4/binding.c` translates float actions to the old engine's
  integer actions and byte terminal flags to floats. Shapes remain 1641
  observations and one action chosen from 1088 values per power.
- The seven powers stay adjacent in a complete game. The checked Python entry
  point accepts a positive multiple of seven agents, up to 4096 games (28,672
  agents) in this CPU pilot. This is a resource/integer-size guard, not a game
  rule; allocations still depend on available memory. This CPU path requires one
  buffer because upstream `cpu_vec_step` asserts that condition.
- Upstream initializes games before attaching buffers. The adapter allocates
  game state then waits until reset to bind buffers and encode observations.
- A completed game resets immediately, returning next-game observations with
  the completed transition's rewards and terminal flags. Terminal observations
  are not available through this interface. Do not bootstrap through done flags.
- Logs contain floats with `n` last. Completed games contribute once; upstream
  aggregates/clears the adapter logs when queried.
- The Python pilot copies output arrays so they remain valid after close.
  It checks configurations/actions before calling the raw pointer-based binding.
  It is not a sandbox: bypassing it and calling `_C` directly remains unsafe.

## Build and run

Use a separate upstream checkout so this does not replace the old `pufferlib`
package. For example, outside the repository:

```bash
git clone https://github.com/PufferAI/PufferLib.git PufferLib-4-source
git -C PufferLib-4-source checkout --detach 42f70d6932c30ac977736f861006809c50168ba9
```

Install a C/C++ compiler and OpenMP. Ubuntu CI installs `clang libomp-dev`.
On Apple Silicon, `brew install libomp` provides the default prefix
`/opt/homebrew/opt/libomp`; set `PUFFERWELFARE_LIBOMP` for another prefix. This
headless build does not use upstream's x86-only flags or download Raylib.
It uses upstream's CPU binding, including its own CUDA stubs, without pretending
GPU code ran. It does not use `build.sh all`, which can hide failed environment builds.

```bash
python -m pip install -r requirements/pufferlib4-test.txt
python scripts/build_diplomacy_ext.py build_ext --inplace
python scripts/build_diplomacy_puffer4.py --upstream /path/to/PufferLib-4-source
python -m pytest tests/diplomacy/c_tests tests/diplomacy/original/test_datc.py tests/test_diplomacy.py tests/test_agent_worktree.py -q
```

The new tests must fail when the native extension is missing; do not skip them.
The build writes only under `build/pufferlib4`. For a short CPU environment run:

```python
import numpy as np
from native.pufferlib4 import DiplomacyVec

with DiplomacyVec(total_agents=14, max_years=1) as games:
    observations, rewards, terminals = games.step(np.zeros(14))
```

This native-only slice can compile with the existing Python 3.9 test environment.
The full upstream 4 package requires Python >=3.10 and Torch >=2.9. Native adapter
tests on 3.9 are not evidence that the full trainer supports 3.9.

## Remaining work, in order

1. **CPU interface (this PR).** Preserve existing adjudication tests; compare
   old/new transitions in standard and welfare modes, independent games,
   terminal/reset behavior, output lifetime, logging, and input validation.
2. **Reward blocker (issue #11).** Four new tests reproduced dropped Winter
   rewards in both bindings, including final turns (4 failed, 1 standard-mode
   check passed before the fix). `c_step` now remembers the processed phase
   instead of testing the next phase. Scoring rules and adjudication are unchanged.
   The tests cover positive deltas, no repeat reward, reset bookkeeping and the
   adapter retaining final rewards through autoreset. Review and full checks
   remain required; this fix alone does not establish a usable training game.
3. **Real 4.0 training (issue #13).** The isolated smoke run below checks the
   actual upstream CPU PyTorch trainer on Python >=3.10. It checks nonzero reward
   input, finite losses, policy updates, and saved-weight reload. Record
   differences in clipping, recurrence, truncation and training configuration.
   Upstream CPU `create_vec` leaks temporary configuration dictionaries and
   leaves lifecycle management to the caller; inspect/fix these before long runs.
4. **GPU and historical opponents.** Run on approved GPU hardware. Test whole
   seven-power games per buffer, episode-boundary swaps, and which trajectories
   enter the learner. Upstream's native historical pool requires CUDA and uses
   win-rate/Elo assumptions; it is not a seven-player welfare evaluation system.
   Do not enable it merely because an upstream option exists.
5. **Default switch.** Remove or archive obsolete 3.0 training/packaging only
   after the new install and training path are independently checked. Preserve
   the regression adapter or replace it with an equally capable test interface.

The limited action encoding (one order per power), ownership/map/observation
defects, rule-source conflict, and 46 existing skipped tests remain separate
correctness work. Neither random stepping nor a finite-loss smoke run establishes
a usable Welfare Diplomacy benchmark. Keep these limitations in PR handoffs.

## First-slice evidence

Local native build succeeded on macOS arm64, Python 3.9.6, Clang, libomp 23.1.3.
The focused adapter suite passed 28 tests. Full regression results and independent
review will be recorded in the PR at an exact commit. No CUDA build, GPU training,
CPU training, or performance claim is made by this slice.

Setup failures: issue creation with `area:engine` failed because the label is not
configured; creation without labels succeeded. A malformed worktree command was
rejected before changing files. One native build started before upstream checkout
finished and failed on a missing source file; rerunning after checkout succeeded.
The old extension still emits unused-variable/function and signedness warnings.
Independent review found that unbounded agent counts could overflow upstream's
C size arithmetic. Three guarded regressions failed before the limit was added;
they check rejection without calling the unsafe native allocator. A separate
test checks acceptance of the upper bound without allocating thousands of games.
The first Linux CI run failed because strict C11 hid upstream's POSIX clock
declarations. The build now requests POSIX 2008 declarations explicitly;
implicit-declaration checking stays enabled. The fixed adapter passed Linux CI
run 38182492879 (247 passed, 46 skipped). The Winter reward fix passed run
38182603605 (252 passed, 46 skipped).

## CPU training check

Use a separate virtual environment with Python >=3.10. The runtime helper
archives the exact pinned Git object into a temporary directory and applies the
explicit local patches there before loading the native adapter under that
package. It does not alter either checkout. Use a separate process so old and
new packages cannot accidentally share imports.

```bash
python -m pip install -r requirements/pufferlib4-train.txt
# macOS: use --torch-openmp to share Torch's bundled OpenMP runtime.
python scripts/build_diplomacy_puffer4.py --upstream /path/to/PufferLib-4-source --torch-openmp
python tests/test_pufferlib4_rollout_boundaries.py --upstream /path/to/PufferLib-4-source
python scripts/smoke_pufferlib4_training.py --upstream /path/to/PufferLib-4-source
```

On Linux omit `--torch-openmp`; CI installs a CPU Torch wheel and runs this as a
separate job. The flag only changes the newly built adapter's library reference;
it does not modify Torch, Homebrew, or upstream files. On macOS it uses Torch's
headers/library, rewrites its retained Homebrew install name to the Torch rpath,
and ad-hoc signs the generated library. This avoids loading two copies of libomp.
The first training run aborted with OpenMP error 15; no unsafe duplicate-runtime
override was used. The rebuilt run completed. Torch emits an upstream TF32 API
deprecation warning; it has not been suppressed.

The smoke policy uses upstream's MLP and trainer, with the same action mask in
rollout and update: HOLD everywhere, or disband unit zero during Winter. It
starts with a disband-logit bias so a short check actually sees positive rewards.
The script verifies this restriction from recorded observations/actions. This
avoids relying on the existing illegal non-Winter voluntary-disband behavior.
It is a restricted integration test, not a learned or strategic baseline, and
does not demonstrate full order legality or action-mask support in the GPU backend.

On macOS arm64, Python 3.12.14 and Torch 2.9.0, a 336-step run received positive
welfare rewards, had finite losses, changed eight parameter tensors, and restored
identical logits/value predictions after saving, perturbing and reloading weights.
The script prints exact configuration and measurements. Independent review
reproduced the run and full suite (252 passed, 46 skipped); Linux CI run
38183006374 passed both jobs at reviewed commit 3add2322. New heads still require
CI/review re-checks after changes.

Upstream clips rewards to [-1, 1]. The explicit local Torch patch now makes
`train.clip_rewards` a boolean, defaulting to `True` to preserve existing behavior
for other environments. The welfare CPU smoke sets it to `False`: each power's
reward magnitude reaches the compiled advantage function without clipping or
normalization. This follows the accumulated per-power Welfare Points objective
in paper section 2.2 and `docs/rules/welfare-diplomacy.md`; it does not change the
engine's scoring rules. The script reports entries outside [-1, 1], not entries
clipped by the patched trainer.

Run the dedicated check in the training environment:

```bash
python tests/test_pufferlib4_reward_clipping.py --upstream /path/to/PufferLib-4-source
```

The test observes and delegates the actual compiled CPU advantage call from the
pinned Torch trainer. Synthetic distinct rewards for seven powers check raw
magnitudes when disabled, unchanged default/enabled clipping, strict boolean
validation, unchanged stored rewards, and numerical advantages. These synthetic
rewards are interface fixtures, not claims about game scoring. Before the fix,
the disabled test failed (27 of 28 values differed); the default/enabled test
passed, and five invalid-setting cases errored because validation was absent.
The initial test setup also passed a noncontiguous action tensor to upstream's
`view`; correcting it to match the trainer's contiguous layout exposed the real
clipping failure. No expected numerical result was changed to get a pass.

With both explicit patches, the macOS arm64 CPU run on Python 3.12.14/Torch
2.9.0 passes all three clipping checks and the three separate boundary checks.
The restricted 336-step smoke stores 125 reward points, including 33 entries
outside [-1, 1], without clipping; it has finite losses, eight changed parameter
tensors and exact saved-weight predictions. The prescribed simulator suite on
Python 3.9.6 passes 256 tests with 46 existing skips. Linux checks and independent
review of the clipping change are still required; prior-head CI is not evidence
for this new patch.

This patch affects only the isolated Torch runtime. Native CUDA still clamps
rewards in `src/pufferlib.cu` at the pinned upstream commit. No GPU build or
parity test has been run, and the CPU setting is not a claim that native training
supports it. Unclipped reward magnitudes can change optimization stability;
the smoke test is not evidence of long-run learning performance.

A deterministic two-batch regression reproduced the boundary bug
before the local patch: two checks failed and the initial-reset check passed.
The first row of batch two had lost transition three's rewards and terminal flags.
After the patch, all three checks pass and the two-batch transition stream matches
one uninterrupted rollout. The real smoke run now carries 22 pending reward
points and 14 terminal flags into batch two; its stored reward total rises from
103 to 125. Another 24 reward points and 14 flags remain pending after the final
batch, as there is no following rollout in which to store them.

This is a stream-preservation fix, not evidence that every boundary transition
now contributes to learning. Upstream's CPU advantage calculation iterates only
through `horizon - 2` and assigns `rewards[t + 1]` to action `t`. Consequently,
the carried first-row reward is not consumed by that batch's update, while the
preceding batch's final action/value row is no longer available. That conclusion
follows from the pinned CPU source and a direct binding probe: with rewards
`[99, 1, 2, 3]`, zero values and `gamma = lambda = 0`, the actual function
returned advantages `[1, 2, 3, 0]`. The smoke run itself does not measure an
advantage attributable to the carried reward. Correct bootstrapping/last-action
learning is tracked separately in
[#19](https://github.com/m2kulkarni/PufferWelfare/issues/19); it needs its own
regression and may require a broader trainer change.
Native GPU reward clipping, MinGRU episode resets, long-run binding leaks, GPU behavior,
historical opponents, and full optimizer/RNG checkpoint resume also still need
focused tests. Do not turn this smoke script into the default training entry
point without resolving those issues.
