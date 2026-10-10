# PufferLib 5.0 environment-interface audit for PufferWelfare

Audit date: 2026-10-10  
Upstream checkout: `work/PufferLib-upstream` at commit `6ffa5b10dbbbe4d1e8288367c7d9d3acd3bad4a2` (`5.0`)  
PufferWelfare checkout: `PufferWelfare` at commit `f398421f3a233352de04684ad2ff56481078e45c` (`welfare`)

This is a read-only compatibility audit. It does not migrate code, change either checkout, or claim behavior that is not visible in the pinned sources. Line numbers below refer to those exact commits.

## Bottom line

PufferLib 5.0 can host a seven-agent Welfare Diplomacy simulation as a **CPU environment feeding a CUDA PPO trainer**, but the current PufferWelfare environment is not a drop-in environment for it. The engine can be preserved, but the wrapper and action protocol need a deliberate redesign.

The largest blockers are semantic rather than syntactic:

1. The current environment gives each power one integer action and then silently holds all of that power's other units. It therefore cannot express a normal simultaneous Diplomacy order set (`PufferWelfare/pufferlib/ocean/diplomacy/diplomacy.c:4287-4460`).
2. PufferLib 5.0 masks each discrete action head independently. It has no native autoregressive or cross-head constraint mechanism. A naive “one head per unit/order field” encoding cannot enforce all complete-order-set constraints.
3. The trainer clamps every reward to `[-1, 1]` before optimization (`PufferLib-upstream/src/pufferl.cu:1470-1489`). Raw yearly welfare deltas larger than one would train as one unless the project explicitly scales rewards or changes the trainer.
4. The default vector sizes are invalid for fixed seven-player games. The core divides `total_agents` across buffers without a clear whole-game preflight check; its standard assert-enabled build aborts later on a generic buffer-count assertion, after creating an overshooting logical environment count but before binding rows (`PufferLib-upstream/src/pufferl.cu:986-1064`, `1835-1842`).
5. PufferLib has no persistent individual identity or returning-partner memory abstraction. `Agent.policy` selects a policy bank; it is not an individual ID (`PufferLib-upstream/src/pufferenv.h:24-31`). Recurrent state is keyed to a physical agent row and is cleared on terminal.
6. Native historical-policy self-play is CPU-environment-only, swaps policy weights on a global-step timeout, and does not actually perform the truncate/reset promised by its comment (`PufferLib-upstream/src/pufferl.cu:1847-1850`, `3124-3134`). That mechanism is not a returning-partner scheduler.
7. The current fork appears not to emit welfare rewards at all: it checks for `PHASE_WINTER_ADJUSTMENT` only **after** advancing out of the just-resolved phase. Entering Winter is checked before adjustment/scoring, and completing adjustment advances to Spring/Completed before the reward branch (`PufferWelfare/pufferlib/ocean/diplomacy/diplomacy.c:4120-4167`, `4508-4552`). The skipped welfare reward tests do not catch this (`PufferWelfare/tests/diplomacy/c_tests/test_c_welfare.py:34-59`).

The best first integration is therefore not a broad port. It is a thin PufferLib 5 adapter behind a tested, canonical **sequential atomic-order submission protocol**, with the adjudicator still resolving all powers' completed order sets simultaneously.

## What PufferLib 5.0 actually exposes

### Environment ABI

The shared agent structure contains only:

- a flat observation pointer;
- a float action vector;
- one reward and one terminal value;
- a byte action-mask vector; and
- an integer policy-bank selector.

The environment supplies `puf_init`, `puf_reset`, `puf_step`, `puf_render`, `puf_close`, and `puf_log` (`PufferLib-upstream/src/pufferenv.h:21-50`). There is no Gym/PettingZoo space object, info dictionary, truncation channel, agent ID, or variable-length entity API in this native interface.

The environment header defines fixed compile-time `OBS_SIZE`, `NUM_ATNS`, and `ACT_SIZES`. One environment is compiled into the trainer via `ENV_HEADER` (`PufferLib-upstream/build.sh:448-465`; `PufferLib-upstream/src/algo.cu:1269-1287`). The pinned checkout's root has no `pufferlib/` Python package. Files such as `tests/test_api.py`, which import an older Python package, should not be treated as the 5.0 native contract unless upstream restores that package; the C/CUDA sources are the reliable evidence here.

