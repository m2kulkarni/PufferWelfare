# PufferLib 5.0 build, platform, test, packaging, and workflow audit

Audit target: `PufferAI/PufferLib` branch `5.0`, pinned commit
`6ffa5b10dbbbe4d1e8288367c7d9d3acd3bad4a2`.

This is a source audit. I did not run PufferTank's installer or Docker scripts,
install packages, or modify the pinned upstream checkout. Documented features,
source-visible implementations, and locally/CI-tested behavior are kept
separate below.

## Bottom line

PufferLib 5.0 is not a normal dependency update for PufferWelfare. The fork is
a Python package with Gym/Gymnasium/PettingZoo-style wrappers and compiled C
extensions. The target branch has no Python package or package manifest; it is
a native CUDA trainer with the selected environment compiled into the binary.
The common ancestor is `2755f633`; upstream has 1,279 commits and the pre-setup
fork has 13 commits after that point. A direct merge changes 1,150 paths (662
added upstream, 330 deleted, plus modifications/renames). Treat this as an
adapter port with a pinned upstream reference, not as a bulk merge or `pip`
upgrade.

The most important operational limit is that training requires the NVIDIA CUDA
toolchain. The `--cpu` mode is standalone evaluation/rendering, not CPU
training. The source contains useful implementation examples, but the pinned
commit has no current CI workflow, no unified test command, several stale test
entry points, and no successful build verified in this audit.

## Build and platform evidence

| Capability | What the source shows | What is actually verified here |
| --- | --- | --- |
| Native training | `build.sh:479-528` invokes `nvcc` and links CUDA runtime, NCCL, NVML, cuBLAS, cuSolver, and cuRAND even when the environment itself is a CPU `.h` environment. | Not built or run. No `nvcc` is available on the audit Mac. |
| CPU environments | The default trainer selects `ocean/<env>/<env>.h` (`build.sh:448-458`) and runs host environments alongside the GPU learner. | Source-visible, not locally trained. “CPU environment” does not mean CPU-only training. |
| Standalone CPU eval | The active `--cpu` branch compiles `src/puffercpu.c` (`build.sh:279-313`). Official docs call this CPU eval/rendering and explicitly say 5.0 has no CPU training. | A separate clean scratch checkout attempted `bash build.sh minimal --cpu --debug` on Darwin arm64. Raylib downloaded, then stock macOS Bash 3.2.57 failed at `build.sh:208` on `${ENV^^}` with `bad substitution`; compilation never started. |
| Mac support | Non-Linux platforms are all treated as macOS; the script uses Homebrew `libomp` and Apple frameworks (`build.sh:81-96`). | The stock-Mac Bash failure above is a real incompatibility at the pinned commit. In addition, `SANITIZE_FLAGS` is empty on Mac, so `--cpu --debug` does not provide the ASan/UBSan advertised generically in the docs. |
| Linux support | Raylib is hard-coded to `raylib-5.5_linux_amd64`; AVX2/FMA is enabled on x86-64 (`build.sh:83-88`, `228-232`). | The source targets Linux amd64. Linux arm64 is not supported by the bundled Raylib selection. No Linux upstream build was run here. |
| Windows | There is no Windows branch. A non-Linux `uname` falls into the macOS/Homebrew/framework path. | Native Windows is not supported by this build script. WSL/container use would be Linux, not native Windows support. |
| GPU environments | `--cu` selects a matching `.cu`; `robot_arm` forces this and rejects CPU/web (`build.sh:53-61`, `448-455`). | Source-visible only. |
| Web | `build.sh:314-397` has an Emscripten path and optional website-repository policy/config overlays. | Not built. It depends on separately installed Emscripten and external website assets. |
| Multi-GPU | Native code/config contains NCCL setup and multiple local GPU worker processes. | Implemented in source, but no multi-GPU or multi-node run was verified. Do not describe it as scheduler/orchestrator support. |

Two build-script details should be stated precisely:

