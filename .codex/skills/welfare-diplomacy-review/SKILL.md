---
name: welfare-diplomacy-review
description: Independently review a PufferWelfare pull request or diff that changes Diplomacy rules, the C/Python simulator boundary, tests, CI, or the RL interface. Do not use to author the same change or to merge it.
---

# Independent PufferWelfare review

Do not review a change you authored. If the author and reviewer agent identities
are the same, stop and have the orchestrator assign a fresh agent context.

Read the linked issue, the diff, `AGENTS.md`, and
`docs/rules/welfare-diplomacy.md`. Review only; do not edit the author's worktree
or merge the pull request.

## Review order

1. Confirm the diff matches the issue's scope and non-goals.
2. For behavioral changes, verify that the claimed rule/interface source actually
   supports the expectation. Treat current code and historical status notes as
   evidence, not authority.
3. Check C/Python ownership and lifetime, bounds, phase transitions, coupled
   observation/action constants, standard-vs-welfare isolation, and deterministic
   reset/reward/termination behavior as applicable.
4. Check that a regression test fails for the original defect and exercises the
   public path where practical. Reject golden-test weakening without a cited
   upstream correction.
5. Run the narrow test and the full Diplomacy gate from `AGENTS.md`, or explain
   exactly why a command could not run.
6. Identify sensitive changes that still require human approval.

Report findings first, ordered by severity, with tight file/line references and a
concrete failure mode. If there are no findings, state that explicitly and list
remaining test or source uncertainty. When author and reviewer share one GitHub
principal, submit a `COMMENT` review rather than an `APPROVE` review because the
platform cannot attest agent separation.