The core also directly accesses environment-owned fields including `log`, `agents`, `num_agents`, `rng`, `tag`, and `boundary_reached` (`PufferLib-upstream/src/pufferl.cu:1000-1006`, `1045-1086`, `921-965`). A new Diplomacy `Env` must provide those fields in addition to its game state.

`puf_init` runs before the core binds agent buffers. It should set configuration, `num_agents`, and each slot's `policy`, but must not write observations or masks yet. The core binds pointers at `src/pufferl.cu:1077-1084` and calls `puf_reset` later through `env_start` (`src/pufferl.cu:1204-1237`).

### Build and execution model

For native training, a CPU `.h` environment is still compiled into `src/pufferl.cu` with NVCC and linked to CUDA, NCCL, cuBLAS, cuSOLVER, and cuRAND (`PufferLib-upstream/build.sh:505-527`). “CPU environment” means the simulation runs on the host; it does **not** mean the native trainer is CPU-only.

There is a separate `--cpu` standalone inference/evaluation build using `src/puffercpu.c` (`PufferLib-upstream/build.sh:279-313`). That is useful for engine smoke tests and rendering, but it is not the native PPO training path.

The build automatically defines `PUFFER_<ENV>` (`PufferLib-upstream/build.sh:201-208`), but extra source files are not generically discovered. `EXTRA_SRC` is initialized empty and populated only for hard-coded cases (`build.sh:131`, `180`). A Diplomacy port must either:

- expose a C/C++-compatible amalgamated header included by `ocean/diplomacy/diplomacy.h`; or
- add a narrow Diplomacy build case listing the existing engine sources.

That integration choice should preserve the pure adjudication engine as independently testable code. It should not fold game rules into the PPO wrapper.

On Linux, the debug/local C build enables address, undefined-behavior, bounds, and pointer-overflow sanitizers; on macOS the sanitizer flag list is empty (`PufferLib-upstream/build.sh:81-107`, `233-240`). A macOS compile alone is not equivalent to the intended Linux sanitizer gate.

## Compatibility matrix

| Concern | PufferLib 5.0 at the pinned commit | PufferWelfare today | Required decision/work |
|---|---|---|---|
| Agents | Fixed flat rows; env sets `num_agents` and policy bank | Seven powers, global buffers in one `Env` | Replace global buffers with `Agent agents[7]` and native lifecycle |
| Observation | Fixed flat `OBS_SIZE`; `obs_t` is byte or float | Float vector of 1,641 values | May be carried initially, but validate its known map/bounds problems before calling it stable |
| Action | Fixed continuous or independent multidiscrete heads | One `Discrete(1088)` action per power | Define a complete-order submission protocol before porting |
| Legal actions | Concatenated byte mask, one slice per head | No environment mask | Add exact legal masks and all-head-nonempty assertions |
| Step | All agent actions dispatched, one `puf_step` per environment | One order decoded per power, remaining units held, then phase resolves | Collect complete order sets across microsteps; resolve simultaneously only when complete |
| Reward | Per-agent float; trainer later clamps to `[-1,1]` | Intended welfare delta, but current timing misses scored adjustment | Fix reward timing; decide explicit train scaling versus trainer change |
| Done/reset | One float terminal; examples auto-reset inside terminal step | Terminal is byte; game remains over until explicit reset | Adapt type and auto-reset while preserving final reward/terminal |
| Truncation | No separate channel | Wrapper has truncations, but binding ignores their storage | Represent fixed-year end as terminal; document any other time-limit semantics |
| Memory | MinGRU per physical row; reset on terminal | No returning-partner representation | Add stable IDs and explicit persistent memory/history design |
| Self-play | Policy 0 trainable; other banks frozen; CPU env only | None integrated | Use only for simple current-vs-checkpoint baselines, not partner identity |
| Logs | Flat all-float sums divided by episode count | Four float fields, accumulated per episode | Keep aggregate sums; use external records for per-identity trajectories |
| Seed | Policy RNG uses base seed; generic CPU env RNG starts at env index | Python seed/global `srand` path | Add benchmark seed to env configuration and deterministic vector derivation |

## Observations and individual identity

PufferLib 5.0 allocates observations as `[total_agents, OBS_SIZE]` with a compile-time scalar type (`PufferLib-upstream/src/pufferl.cu:1874-1893`). The default architecture is a flat linear encoder, MinGRU stack, and linear decoder (`PufferLib-upstream/src/algo.cu:911-917`, `984-1024`; `PufferLib-upstream/src/puffercpu.c:530-572`). This is enough for an initial flat baseline. There is no native dictionary, ragged entity table, message stream, or bounds metadata.

