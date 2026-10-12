# Multi-agent development workflow

Each coding agent works in a separate checkout. Another agent reviews the change,
and the owner approves the merge. Follow the plain-language and honesty rules in
`AGENTS.md`: report bugs and failed, skipped, or unrun checks.

Codex and Claude Code are collaborators with the same responsibilities. Either
can author or review; use a different session for review. Shared instructions live
in `AGENTS.md` and `.agents/skills/`, not in a particular tool's private config.

## Shared files

- Codex reads `AGENTS.md` and discovers skills under `.agents/skills/`.
- Root and Diplomacy `CLAUDE.md` files import the adjacent `AGENTS.md`.
- Claude's `.claude/skills/<name>` links point to the shared skill folders.
  Edit the shared files, not separate copies for each tool.
- Other tools can read those same files explicitly. If a checkout does not
  support symlinks, open `.agents/skills/<name>/SKILL.md` directly.

Give each session an issue, role, worktree path, and unique name. Handoffs must
include the branch/commit, changes, test results, and remaining problems in the
issue or PR. Private chat history is not a substitute for that record.

Discovery follows the documented conventions for
[Codex skills](https://developers.openai.com/codex/skills/),
[Claude instructions](https://code.claude.com/docs/en/memory), and
[Claude skills](https://code.claude.com/docs/en/skills).

## Responsibility boundaries

| Capability | Native mechanism | What it does not guarantee |
| --- | --- | --- |
| Separate edits | Git branch plus worktree, created by Git or the chosen tool | Starting another agent does not by itself give it a separate checkout |
| Task tracking | GitHub Issues and issue forms | Issues, labels, and milestones must first be enabled/configured by a repository admin |
| Code changes | Coding agent in the issue worktree | The agent must not review or merge its own change |
| Independent review | Fresh reviewer agent plus PR review/comment | Two agents using one GitHub account are not distinct GitHub approvers |
| Automatic tests | GitHub Actions `diplomacy-tests` job | CI establishes tested behavior, not the correct interpretation of an ambiguous rule |
| Sensitive approval | CODEOWNERS plus branch rules, backed by a human | Requires a non-author human identity and repository administration access |

## Issue to merge loop

1. A human or orchestrator creates a narrow issue with acceptance criteria,
   non-goals, affected interfaces, and rule evidence where applicable.
2. The orchestrator creates one worktree and branch for the author:

   ```bash
   scripts/new_agent_worktree.sh <issue-number> <short-slug>
   ```

3. The coding agent receives only the issue and its worktree path. It adds a
   failing regression test before a behavioral fix and opens a draft PR.
4. A fresh reviewer agent receives the issue and PR, not the author's hidden
   reasoning. It uses the review skill, runs the relevant checks, and leaves a
   review that starts with the problems found.
5. The author resolves findings. The same reviewer re-checks; if two review rounds fail to resolve a problem,
   explain what is stuck and ask the owner for direction.
6. Before marking ready for human review, check the current commit: independent
   agent review is complete, current CI passes, blocking findings and review
   threads are resolved, and the description reports what changed, test results,
   failures, skips, and unrun checks accurately. Record the checked commit, take
   the PR out of draft, and notify the owner. If a new commit is added, return it
   to draft until this check passes again.
7. A human reads every pilot PR and any sensitive change, discusses it with the
   author, and explicitly approves before merge. Ready for review does not
   authorize a merge. Archive/remove the worktree only after merge.

## GitHub setup stages

### Stage 0: enable the workflow

- Enable GitHub Issues on the repository.
- Install/connect the GitHub integration with read access and low-risk write
  actions; retain review for important actions.
- Create milestone `M0: Reliable simulator` for correctness, determinism, and a
  reproducible test baseline.

For a dedicated GitHub App, select this repository only. Author capabilities
need repository Contents and Pull requests write, Issues write for task tracking,
and Actions/Checks read to inspect CI. Reviewer capabilities need Contents read
and Pull requests write for reviews, plus Actions/Checks read. Metadata read is
implicit. A workflow-file write capability should be limited to infrastructure
tasks where the integration supports it. Neither role needs repository
administration, secrets access, or merge authority. These are desired capability
boundaries, not a claim about the installed connector's exact token scopes.
The owner performs administration. A local worktree is isolation for edits,
not a credential/security sandbox.

### Stage 1: minimal labels

Create only labels that change routing decisions:

| Label | Purpose |
| --- | --- |
| `bug` (existing) | Existing behavior violates a supported contract |
| `enhancement` (existing) | New or changed behavior |
| `area:engine` | C engine, binding, map, or wrapper |
| `area:rl` | Observation, action, reward, termination, policy, or training |
| `area:infra` | CI, packaging, agent workflow, or repository settings |
| `risk:rules` | Changes game or adjudication semantics |
| `risk:interface` | Changes a public/buffer/checkpoint-compatible interface |
| `agent:ready` | Scope and acceptance criteria are sufficient for assignment |
| `human:approval-required` | Must not merge without recorded human review |

Do not encode priority in a label until there is enough issue volume to need it.

### Stage 2: branch rules

Apply a ruleset to `welfare` only after the pilot CI check has appeared and the
author/reviewer GitHub identities are known:

- require a pull request;
- block force pushes and branch deletion;
- require conversation resolution;
- require the `diplomacy-tests` check;
- require Code Owner review for sensitive paths only when the PR author is a
  distinct bot or collaborator identity.

With all agents and the human using the same GitHub account, a required approval
would prevent approval of PRs made through the owner's account and would not prove agent independence. In that
configuration, keep human merge as a manual approval or provision a dedicated
author bot/account before enabling required approvals.

## Sensitive paths

Human review is required for `.github/**`, `AGENTS.md`, `CLAUDE.md`, `.agents/**`,
`.claude/**`, build and
dependency files, the Diplomacy engine and tests, and the observation/action/
reward-facing model/config files listed in `.github/CODEOWNERS`.