- `build.sh:398-420` is a duplicate, unreachable second `elif [ "$MODE" =
  "cpu" ]` after the active CPU branch already exits. It references the absent
  `src/puffercpu.h`. This is dead/stale source, not evidence that the active
  `--cpu` branch is broken for that reason.
- `build.sh all` records failed environment builds but always exits zero
  (`build.sh:63-79`). It is therefore not a safe CI gate without changing its
  exit behavior.

The official documentation currently advertises up to 60 million steps/second,
60+ environments, CUDA graphs, buffered CPU vectorization, asynchronous PPO,
and a sub-second RTX 5090 demo. Those are upstream claims and source features,
not measurements on PufferWelfare or this machine. The same documentation says
there is no CPU training in 5.0:
<https://puffer.ai/docs.html>.

## Packaging and supply-chain findings

The pinned tree has no `pyproject.toml`, `setup.py`, requirements/lock file,
Makefile/CMake project, package version metadata, or current `.github`
directory. The only visible 5.0 release-like tag is `5.0-experiments`, two
commits behind the pinned head; `5.0` is a moving branch. Pin the exact commit,
not just the branch or tag description.

`build.sh` is also a dependency installer:

- Raylib 5.5 is downloaded and extracted without a checksum
  (`build.sh:110-126`).
- Box2D uses a mutable `releases/latest/download` URL and no checksum
  (`build.sh:168-177`).
- NetHack clones the current tip of `FinlaySanders/fast-nle` with
  `--depth 1`, with no commit pin (`build.sh:182-200`; the web helper does the
  same).
- OSRS assets are better: `ocean/osrs/scripts/setup-data.sh` verifies the
  archive SHA-256 from its manifest. It still performs a network download as a
  side effect of the normal OSRS build.
- Boxoban's C runtime can invoke `curl` to fetch map archives
  (`ocean/boxoban/boxoban_maps.h:225-241`) without a visible checksum in that
  path.

The companion `PufferAI/PufferTank` 5.0 installer is not a reproducible release
boundary. At pinned PufferTank commit
`240d9794872fc1f9e0948588d1f453dbbc15b8f5`, `install.sh` runs `apt-get` as
root, can download an unchecked CMake tarball into `/usr/local`, and clones the
moving PufferLib `5.0` branch. Its image starts from
`nvcr.io/nvidia/cuda:13.0.2-cudnn-devel-ubuntu24.04` by tag rather than digest
and downloads editor tooling without hashes. `docker.sh test` is an interactive
container launcher, not an automated test: it grants GPUs, host networking,
host IPC/cgroup namespace, X11 credentials, and the host Docker socket; its
build path also stops/removes a container with the selected name. Do not run
these scripts in agent CI. Build a project-owned, digest/checksum-pinned image
after owner review if a CUDA environment is needed.

## Tests and CI

There is no current workflow in the pinned upstream tree. GitHub's read API
returned no pull-request workflow runs and no commit statuses for the pinned
merge commit. This does not prove upstream never tested it; it does mean there
is no machine-readable green gate attached to this pin that we can rely on.

The tree has test material, but not one coherent suite:

- `tests/` contains 18 `test_*.py` files with 51 pytest-shaped functions, 10
  `test_*.c`/`.cu` files, and numerous benchmark/study scripts.
- At least five Python test modules import a `pufferlib` package that does not
  exist in the 5.0 tree (`tests/test_api.py`, `test_league.py`,
  `test_pufferl_train.py`, `test_puffernet.py`, `test_selfplay.py`). An isolated
  `pytest tests/test_selfplay.py --collect-only` check failed with
  `ModuleNotFoundError: No module named 'pufferlib'`; installing the old fork
  would test the wrong code.
- Some native test recipes are stale: `tests/test_nmmo3_cuda.cu` includes the
  absent `../pufferlib/src/models.cu`; `tests/test_protein.cu` and
  `test_protein_sweep.cu` include absent `gp_cuda.cu`; a NetHack comment points
  to absent `tests/build_test.sh`; the Craftax benchmark comment names a moved
  source path.