Custom encoders and decoders are possible, but only as compile-time C/CUDA hooks. `src/ocean.cu` explicitly lists supported environment macros and otherwise leaves the default encoder/decoder unchanged (`PufferLib-upstream/src/ocean.cu:20-92`). At this pin there is no corresponding custom-network hook: `build_arch` always installs MinGRU as the recurrent network (`PufferLib-upstream/src/algo.cu:1012-1024`). A Diplomacy-specific structured encoder would require adding a `PUFFER_DIPLOMACY` hook and maintaining matching standalone-CPU weight loading. Replacing MinGRU would be a deeper trainer modification. Both should come after the flat interface is correct.

The current fork's observation contract is described as 81 locations × 20 features plus 21 globals (`PufferWelfare/pufferlib/ocean/diplomacy/diplomacy.py:43-48`; `diplomacy.h:16-20`). Every power currently sees the same board (`diplomacy.c:4211-4217`). Nothing encodes an individual player distinct from a power/seat, a mapping from individuals to powers, an absence/return, or what information about that individual remains observable.

Although the model is called MinGRU, the pinned training implementation does not demonstrate a time-parallel recurrent scan. Each CUDA thread advances sequentially across the horizon in `for (t = 0; t < T_seq; t++)`, and the long-sequence tiled alternative still applies each time tile sequentially in one lane (`PufferLib-upstream/src/algo.cu:241-303`, `399-455`, `831-852`). This is not an interface blocker, but it matters for performance expectations once sequential order entry lengthens Diplomacy episodes. Benchmark the actual compiled path rather than relying on the architecture name or marketing description.

For a returning-partner benchmark, distinguish at least four concepts:

- **power/seat**: Austria, England, and so on;
- **individual ID**: the persistent entity whose behavior may change and later return;
- **policy bank/checkpoint**: an implementation detail selecting weights;
- **physical row**: the vector slot whose recurrent state PufferLib stores.

PufferLib supplies only the last two, and they are not interchangeable with individual identity. `Agent.policy` is read once during setup to group rows by policy (`PufferLib-upstream/src/pufferl.cu:1035-1089`). Changing it after setup would not move the bound row or change the policy layout. A benchmark must own individual IDs, the individual-to-seat schedule, observable identity features, and any memory keyed by those IDs.

The default MinGRU state persists across rollout horizons because `reset_every_horizon=0`, but terminal rows are zeroed before their next observation is encoded (`PufferLib-upstream/config/default.ini:18-28`; `PufferLib-upstream/src/pufferl.cu:850-859`). The standalone CPU network does the same (`PufferLib-upstream/src/puffercpu.c:516-527`, `584-594`). Consequently:

- if each Diplomacy game terminates, recurrent memory alone cannot remember a partner across games;
- if identities rotate through a physical row without a terminal, state can leak from one person to another; and
- if the same individual later occupies another row, row-local state does not follow them.

Do not work around this merely by suppressing terminal. That would change PPO episode and credit-assignment semantics. The minimal honest baseline is to put a bounded, explicitly specified interaction history in the observation or add a separate identity-keyed memory component. A later recurrent-memory research baseline can then be compared against it.

Private press/messaging remains out of scope: the local rule contract explicitly says it is not implemented (`PufferWelfare/docs/rules/welfare-diplomacy.md:72-74`). PufferLib's flat observation interface does not add it implicitly.

## Full orders and legal masks

### Why the current action cannot migrate as-is

The current action is `unit_index * 64 + order_type` (`PufferWelfare/pufferlib/ocean/diplomacy/diplomacy.py:50-52`; `diplomacy.c:4287-4296`). One such action is read for each power. The decoder supports hold, a limited adjacent move, support-hold, voluntary disband, and a first available build location. Convoys are converted to hold, support-move is not represented, and retreat orders are not represented (`diplomacy.c:4319-4397`). The wrapper then adds hold orders for every remaining unit and resolves the phase (`diplomacy.c:4423-4460`, `4508-4536`). This is not a complete Diplomacy action space.

### What masks can and cannot express

PufferLib 5 defines one categorical size for each of `NUM_ATNS` heads. The mask is a concatenation of those head masks, with total width `sum(ACT_SIZES)` (`PufferLib-upstream/src/pufferl.cu:1852-1893`). Sampling loops over heads independently and sums their log probabilities (`src/pufferl.cu:589-703`; `src/algo.cu:1426-1452`).

