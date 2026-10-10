# PufferLib 5.0 audit

Audit date: 10 October 2026. Author: Codex session `/root`.

This is a source audit, not a successful training run. Migration has not started.
Build/platform and environment findings are being checked by other sessions.

## Versions and the size of the change

The target is upstream branch `5.0`, commit
`6ffa5b10dbbbe4d1e8288367c7d9d3acd3bad4a2`. The `5.0-experiments` tag points
to `89414204`; it contains experiments/models, not a separately versioned Python
package. The branch and release are not interchangeable pins.

Our development branch is `welfare`, now at `f398421f` after setup PR #1.
Its pre-setup head was `62df16a0`. The common ancestor with upstream is
`2755f633`, dated 13 October 2025. From that ancestor, upstream has 1,279
commits and the pre-setup fork has 13. Commit counts describe divergence, not
the amount of work required.

The old fork has `setup.py`, Python wrappers, a PyTorch policy/trainer, and
`pufferlib/ocean/diplomacy`. The target has `build.sh`, `src/*.cu`, `ocean/*`,
and `config/*.ini`, with no `pufferlib` Python package. This is an interface and
runtime migration, not `pip install` or a routine merge of a few recent features.
A bulk upstream merge would remove custom files absent upstream, including
Diplomacy. Preserve those files and the independent test interface deliberately.

