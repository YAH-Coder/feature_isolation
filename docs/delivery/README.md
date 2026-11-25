# Delivery backlog

Use this backlog to bring the implementation and tests into agreement with the [requirements](../requirements/README.md), then measure where changes can improve the CLI. The [coverage map](coverage.md) links each requirement to its verification ticket.

## Milestones

| Milestone | Scope | Completion evidence |
| --- | --- | --- |
| M1 · Acceptance tests | [Test-suite rework](verification.md) and [build reliability](release.md#build-001--make-builds-reproducible) | Existing behavior has executable checks at public boundaries, and the harness can exercise subsequent fixes. The build tests the binaries it produced on macOS and Linux. |
| M2 · Correctness | [Decisions and implementation fixes](correctness.md) | Required behavior is decided and verified for SPL parsing, CLI workflows, reporting, expressions, traces, and isolation. |
| M3 · Exact summary and baseline | [ArgoUML summary and measurements](scalability.md) | `REPORT-008` passes and representative release-build results are reproducible across the CLI workflows. |
| M4 · OR-Tools trial | [Solver experiments](scalability.md#m4--or-tools-c-trial) | The paths are evaluated with the same acceptance cases. The comparison and adoption decision identify any workloads that benefit. |
| M5 · Release | [Integration and release checks](release.md#milestone-m5--release) | `development` passes the agreed acceptance checks on macOS and Linux and is merged into `main`. |

Start with [TST-001](verification.md#tst-001--restore-agreed-dnf-and-repeated-trace-key-expectations) to restore the agreed DNF and repeated-trace-key checks, then [TST-002](verification.md#tst-002--isolate-cli-test-runs) to make the acceptance harness reliable. Requirement decisions in M2 can proceed in parallel. The exact ArgoUML summary can start when the CLI harness can exercise its scenario. The solver work follows the relevant correctness checks and release-build baseline.

## Working a ticket

- Keep its status on the ticket. Use **Ready** when it can start, **Planned** while prerequisites are pending, **In progress** while it is being worked, **Blocked** when it needs a named decision or external change, and **Done** when its acceptance evidence is recorded.
- Start the feature or fix branch from `development`. Use the ticket ID in the branch or commit description so the change is easy to find. Merge the branch into `development` after its relevant checks pass. [Branch workflow](../../AGENTS.md#branch-workflow)
- For a behavior change, turn the agreed Given/When/Then example into a failing test at the lowest public boundary that can verify it. Make the test pass, then refactor and rerun it. For a behavior-preserving refactor, establish passing examples before editing.
- Record the test command, the requirement IDs it verifies, and the commit that was checked. Keep unresolved behaviors in decision tickets and update the EARS requirement when a decision is made.