This supports exact legality for independent categorical choices. It does not make head 2's mask depend on the action just sampled from head 1. Therefore a factorization such as separate order-type, origin, supported-origin, and destination heads can sample combinations that no legal order represents. Multiple whole-order heads can also violate set-level constraints, such as submitting two orders for one unit or mixing build and disband types in one adjustment set, unless the slot design removes those dependencies.

Masked logits are replaced with `-1e4` (`PufferLib-upstream/src/algo.cu:1342-1373`). There is no assertion that every head has a legal action. If a CUDA head is all zero, all logits become equal and sampling can still return an illegal category; the final “last legal action” scan finds nothing (`PufferLib-upstream/src/pufferl.cu:637-697`). Standalone CPU inference instead leaves that head at action zero (`PufferLib-upstream/src/puffercpu.c:179-250`). An all-zero mask is thus both invalid and backend-divergent.

### Recommended pilot protocol

Use one categorical head over a fixed canonical vocabulary of **complete atomic orders**, and submit order sets over several environment microsteps:

1. At the start of a game phase, freeze the public board state used to determine legal orders.
2. Give every power a deterministic cursor over its units, dislodged units, or adjustment slots.
3. On each microstep, mask the single categorical head to the complete atomic orders legal for that cursor. A support-move token includes both the supported origin and destination; a convoy token includes the convoyed origin and destination. Include explicit hold, waive, disband, and no-op tokens where the phase contract permits them.
4. Store each power's submissions privately. Do not expose other powers' pending orders.
5. Give inactive/completed powers exactly one legal no-op action.
6. Once all seven powers have completed the phase's order set, call the existing simultaneous adjudicator once, emit any reward, and advance the game phase.

This is a recommendation, not an upstream feature. It makes each mask a true set of atomic choices and avoids cross-head conditional legality in the first baseline. It also changes one game phase into several RL timesteps, so horizon, discounting, recurrent state, action logging, and “one step” metrics must be documented accordingly. Reward should normally remain zero on collection microsteps and be emitted on adjudication/scoring.

A single categorical over every complete order **set** would preserve one-step phases but is likely too large and dynamic for the fixed compile-time interface. A custom autoregressive decoder is possible only through new C/CUDA policy code and is not appropriate for the first integration.

Required mask invariants:

- every head for every agent has at least one legal category;
- every unmasked token round-trips to a well-formed atomic order;
- every well-formed legal atomic order has exactly one token;
- inactive agents have only no-op legal;
- masks are refreshed after reset and every microstep;
- actions sampled under both CUDA and standalone CPU paths are legal; and
- the same state, cursor, and seed produce the same mask ordering.

## Multi-agent vectorization and seven-player alignment

For CPU environments, PufferLib repeatedly calls `puf_init` until the sum of `env.num_agents` reaches `vec.total_agents` (`PufferLib-upstream/src/pufferl.cu:986-1007`). It allocates observation/action/reward/terminal/mask buffers for exactly `total_agents`, then binds every environment agent into those buffers (`src/pufferl.cu:1023-1084`).

There is no early assertion that the loop ended at exactly `total_agents`. For seven-agent environments, the default `total_agents=4096` creates 586 logical games, or 4102 agent slots. Similarly, `agents_per_buf = total_agents / num_buffers` is integer division (`src/pufferl.cu:1835-1842`), and environment partitions advance when their accumulated agent count is **at least** that value (`src/pufferl.cu:1009-1018`). In the repository's normal build, assertions remain enabled and the later `assert(offset == apb)` aborts before row binding (`src/pufferl.cu:1053-1064`; `build.sh:238-240`). That protects the standard binary from the immediate out-of-bounds bind, but it is a generic late failure rather than a usable configuration contract; an assertion-disabled derivative would lose that protection.

Until upstream adds validation, require these configuration invariants before allocation:

```text
total_agents % num_buffers == 0
(total_agents / num_buffers) % 7 == 0
```

Equivalently, every buffer must contain an integer number of whole seven-player games. The default 4096/2 does not qualify. CI should intentionally run a bad configuration and verify a clear failure instead of memory corruption.

Each buffer has one worker thread loop; environments inside it step in an OpenMP loop (`PufferLib-upstream/src/pufferl.cu:1121-1201`). A single `puf_step` processes one complete seven-player game, which is appropriate for simultaneous state transition after order collection.