- There are useful standalone suites under `ocean/affine_lock/tests` and
  `ocean/bat/tests`, 27 OSRS C test files, and a detailed tower-defence test,
  but only the first two have `run_all.sh` scripts and there is no top-level
  runner. The bat runner is Linux-Raylib-specific. OSRS documents one manual C
  command, not all of its tests.
- Documentation is also stale in places: `ocean/tower_defence/README.md`
  recommends `build.sh ... --fast`, but `build.sh` rejects `--fast` as an
  unknown argument.

Therefore report tests individually. Do not say “the upstream suite passes” or
“all upstream environments build.” The merged PufferWelfare CI result (210
passed, 46 skipped on Ubuntu/Python 3.11 before the stacked-workflow helper) is
evidence for the old Diplomacy engine only, not for 5.0 compatibility.

Suggested migration gates, in order:

1. Keep the existing old-engine suite as the behavior reference.
2. Add a network-free Linux CPU compile/run smoke for only the new Diplomacy
   adapter, with pinned dependencies.
3. Compare seeded engine state, orders, phase transitions, rewards, resets, and
   player identities between the old wrapper and native adapter.
4. Add the Mac/Bash portability check if local Mac CPU eval remains supported.
5. On an owner-approved NVIDIA host, run a tiny FP32 synchronous train and
   checkpoint/eval test before BF16, CUDA graphs, asynchronous rollout, or
   multi-GPU claims.
6. Preserve failures and skips in CI output; do not use `build.sh all` as a
   pass/fail gate in its current form.

## License and provenance

The root project is MIT (`LICENSE`), but that is not a complete third-party
inventory:

- `vendor/collections-c/include/cc_array.h` and `cc_common.h` declare
  LGPL-3.0-or-later. They are used by Impulse Wars, and no full LGPL license
  file is present at the repository root. Redistribution/linking obligations
  need owner/legal review before copying or shipping that environment.
- cJSON carries MIT text; Raygui/Rcamera/Rlights carry zlib-style notices;
  Khronos headers carry their own permission notice; dlmalloc states public
  domain. Preserve these source notices.
- Chess records CC BY-SA 3.0 artwork in a source comment
  (`ocean/chess/chess.h:2865-2866`).
- NMMO3's asset notice says Mana Seed assets were commercially licensed and
  may not be repurposed without purchasing a license
  (`resources/nmmo3/ASSETS_LICENSE.md`).
- Robot-arm assets have an Apache-2.0 attribution/license pair; tower-defence
  has a contributor provenance manifest and MIT grant.
- Most resource directories have no adjacent license/attribution file. That is
  not proof of infringement, but there is no comprehensive NOTICE/SBOM that
  makes bulk redistribution safe to assume.

PufferWelfare has its own unresolved issue: the root is MIT, while the copied
and modified `tests/diplomacy/original/test_datc.py`, `test_game.py`, and
`test_map.py` carry AGPL-3.0-or-later notices. Preserve those notices and obtain
an owner/legal provenance decision; do not silently relicense, delete, or
rewrite expected results merely to avoid the question.

The safe migration policy is to copy only necessary PufferLib source at the
pinned revision, retain all notices, record origin/SHA per imported file, and
avoid importing unrelated environments or assets.

## Migration-specific interface risks

- Upstream 5.0's contract is `obs_t` plus `puf_init`, `puf_reset`, `puf_step`,
  `puf_render`, `puf_close`, and `puf_log` around `Agent` buffers
  (`src/pufferenv.h:21-57`). Existing Python bindings cannot be moved unchanged.