Sources: [pinned upstream](https://github.com/PufferAI/PufferLib/tree/6ffa5b10dbbbe4d1e8288367c7d9d3acd3bad4a2),
[release](https://github.com/PufferAI/PufferLib/releases/tag/5.0-experiments),
[official documentation](https://puffer.ai/docs.html).

## Feature inventory

Paths below refer to the pinned upstream checkout unless marked as fork paths.
Implemented means visible in code; it does not mean tested on our simulator.

| Feature | Evidence and value | Adoption constraint |
| --- | --- | --- |
| Native trainer and compiled-in environment | `src/pufferl.cu`, `build.sh`; removes Python from the training loop | Build a new native adapter; keep the existing test-facing engine interface |
| CPU and CUDA environments | `src/pufferenv.h`, `src/ocean.cu`, `ocean/minimal/minimal.h` and `.cu` | Use a CPU Diplomacy engine first; GPU environments currently reject multi-policy/self-play |
| Static memory allocation | `Allocator` in `src/pufferl.cu`; contiguous weights, gradients and activations | Shapes must be known at initialization; custom policies need explicit buffers |
| CUDA graphs | Rollout graphs and `train_impl` in `src/pufferl.cu` | Test graphs on/off before performance claims; fixed shapes and resource lifetimes matter |
| Buffered CPU vectorization | `env_setup`, `vec_thread_main`; pinned host buffers and separate CUDA streams | Seven players must fit exactly into each buffer; policy grouping must not swap player identities |
| One-epoch actor/learner overlap | `base.async`, rollout slots and actor snapshots | Start synchronous for debugging; compare learning/numerics before enabling async |
| PPO variant | `train_epoch_gpu`, fused loss in `src/algo.cu`; clipping, entropy, value loss, optional V-trace | Old hyperparameters and update schedule are not equivalent; retune only after correctness |
| Reward clipping | `train_epoch_gpu` clamps every training reward to [-1, 1] | Raw welfare rewards above one lose magnitude; explicitly decide scaling while logging unscaled welfare |
| Muon optimizer and mixed precision | `src/algo.cu` Muon, FP32 master weights with BF16 parameters | Old Adam settings do not transfer; use `--float` for numerical checks |
| MinGRU/highway recurrent network | `mingru_gate`, `mingru_scan_forward` in `src/algo.cu` | The pinned train scan loops over time: do not assume the advertised time-parallel implementation is active |
| Hidden-state carry | `reset_every_horizon=0`, rollout initial states and terminal resets | Useful within games; not an individual-keyed memory across returning-partner encounters |
| Custom encoders/decoders/networks | Function tables in `src/algo.cu`; environment `*_net.h` examples | Existing PyTorch Diplomacy policy cannot be imported unchanged; backward buffers are manual |
| Multi-head discrete actions | `NUM_ATNS`, `ACT_SIZES`, per-head masked sampling/loss | Supports factorized heads, not automatic conditional joint orders or adjudication legality |
| Legal action masks | `Agent.action_mask`, stored rollout masks, masked sampling and PPO loss | Diplomacy must generate masks; all-masked heads and conditional orders need explicit tests |
| Continuous actions | Continuous decoder and Gaussian loss in `src/algo.cu` | Not needed for Diplomacy; masks are ignored for continuous actions |
| Frozen policies and self-play pool | `run_train`, checkpoint pool/resampling in `src/pufferl.cu`, `[selfplay]` config | CPU environments only; environment defines policy assignment and episode boundaries |
| Match evaluation | `eval_make`, `eval_loop`; policy-0 score/draw metrics | Generic two-policy evaluation is not a seven-player welfare tournament or partner-memory benchmark |
| Scripted-bot ladder | `puf_set_bot_policy`, `[bot_eval]`, final ladder evaluation | Existing evaluator changes agent/bot counts to one each; cannot assume it supports seven-player Diplomacy |
| Multi-GPU training | NCCL gradient averaging and `launch_train` process fork | Local GPU processes, not a proven multi-node scheduler; CUDA must not initialize before fork |
| Parallel sweeps | `run_sweep`; disjoint GPU blocks, failed workers marked bad | Needs explicit GPU/time budget; unsuccessful trials must remain visible |
| Protein tuning | `src/protein.cu`; Gaussian-process/Pareto cost-aware suggestions, pruning | Tune welfare/game metrics deliberately; reward tuning must not redefine the benchmark objective |
| Local experiment visualization | `src/constellation.c`; cached log data, score/time plots and hyperparameter coloring | Helpful after reproducible small experiments exist; not an orchestration system |
| Standalone CPU evaluation/rendering | `src/puffercpu.c`, `build.sh --cpu` | No native CPU training; preserve CPU engine tests and use an NVIDIA machine for training |
| Web rendering/builds | `build.sh --web`, Emscripten branches | Optional; not needed for the first migration loop |
| Debugging/profiling | `--debug`, `--profile`, timing/dashboard/NVTX in source | Check sanitizer coverage by platform; throughput claims require local measurements |
| Checkpoints | `puf_save_weights`, `puf_load_weights_into`: raw flat FP32 `.bin` | Weights only, no optimizer/RNG/episode state or self-describing schema; not exact training resume |
| Logs and configuration | `[section]` configuration, `--section.key=value`, per-run INI logs | Record upstream SHA, environment version, seeds and metrics alongside results; do not reuse stale fork keys |
| Ocean examples and pretrained models | `ocean/*`, release assets and config examples | Useful interface references, not Diplomacy baselines; no need to import all environments/assets |

## Compatibility decisions before implementation

1. Keep the current C engine and test harness as the reference behavior while
   building a small native adapter. Do not rewrite adjudication to match a new
   framework.
2. Define the seven-player buffer layout and a complete simultaneous-order
   contract. Our current single-order action interface is already incomplete;
   migrating it mechanically would preserve a misleading game.
3. Test terminal reward/reset ordering and player identity before training.
   Separate raw welfare accounting from the trainer's scaled/clipped reward.
4. Keep Python only where it earns its place: tests and research orchestration.
   Document whether an API remains available for external/LLM agents. Upstream
   no longer supplies the old Gym/PettingZoo/Python integration layer.
5. Retain CPU checks on this Mac. Native training requires NVIDIA CUDA hardware;
   no training speedup has been measured here.
6. Version action/observation schemas and checkpoints. Raw weights cannot safely
   stand in for a reproducible experiment or a full training-resume snapshot.

## Build, tests and reproducibility risks

- `build.sh` downloads Raylib and some environment-specific dependencies when
  absent. The URLs are not a checksummed dependency lock. Review/pin only the
  dependencies needed by Diplomacy rather than running the general installer.
- Native training links CUDA, cuBLAS, cuRAND, cuSOLVER, NCCL and NVML. The build
  also assumes tools such as clang and ccache. Check the chosen NVIDIA machine
  against the actual script, not just the documentation's installation example.
- On Linux, CPU debug builds enable sanitizers. On this Mac, the script sets
  `SANITIZE_FLAGS=()`. The `--debug` flag alone therefore does not establish
  memory-safety testing here. Add an explicit tested sanitizer job on Linux.
- `build.sh all` prints failed builds but exits zero. Do not use its exit code
  as a CI success signal without fixing or wrapping that behavior transparently.
- There is a duplicate, unreachable CPU branch referencing missing
  `src/puffercpu.h`. The earlier CPU branch compiles `src/puffercpu.c` and exits;
  the dead reference is not evidence that the active CPU build is broken.
- Upstream has no checked-in GitHub Actions workflow at this pin. Several Python
  tests reference removed modules; the presence of tests is not a working suite.
  Native CUDA numerical tests exist, but still need a supported GPU and explicit
  selection. Preserve our Diplomacy tests instead of inheriting everything blindly.
- `close_pufferl` deliberately leaves allocations to process exit. Training and
  repeated evaluation in one long-running process need resource-lifetime checks;
  do not assume the old Python library lifecycle remains supported.
- `env_setup` initially seeds CPU environments by index (`rng = num_envs`).
  `base.seed` seeds other trainer state. Establish what an experiment seed controls
  with reproducibility tests; a single config field is not proof of game replay.
- Root upstream is MIT. Copied/vendor code and model/asset licenses still need
  inventory. Our copied Diplomacy test notices already need an owner decision;
  an upstream upgrade cannot resolve that provenance automatically.
- Logs save configuration and learning history; `.bin` checkpoints contain only
  weights. Record revision/schema/seed/config/metric definitions and environment
  traces separately. Exact resume requires more state than upstream saves.

## Checks actually run

- Cloned upstream, checked HEAD and branch against the remote, inspected source,
  and computed the common ancestor/divergence counts.
- On this Darwin arm64 machine, `nvcc` is absent. No CUDA build, GPU training,
  multi-GPU, speed, or numerical parity check has been run.
- Ran `python -m pytest tests/test_selfplay.py --collect-only -q` from upstream
  using our isolated Python 3.9.6 test environment. It failed collection:
  `ModuleNotFoundError: No module named 'pufferlib'`, one error, no tests collected.
  The checked-in test imports a Python package absent from the target tree.
  Installing our old fork to make this pass would test the wrong implementation.
- The merged fork setup's CI passed 210 tests with 46 skips. That is evidence
  about the old engine, not proof of compatibility with 5.0.

## Proposed migration stack

Each item is a small PR with its own issue, worktree, tests and independent review.
Dependent PRs can be developed on top of an unmerged parent. No whole-stack merge
without approval for every included PR.

1. Finish/review this audit and pin the upstream revision and dependency policy.
2. Make the existing CI run for dependent PR bases as well as `welfare`; extend
   the worktree helper to accept a parent branch. Keep all current tests.
3. Add a side-by-side native CPU Diplomacy adapter/build smoke test. Preserve the
   original test API and demonstrate equal seeded state/reward/order traces.
4. Fix reward/ownership/reset defects in separate reproduced issues before using
   the simulator as a learning benchmark. Resolve rules conflicts and copied-test
   license provenance with the owner.
5. Implement full orders and legal masks, updating engine, adapter, observations,
   policy and tests together. This needs an approved interface design.
6. On an approved NVIDIA machine, run tiny FP32 synchronous training, checkpoint
   load/evaluation and recurrent-state tests. Then check BF16, CUDA graphs and async.
7. Add scripted/cooperative/selfish baselines and seven-player evaluation, then
   historical self-play, tuning and measured throughput. Multi-GPU and web builds
   are optional follow-ups, not prerequisites for correctness.

## Workflow state

GitHub CLI 2.102.0 and official `github/gh-stack` 0.2.1 are installed; stack help
works. The owner authenticated the CLI. Active ruleset `24849342` was created
and read back: `welfare` requires a PR, resolved review conversations and the
`diplomacy-tests` check from GitHub Actions, tested against the latest branch.
Force pushes/deletion are blocked and there are no bypass actors. Required
GitHub approval count is zero because the human and author use one account;
human approval remains mandatory in shared instructions. No destructive push
was attempted to test enforcement. Issue #4 is closed; the audit is issue #3.

Two audit collaborator sessions reported account usage errors. Their audits have
not been completed; a user-reported reset prompted one retry. This draft must not
be described as independently reviewed while those checks remain unfinished.

The 11:47 PM UK follow-up is active for this chat on 10 October 2026. Its prompt
requests another follow-up 5 hours 30 minutes later if work remains. Scheduling
does not override actual account limits or guarantee unattended completion.
