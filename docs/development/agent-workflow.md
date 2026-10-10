# Multi-agent development workflow

This is the minimum control plane for PufferWelfare. It favors traceability and
rule fidelity over agent throughput.

## Responsibility boundaries

| Capability | Native mechanism | What it does not guarantee |
| --- | --- | --- |
| Coding isolation | Git branch plus worktree; Codex managed worktrees when explicitly created | Collaboration subagents share a filesystem and do not automatically get separate worktrees |
| Task tracking | GitHub Issues and issue forms | Issues, labels, and milestones must first be enabled/configured by a repository admin |
| Code changes | Coding agent in the issue worktree | The agent must not review or merge its own change |
| Independent review | Fresh reviewer agent plus PR review/comment | Two agents using one GitHub account are not distinct GitHub approvers |
| Automated gate | GitHub Actions `diplomacy-tests` job | CI establishes tested behavior, not the correct interpretation of an ambiguous rule |
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
   findings-first review.
5. The author resolves findings. The same reviewer re-checks; after two failed
   review cycles, escalate rather than looping indefinitely.
6. CI must pass. A human reviews every pilot PR and any sensitive change, then
   performs the merge. The orchestrator archives/removes the worktree only after
   merge.

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
would deadlock self-authored PRs and would not prove agent independence. In that
configuration, keep human merge as a procedural gate or provision a dedicated
author bot/account before enabling required approvals.

## Sensitive paths

Human review is required for `.github/**`, `AGENTS.md`, `.codex/**`, build and
dependency files, the Diplomacy engine and tests, and the observation/action/
reward-facing model/config files listed in `.github/CODEOWNERS`.