Rows are grouped by policy bank within each buffer, not by environment. The per-environment `Agent` pointers retain the correct mapping (`PufferLib-upstream/src/pufferl.cu:1035-1089`), but code outside those pointers must not assume that the seven powers occupy adjacent physical rows. Every buffer must also produce exactly the same per-policy row counts (`src/pufferl.cu:1053-1059`). Custom identity or partner schedules must not silently change those counts.

## Rewards, terminal steps, and reset

### Native transition convention

The CPU worker zeros reward and terminal buffers, runs each `puf_step`, and uploads the resulting next observation/reward/terminal (`PufferLib-upstream/src/pufferl.cu:1180-1193`). The standalone CPU loop does not do that zeroing before `puf_step` (`PufferLib-upstream/src/puffercpu.c:899-939`), so the environment should still clear nonterminal rewards/terminals itself for portable behavior.

Upstream examples auto-reset on the terminal transition, while leaving terminal and final reward set. The template calls `puf_reset`, then writes the terminal reward and terminal flag; its final observation is the next episode's initial observation (`PufferLib-upstream/ocean/template/template.h:33-60`). Soccer follows the same contract (`PufferLib-upstream/ocean/soccer/soccer.h:400-414`).

The rollout copies the current observations, incoming reward, terminal, and mask before sampling the next action (`PufferLib-upstream/src/pufferl.cu:784-877`). GAE explicitly uses the next reward/terminal convention (`PufferLib-upstream/src/algo.cu:1696-1739`). A Diplomacy wrapper must match this exactly:

- on the final Winter action, finish scoring and logging;
- reset the game into its next initial state;
- expose that reset observation;
- keep the ended game's final reward and `terminal=1` for all seven rows; and
- on the following ordinary step, clear terminal and reward.

The current fork instead returns immediately forever once `is_game_over` is set (`PufferWelfare/pufferlib/ocean/diplomacy/diplomacy.c:4508-4511`). That behavior would leave native vector workers stuck on a completed game. Its terminal storage is also `unsigned char*`, whereas PufferLib 5 expects `float*` (`PufferWelfare/pufferlib/ocean/diplomacy/diplomacy.h:238-243`; `PufferLib-upstream/src/pufferenv.h:24-31`).

PufferLib 5 has no separate truncation field. The current old-style binding accepts a truncation array but deliberately does not save its pointer (`PufferWelfare/pufferlib/ocean/env_binding.h:122-136`). The fixed-year Welfare Diplomacy end should be a true terminal. Any operational timeout needs an explicit project decision rather than a fabricated truncation convention.

### Welfare reward bug and clipping

The rule contract says welfare is added after each Winter and accumulates (`PufferWelfare/docs/rules/welfare-diplomacy.md:28-41`). In the current engine, `resolve_adjustment_phase` adds positive `centers - units` at `diplomacy.c:4120-4127`. But `c_step` advances the phase first and then emits a delta only if the **new** phase is Winter (`diplomacy.c:4535-4552`). Therefore:

- after Fall, the new phase is Winter but adjustment has not scored yet, so the delta is normally zero;
- after Winter resolves and scores, the new phase is Spring or Completed, so the branch clears reward; and
- the final accumulated welfare is terminally logged but is not emitted as the intended delta reward.

This must be fixed before migration and covered by an exact phase-by-phase reward trace.

Even after that correction, PufferLib clamps training rewards after transposition (`PufferLib-upstream/src/pufferl.cu:1487-1489`). This is unconditional and has no configuration switch at the pinned commit. The project must choose and document one of:

- scale each welfare delta into `[-1,1]` in the environment while logging raw welfare separately; or
- patch/pin the trainer to make reward clipping configurable or disabled for Diplomacy.

Silently accepting the clamp changes the optimization objective by making welfare improvements of 2 and 10 identical for PPO. Raw episode logs are not affected by that kernel, so logged welfare and trained reward could otherwise disagree.

## Logging

`Log` is treated as a flat array of floats (`PufferLib-upstream/src/pufferl.cu:921-938`). Only an environment log with nonzero `log.n` is accumulated. Across environments, every float field is summed, and then every field is divided by aggregate `n` before `puf_log` is called; the original count is exported separately as `env/n` (`src/pufferl.cu:954-965`, `1363-1383`).

The safe contract is therefore:

- `log.n` counts completed games;
- every other `Log` field is a sum across completed games that is meaningful when divided by game count; and
- `puf_log` exports the resulting averages.

The fork's `avg_welfare`, `avg_centers`, and `avg_units` fields are in fact accumulated episode sums despite their names (`PufferWelfare/pufferlib/ocean/diplomacy/diplomacy.c:4560-4574`), so their aggregation shape is compatible.