- The fork's current RL interface gives each of seven powers one `Discrete(1088)`
  action per phase (`pufferlib/ocean/diplomacy/diplomacy.py:43-55`). The C path
  decodes one order per power and fills every other unit with HOLD
  (`diplomacy.c:4400-4460`); convoy action slots are explicitly simplified to
  HOLD (`diplomacy.c:4363-4366`). That is not a complete simultaneous Diplomacy
  order interface. A mechanical adapter would preserve a known-invalid action
  contract.
- Upstream factorized action heads and masks can represent a designed order
  schema, but they do not automatically solve conditional legality or whole-set
  order consistency. Version the action/observation schema and test all-masked,
  phase-specific, build/retreat, support, and convoy cases.
- Upstream's native checkpoint is a flat weights file, not a complete training
  state. Record schema/upstream SHA/config/seeds separately and do not promise
  exact resume without adding optimizer/RNG/episode state.

## Official `gh-stack` workflow

The installed tools are GitHub CLI 2.102.0 and official `github/gh-stack`
0.2.1. PufferLib itself contains no gh-stack configuration; this is project
workflow tooling, not an upstream PufferLib convention.

Verified official behavior relevant to the project:

- Stacks require all branches in the same repository. Each PR targets the
  branch immediately below it, so review sees only that layer.
- Local metadata is stored in Git's common directory, so linked worktrees share
  it. The tool can update clean branch-owning worktrees, but it does not create,
  remove, switch, or auto-stash them. Its mutation lock coordinates only other
  gh-stack processes, not arbitrary Git commands or coding agents.
- `gh stack submit --auto` pushes and opens/updates PRs; new PRs are drafts by
  default unless `--open` is passed.
- `gh stack push` uses per-branch force-with-lease and is not atomic; some
  branches may update before another lease fails. Recheck every head/check
  after it.
- In installed 0.2.1, `gh stack sync` fetches, cascade-rebases, and performs an
  atomic force-with-lease push. It can rewrite every layer, so it still needs a
  single coordinator and idle/clean affected worktrees.
- `gh stack merge` cannot bypass GitHub requirements. GitHub applies the bottom
  PR's base-branch rules, CODEOWNERS, and default-branch PR CI to every layer.
  Stacks merge bottom-up; the command can merge several PRs at once.

Recommended role split: authors edit and commit only their assigned worktree;
reviewers inspect a fixed commit and do not mutate the stack; one coordinator
alone runs `init`, `submit`, `rebase`, `sync`, or `push`, after checking all
authors' worktrees are clean/idle. Use stacks only for actual dependencies, not
parallel independent issues. Keep every PR draft until a different session has
reviewed it and the human owner explicitly approves it. Never use `gh stack
merge` as a substitute for that approval.

Local evidence: `gh stack view --json` shows issue-5's workflow branch based on
`welfare` and issue-3's audit branch based on issue 5. The first publish attempts
were rejected for missing workflow scope and an older credential selected by the
system Git helper. The owner refreshed the CLI scope; the repository-local helper
was set to use that authenticated CLI. The official extension then published
draft PRs #6 and #7 in stack #8. No merge or direct `welfare` push occurred.

Official references:

- <https://github.com/github/gh-stack/tree/v0.2.1>
- <https://docs.github.com/en/pull-requests/how-tos/create-pull-requests/creating-stacked-pull-requests>
- <https://docs.github.com/en/pull-requests/get-started/about-stacked-prs>

## Independent review of stacked-workflow helper

Reviewed author commit `6d000604ff51581b078b3d96a892126c81b516b5`
as session `/root/governance_audit`. `git diff --check` and shell syntax passed;
the worktree helper's focused tests passed 9/9, and the combined gate passed 219
with 46 skipped on macOS using Python 3.9.6.

The initial review found that CI added `tests/test_agent_worktree.py` while the
mandatory “full command above” in `AGENTS.md` still omitted it. Author follow-up
commit `4cd175015c38bd56e04d40f567c8b574e1fe184c` adds that path to the shared
command. I checked that focused one-line delta and `git diff --check`; the
finding is resolved. No other blocking finding was identified in the inspected
workflow/helper change.
