# Build and release

## Build reliability

### BUILD-001 — Make builds reproducible

- **Status** Planned
- **Depends on** [TST-002](verification.md#tst-002--isolate-cli-test-runs)
- **Scope** Make the build track header changes for tests and benchmarks. Keep debug and optimized builds in distinct output directories. Coordinate CLI executable selection with [TST-002](verification.md#tst-002--isolate-cli-test-runs). [Makefile](../../Makefile#L59) · [Build contract](../requirements/verification.md#verify-001--build-and-platform-coverage)

**Done when**

- A header change rebuilds the relevant test and benchmark binaries.
- A clean default build and a separate optimized build both produce and test their own artifacts.
- The supported macOS and Linux commands and output locations are documented.

### BUILD-002 — Separate reusable isolation definitions

- **Status** Planned
- **Depends on** [TST-006](verification.md#tst-006--verify-isolation-candidates-and-witnesses)
- **Scope** Fix the duplicate-symbol link failure when two translation units include `isolation.hpp`. Move non-inline function definitions into a compiled source file while preserving the public isolation contract. [Header definitions](../../include/isolation.hpp#L342) · [Isolation requirements](../requirements/isolation.md)

**Done when**

- A link check includes the header from two translation units and succeeds.
- The selected isolation and CLI acceptance tests still pass after the move.

## Milestone M5 — Release

### REL-001 — Verify the release candidate

- **Status** Planned
- **Depends on** M1–M4 and [BUILD-002](#build-002--separate-reusable-isolation-definitions)
- **Scope** Verify one `development` commit against the agreed scope. Use the [coverage map](coverage.md), confirm that tests exercise the candidate binaries, and review the OR-Tools adoption decision and supported build modes.

**Done when**

- Each agreed requirement links to its test or other verification evidence. Accepted scope decisions are reflected in the requirements and associated tests.
- The full suite passes on macOS and Linux, including the resolved isolation theory examples and the exact ArgoUML summary. The documentation build produces its HTML entry point.
- The release record identifies the commit tested, the supported build modes, and the OR-Tools decision. If the optional solver is included in the release, its contract tests pass on both supported platforms.

### REL-002 — Merge the tested candidate into main

- **Status** Planned
- **Depends on** [REL-001](#rel-001--verify-the-release-candidate)
- **Scope** Integrate the verified `development` candidate into `main` once the intended work is complete. [Branch workflow](../../AGENTS.md#branch-workflow)

**Done when**

- `main` contains the verified candidate, with verification repeated if the merge changes its contents.
- Tickets point to their verification evidence and the delivered commit.
