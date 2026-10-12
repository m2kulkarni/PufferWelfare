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
2. **Reward blocker.** Add a failing test for Winter reward timing, then fix
   the engine once for both bindings. Currently `c_step` advances the phase
   before checking whether to emit the Winter welfare delta. Preserving this
   bug here does not make rewards suitable for training.
3. **Real 4.0 training.** Isolate the pinned Python package and native module;
   run a small CPU PyTorch training job on Python >=3.10. Verify nonzero reward
   input, finite losses, policy updates, and checkpoint load behavior. Record
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
The focused adapter suite passed 32 tests; the full suite passed 251 tests with
46 existing skips. Independent review reproduced those counts at ce0ba249;
final documentation review will be recorded in the PR. No CUDA build, GPU training,
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
implicit-declaration checking stays enabled. Linux CI run 38183271557 passed
at ce0ba249 (251 passed, 46 skipped). A new documentation commit still requires
its own final review and CI check before readiness.
