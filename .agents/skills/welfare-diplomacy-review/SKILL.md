---
name: welfare-diplomacy-review
description: Review a PufferWelfare change to Diplomacy rules, C/Python code, tests, CI, or observations/actions/rewards. The reviewer must be a different agent from the author and must not merge.
---

# Reviewing a change

Any coding agent can review work by another session, including work authored
with a different tool. Review the code and evidence, not the author's tool choice.

Do not review your own work. Ask for a different reviewer if you authored it.
Read the issue, diff, `AGENTS.md`, and `docs/rules/welfare-diplomacy.md`.
Do not edit the author's checkout or merge the PR.

1. Check that the change solves the issue and stays within its stated scope.
2. For game changes, check that the cited source supports the expected result.
   Current code and old status reports can be wrong.
3. Check memory and array bounds across C/Python, phase transitions, observations,
   actions, rewards, reset behavior, and standard/welfare modes where affected.
4. Check that new tests catch the original bug. Inspect changed expectations,
   removed checks, and new skips. Reject changes made only to hide failures.
5. Run the relevant tests and the full command in `AGENTS.md`. Report failures,
   errors, skips, and checks you could not run, with reasons.
6. Check the issue and PR are understandable and honest. Report misleading
   claims, omitted problems, and unexplained changes to tests.

Write the problems first, most serious first. Give the file/line, what can go
wrong, and evidence. Separate observed bugs from suspected ones. If you found
no new problems, say so, then list missing checks and remaining uncertainty.
Do not treat a passing suite as proof of untested behavior.

Name the author and reviewer. If both use the same GitHub account, leave a
`COMMENT` review, not `APPROVE`. The owner will read and discuss the pilot PR
before approving. Do not merge without explicit human approval.
