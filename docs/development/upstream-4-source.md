# Imported PufferLib 4 source

Source: PufferAI/PufferLib commit
`42f70d6932c30ac977736f861006809c50168ba9` (4.0.0).
The root MIT license retains the PufferAI copyright notice.

These files were imported into this repository and are now ordinary editable
source, not files reconstructed or patched during execution:

- `pufferlib/models.py`, `muon.py`, `pufferl.py`, `selfplay.py`, `sweep.py`,
  and `torch_pufferl.py`;
- `src/bindings_cpu.cpp`, `tensor.h`, and `vecenv.h`;
- the default configuration at `pufferlib/config/default.ini` (also reachable
  through the existing root `config` symlink).

Local changes to the imported trainer preserve pending transition data, make
reward clipping explicit, and include the final-action transition and next-state
value. Their regressions are unchanged apart from importing local source.
Trailing whitespace in the imported Muon/sweep modules and default configuration was
removed. The package initializer is local: it does not import Gym or suppress
warnings for native training, and lazily exposes the legacy `PufferEnv` only
when the adjudication wrapper requests it. The Ocean namespace no longer loads
old 3.0 Torch policies when accessing the retained Diplomacy wrapper.

The old Diplomacy C engine, regression binding and Python wrapper remain in
`pufferlib/ocean/diplomacy`. Keeping them preserves independent rule-test access;
they are not the new trainer. This import does not migrate other legacy Ocean
environments or copied 3.0 utilities.

No CUDA source/build is imported by this CPU slice. GPU training, recurrent
episode handling, the importance-weight discrepancy in issue #24, long-run
lifecycle behavior and fresh package installation remain separate work.
Do not advertise every upstream feature as available merely because a module
was imported. Historical opponents still assume two teams and must not be
enabled for the seven-power game without a separate design and tests.

The removed temporary runtime helper and three patch files remain recoverable
from the parent Git commit. They are no longer used by builds, tests or training.
