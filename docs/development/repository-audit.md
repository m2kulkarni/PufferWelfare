# Repository audit and staged backlog

Audited 2026-10-10 at `62df16a0bef7f0f28fb973590b78ad0b8787129e`
on the `welfare` branch. This is a source audit, not a simulator correctness
certificate.

## Architecture and baseline

The simulator is a C engine (`diplomacy.c`, `diplomacy_map.c`, `diplomacy.h`),
exposed by `binding.c` and wrapped by `diplomacy.py`. Ocean's environment registry,
Torch policy, and Diplomacy configuration provide the training integration.
The public README remains largely inherited PufferLib documentation.

The general build compiles unrelated environments and downloads native
dependencies. The pilot adds a focused extension builder and test requirements.
The inherited workflow installed the package but did not run simulator tests.

The full Diplomacy command in AGENTS.md initially produced **205 passed,
5 failed, 46 skipped** on macOS arm64, Python 3.9.6. Five binding/map assertions
still expected obsolete dimensions. The pilot changes only those storage/interface
assertions, grounded in the existing header and wrapper; it does not change engine
behavior or DATC outcomes. The same command then produced **210 passed,
0 failed, 0 errors, 46 skipped**. Linux/Python 3.11 remains to be verified by CI.
Many welfare tests are skipped, and some map tests contain relaxed expectations.
A green baseline therefore does not establish Welfare Diplomacy fidelity.

## Ordered simulator backlog

Each item needs its own issue, minimal reproducer, author worktree, independent
review, and human approval when it changes rules or the RL interface.

1. **Seven-player buffer safety.** The Python constructor accepts arbitrary
   player counts while C uses fixed seven-player arrays. Validate the supported
   count before allocation and C calls; test rejected counts.
2. **Winter reward delivery.** Scoring adds welfare during Winter, but the step
   checks the advanced phase to decide reward emission. Test exactly-once reward
   deltas, including terminal Winter. This is the recommended first behavior pilot.
3. **Supply-center ownership.** Initial power center lists are populated while
   map ownership starts unset. Trace initialization and Fall capture updates;
   binding test helpers currently conceal parts of the problem.
4. **Home-center and map metadata.** Zero initialization makes non-home entries
   look Austrian; Switzerland is represented as movable coast. Check build
   eligibility, terrain, and ownership against cited sources.
5. **Observation coverage.** Internal storage has 82 entries; encoding reads
   the first 81, includes Switzerland, and omits STP/SC at index 81. Define an
   explicit province/coast mapping and test Russia's initial fleet visibility.
   Observation bounds also conflict with signed build-delta features.
6. **Complete simultaneous orders.** The RL path submits one order per power
   and supplies HOLD for remaining units. Action encoding cannot yet represent
   the full support/convoy/retreat/build contract. Design this interface before
   presenting training results as full-game baselines.
7. **Welfare adjustment fidelity.** Investigate voluntary disbands outside
   adjustment, mixed build/disband handling, and split-coast builds. Resolve the
   paper/reference scoring conflict documented in the rule-source file.
8. **Reproducibility and baselines.** Once correctness and interface gates exist,
   add seeded replay checks, random legal and scripted welfare/selfish agents,
   then feedforward/recurrent learning and population evaluation.

Items 2–7 are source-audit findings requiring regression reproductions, not
permission to rewrite the engine wholesale.

## Governance and provenance

GitHub Issues was disabled; creating the pilot issue returned HTTP 410. The
connected account has repository admin rights, but the integration exposes no
administration mutations. The `welfare` branch was unprotected and no rulesets
were returned. Labels, milestone, settings, and protection remain owner actions.
No merge is authorized during the pilot without recorded human approval.

Copied original test files carry AGPL-3.0-or-later notices while the repository
root has MIT licensing. Preserve notices and have the owner resolve provenance
and licensing before redistribution claims or copying additional reference code.

The setup intentionally has two skills (engine work and independent review),
one CI gate, issue/PR forms, and one worktree helper. An external orchestrator
must still assign agents, create their isolated checkouts, record identities,
retry review cycles, and request human merges. No autonomous daemon is installed.