Do not put IDs, maxima/minima, histograms, strings, integer counters intended as totals, or per-partner traces into this `Log` without custom aggregation. A returning-partner benchmark needs a separate structured episode record containing scenario ID, episode/game ID, individual IDs, seat mapping, behavior-change condition, raw per-power rewards, and outcome. The native dashboard log can retain compact means such as social welfare, per-seat welfare, minimum welfare, inequality/exploitation metrics, game length, and invalid-action count.

Evaluation waits until `env/n` reaches the requested game count (`PufferLib-upstream/src/pufferl.cu:2845-2905`). If no game completes during a log interval, `env/*` values are omitted and the last complete-episode snapshot is kept (`src/pufferl.cu:3187-3200`). Long Diplomacy episodes therefore need a sufficiently small evaluation game count and visible game-length/progress diagnostics.

## Policy hooks, historical opponents, and what self-play does not provide

PufferLib creates policy 0 as trainable and every higher policy index as frozen. All policy banks share the observation/action contract but may use configured historical hidden size/layer count (`PufferLib-upstream/src/pufferl.cu:1931-1960`). In CPU environments, `Agent.policy` determines which frozen bank controls a row in the historical fraction of environments (`src/pufferl.cu:1035-1089`). GPU environments explicitly reject multi-policy and self-play (`src/pufferl.cu:1847-1850`).

The native self-play pool stores checkpoint paths, randomly samples them, and loads them into fixed historical bank indices (`PufferLib-upstream/src/pufferl.cu:2512-2544`, `3030-3054`). The bank index remains the environment `tag`; the framework does not expose which checkpoint/individual is currently loaded as an observation feature.

There is another unverified objective choice: although frozen policies have no
optimizer or gradients, `train_epoch_gpu` processes all `total_agents` rows with
policy 0 and no visible filter for its row layout (`src/pufferl.cu:1460-1566`).
Frozen-policy trajectories therefore appear to update policy 0. This could be
intentional reuse of symmetric self-play experience, not necessarily a bug, but
its suitability for general-sum Welfare Diplomacy is unproven. Begin with
`num_policies=1`. Before historical self-play, use a controlled test with zero
primary rewards and nonzero frozen-policy rewards to identify which trajectories
change learner weights, and make an explicit objective decision.

More importantly, the timeout path says “treat as truncate + reset” but only calls `pufferl_load_policy`; it does not call `env_restart`, set terminal, clear recurrent state, or wait for game boundaries (`PufferLib-upstream/src/pufferl.cu:3124-3134`). Several environments set `boundary_reached`, and setup initializes it (`PufferLib-upstream/ocean/soccer/soccer.h:400-414`; `src/pufferl.cu:1085-1086`), but a repository-wide search at this commit finds no core read of `boundary_reached`. Tests referencing Python `count_aligned` target a package absent from this checkout and do not demonstrate native C/CUDA boundary alignment.

Thus native self-play is usable only as a simple, carefully configured checkpoint-opponent baseline. It does not natively provide:

- stable individual identities;
- absence and return schedules;
- behavior-change interventions;
- identity-keyed histories;
- episode-boundary opponent swaps; or
- general seven-player population matchmaking.

The two-policy match evaluator also reports `env/policy_0_score` and assumes an environment-defined two-bank matchup (`PufferLib-upstream/src/pufferl.cu:2895-2901`, `2932-2943`). It can support a deliberately defined “one live policy versus one frozen policy assigned to selected seats” experiment, but it is not a seven-individual evaluator by itself.

The scripted bot ladder is even narrower: before evaluation it hard-codes `env.num_agents=1` and `env.num_bots=1`, resets to one policy bank, and varies one `bot_policy` integer (`PufferLib-upstream/src/pufferl.cu:3283-3337`). It cannot evaluate a seven-agent Welfare Diplomacy game without a separate Diplomacy-specific bot/evaluation design. Do not advertise the built-in ladder as applicable here.

## Seeding and reproducibility

The trainer derives its model/action RNG seed from `base.seed + rank` (`PufferLib-upstream/src/pufferl.cu:1810-1815`, `1833`). The generic CPU environment setup does not use that seed: it initializes each `Env.rng` to `num_envs`, i.e. its local environment index, before `puf_init` (`src/pufferl.cu:1000-1006`). `puf_reset` has no seed argument.

