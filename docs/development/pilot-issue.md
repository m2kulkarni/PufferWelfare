# First setup issue

Filed as [issue #2](https://github.com/m2kulkarni/PufferWelfare/issues/2).
The current progress and remaining problems are in that issue and
[PR #1](https://github.com/m2kulkarni/PufferWelfare/pull/1).

The first change adds separate agent checkouts, coding/review instructions,
issue and PR forms, human ownership of important files, and simulator tests in CI.
It also corrects old test size expectations using the existing C header and
Python wrapper. It does not fix game rules or redesign the agent interface.

It is ready to merge only after another agent reviews it, GitHub tests pass,
and the owner reads the entire PR, discusses it, and approves.
Skipped tests, missing CI, known bugs, and unfinished settings must stay visible.
Do not claim this setup proves the simulator is correct.
