# Pilot: establish a reproducible agent development loop

GitHub issue creation is pending because repository Issues is disabled. Copy
this body into the first issue after enabling it and link it from the pilot PR.

## Scope

Add source-grounded agent instructions and engine/review skills, issue and PR
forms, human ownership paths, a focused build/test gate, and an author worktree
helper. Correct stale size assertions using the current header/wrapper/storage
contract. Document simulator defects and staged repository settings.

## Acceptance criteria

- An author works on a separate branch/worktree and records its identity.
- The focused extension builds without unrelated environment dependencies.
- The full documented gate passes; skipped welfare coverage remains explicit.
- A different agent inspects the diff and records findings and validation.
- A draft PR contains that review and CI status; the human owns merge approval.
- Issues, labels/milestone, Actions availability, and branch rules are verified
  or explicitly left as owner actions with concrete instructions.

## Non-goals

No engine rule changes, observation/action redesign, RL training, blanket rewrite,
autonomous merge permission, or copying additional reference implementation code.

Suggested labels: `enhancement`, `area:infra`, `human:approval-required`.
Suggested milestone: `M0: Reliable simulator`.