This can be deterministic on one fixed vector layout, but it is not a complete benchmark seed contract and can repeat streams across distributed ranks. A Diplomacy environment that needs stochastic seat/identity schedules should accept an explicit environment seed and combine it with rank and environment index through a documented vector initializer. `MY_VEC_INIT` exists as an extension point (`src/pufferl.cu:996-999`), but no generic implementation is supplied. Exact single-thread versus multithread and rerun determinism should be tested.

## Checkpoint and resume limits

`puf_save_weights` writes only policy 0's flat FP32 master-weight buffer to a temporary file and renames it into place (`PufferLib-upstream/src/pufferl.cu:1722-1736`). Loading reads exactly the number of floats expected by the currently compiled architecture and copies them into a policy (`src/pufferl.cu:1738-1761`). The file has no header containing an upstream revision, environment name, observation/action schema, tensor names/shapes, or reward contract.

It also contains no optimizer momentum, action RNG, recurrent row state, environment/game state, partner schedule, accumulated benchmark memory, or training/log cursor. The visible `load_model_path` path is used by evaluation construction (`src/pufferl.cu:2948-2964`); the audited `run_train` path does not establish a full-state resume operation. Treat `.bin` files as weight snapshots for compatible builds, not reproducible training checkpoints or individual memories.

Every experiment artifact should therefore record at least the upstream SHA, PufferWelfare SHA, observation/action schema version, exact policy architecture, raw/scaled reward definition, full configuration, seed derivation, checkpoint content hash, and structured evaluation schedule. A schema-changing migration must reject incompatible weight files before evaluation rather than relying only on byte count.

## Recommended migration shape

### Stage 0: make four research contracts explicit

For the initial welfare-only simulator, decide the first two contracts in a
human-reviewed issue. The last two apply only when a returning-partner benchmark
is in scope; they must not delay an ordinary simulator/baseline pilot:

1. the atomic order vocabulary and sequential submission protocol;
2. raw welfare reward timing and PPO reward scaling/clipping;
3. whether an episode is one game or a multi-game returning-partner sequence; and
4. how individual IDs, behavior changes, observable cues, and persistent memory are represented.

These are benchmark semantics, not framework plumbing.

### Stage 1: repair and freeze the pure engine boundary

- Keep standard adjudication and Welfare variant rules independent of PufferLib.
- Fix the current reward timing and add an exact final-Winter test.
- Expose deterministic legal atomic-order enumeration with a canonical stable ordering.
- Preserve the DATC/reference regression path described in `PufferWelfare/docs/rules/welfare-diplomacy.md:8-17`.
- Do not add press, inferred rules, or a new voluntary-disband interpretation in the migration.

### Stage 2: add the smallest PufferLib 5 CPU adapter

Create a 5.0 environment with:

- `typedef float obs_t` initially;
- fixed `OBS_SIZE`, `NUM_ATNS=1`, and a measured fixed atomic-order vocabulary size;
- `Log`, `Agent agents[7]`, `num_agents=7`, `rng`, `tag`, and `boundary_reached`;
- `puf_init` for config/policy slots only;
- `puf_reset` for game state, observation, and legal mask;
- `puf_step` for one order-submission microstep or one adjudication transition;
- terminal auto-reset semantics matching the upstream template; and
- `puf_close` that owns and frees engine allocations exactly once.

Use the default flat policy first. Defer a custom encoder until the environment passes semantic tests.

### Stage 3: add vector and native smoke tests

Use whole-game vector sizes and prove that multiple OpenMP environments do not share state. Run the standalone CPU binary for simple evaluation, then the native CUDA trainer for the actual rollout/mask/reward path. Pin the upstream commit in CI or in the vendored subtree because the 5.0 interface is compile-time and still internally inconsistent in areas such as boundary-aligned self-play.

### Stage 4: add benchmark orchestration outside native self-play

Implement the roster, identity schedule, change intervention, and structured episode records in a small external orchestrator or explicit environment-owned meta-episode layer. Native historical policy banks may supply behaviors, but they should not be used as the identity model. Require a separate reviewer to verify identity isolation and episode boundaries before using results as a benchmark.

## Required invariant and regression tests

### Adapter/ABI

- Compile a minimal Diplomacy header against `src/pufferenv.h` and the pinned native trainer.
- Verify `num_agents == 7`, all seven buffer pointers are non-null after binding, and `puf_init` did not dereference them early.
- Verify terminal storage is float and there is no assumed truncation buffer.
- Verify `puf_close` under Linux ASan/UBSan; also run pure-engine sanitizers independently on macOS.

