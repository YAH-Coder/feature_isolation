# Delivery backlog

Open the repository root as an Obsidian vault and open [Delivery.base](Delivery.base). The Base reads each milestone and ticket's Markdown properties. Edit **Status** on the note or in a view that shows the status column.

- **Milestone review** shows the five proposed milestones and their status.
- **Ticket proposals** groups draft tickets by milestone.
- **Ticket board** shows Ready, In progress, Review, Blocked, and Done as Kanban columns. Drag a card to update its status. This view requires Obsidian 1.14 or later.

## Working a ticket

- Review milestone scope together before approving its ticket proposals. A ticket starts as **Draft**. Set it to **Ready** after its scope is agreed and its prerequisites are met.
- Use **In progress** when work starts and **Review** when it is ready for review. Use **Blocked** when an active ticket needs a decision or dependency and record what would unblock it.
- Mark a ticket **Done** once its acceptance evidence is recorded and any associated branch has merged into `development`. If an acceptance gap is found after completion, reopen the ticket and record the gap.
- Start the feature or fix branch from `development`. Use the ticket ID in the branch or commit description. See the [branch workflow](../../AGENTS.md#branch-workflow).
- For a behavior change, turn the agreed Given/When/Then example into a failing test at the lowest public boundary that can verify it. Make the test pass, then refactor and rerun it. For a behavior-preserving refactor, establish passing examples before editing.
- Record the test command, the requirement IDs it verifies, and the commit that was checked. Keep unresolved behaviors in decision tickets and update the EARS requirement when a decision is made.

## Milestones

- [M1 · Acceptance tests and build reliability](milestones/M1.md)
- [M2 · Correctness](milestones/M2.md)
- [M3 · Exact ArgoUML summary and baseline](milestones/M3.md)
- [M4 · OR-Tools C++ trial](milestones/M4.md)
- [M5 · Release](milestones/M5.md)

The [coverage map](coverage.md) links each [requirement](../requirements/README.md) to its proposed verification tickets.