### Full-order action protocol

- For movement, retreat, and adjustment fixtures, compare every unmasked token against `get_possible_orders`/validation and require a one-to-one canonical mapping.
- Submit more than one non-hold order for a power and prove all survive until simultaneous adjudication.
- Cover move, hold, support-hold, support-move, convoy, retreat, disband, build army, build fleet on valid coast, and waive where rules allow.
- Prove other powers' partially submitted orders are not observable.
- Prove no phase is adjudicated until every power has finished or has only no-op remaining.
- Prove every agent/head always has at least one legal action and sampled actions never select a masked token on CUDA or CPU inference.

### Seven-player vector safety

- Reject `total_agents=4096, num_buffers=2` clearly for a seven-agent environment.
- Accept a size divisible into whole seven-agent games in every buffer.
- Test two or more buffers with identical policy layouts and verify no row or mask from one game aliases another.
- Test an intentionally unequal policy count per buffer and require a preflight error rather than the current late assertion.

### Reward and episode semantics

- Trace one complete short game and assert zero reward on order-collection microsteps.
- Construct a Winter with `centers - units = 2`; assert raw reward 2 is emitted exactly once after adjustment and cumulative welfare increases by 2.
- Repeat for the final Winter; assert final reward is present on the same transition as terminal.
- Assert the observation accompanying terminal is the next reset game's initial observation.
- Assert the following step has terminal/reward cleared.
- In a native PPO smoke test, explicitly assert the chosen scaled reward, or demonstrate configurable clipping; never infer training reward from raw log output.
- Assert all seven terminals fire together for a normal fixed-year game.

### Identity and memory

- Use an individual assigned to one seat, absent for at least one game, and returned later; assert the persistent ID is unchanged and distinct from seat and policy-bank IDs.
- Rotate two individuals through one physical row and prove their history features do not leak.
- Move one individual between seats/rows and prove identity-keyed history follows only if the benchmark contract says it should.
- Assert terminal clears native MinGRU state, and separately verify any intended persistent benchmark memory survives at the explicit layer only.
- Replay the same identity/change schedule with the same seed and obtain byte-identical schedule/action masks.

### Self-play/evaluation

- If historical banks are used, swap opponents only at an explicitly tested game boundary; do not rely on the current timeout comment or `boundary_reached` field.
- Record checkpoint identity separately from individual identity.
- Do not invoke the one-agent/one-bot ladder for seven-player evaluation.
- Report per-power raw welfare and social welfare from structured records, not only `env/perf` or `policy_0_score`.

### Logs

- Complete two known games and verify `env/n == 2` and every aggregate field equals the mean of the two per-game sums.
- Verify `puf_reset` does not clear accumulated log data; only the core's clear operation should do that.
- Verify intervals with no completed games do not masquerade as zero-welfare episodes.

## Explicit limits of this audit

- It does not certify the C adjudicator or its map. Those require the existing DATC/reference test program.
- It does not choose a new Welfare Diplomacy rule where the paper and pinned reference conflict. The local rule document requires a human decision for those cases (`PufferWelfare/docs/rules/welfare-diplomacy.md:43-57`, `76-84`).
- It does not claim that `boundary_reached` works for native self-play; the pinned core writes/initializes it but does not read it.
- It does not claim that the stale Python tests in the upstream checkout describe the native 5.0 API.
- It does not claim that PufferLib supplies persistent individuals, returning partners, private press, full-order autoregression, or seven-player matchmaking. Those must be implemented and tested deliberately.
- It does not benchmark the memory/performance cost of the proposed atomic-order vocabulary. That measurement belongs in the pilot implementation because mask storage scales as `total_agents × vocabulary_size`.

## Go/no-go gates for the pilot

Proceed to a training pilot only when all of the following are true:

1. one power can submit a complete multi-unit order set without implicit holds replacing intended orders;
2. legal masks are exact and never empty;
3. a known Winter delta reaches the PPO input under an explicitly documented scale;
4. the final transition emits reward, terminal, and reset observation with the upstream convention;
5. vector configuration consists only of whole seven-player games per buffer;
6. for a returning-partner benchmark only, persistent individual IDs and memory scope are specified independently of policy banks and physical rows; and
7. a deterministic short native run reproduces actions, rewards, terminals, masks, and structured episode records from a fixed seed.

Until then, PufferLib 5.0 should be treated as a promising execution backend, not as evidence that the Welfare Diplomacy benchmark interface is complete.
