# M1 — Rework the test suite

The milestone delivers tests that can exercise the same requirements across changes to the parser, analysis, reports, and solver. Keep the existing Make targets as entry points. Use the [requirements index](../requirements/README.md) for expected behavior and the [coverage map](coverage.md) to track which tickets will verify it.

The starting suite has two stale test expectations in its default run and two failures in its opt-in isolation theory cases. [Known discrepancies](../requirements/verification.md#known-discrepancies)

## Test-driven workflow

1. Pick a decided requirement and write a case from its Given/When/Then example. Put the requirement ID in the case name or metadata.
2. For behavior changes, observe that the case fails for the expected reason. For behavior already implemented, verify that the case passes and record what outcome it checks.
3. Make the smallest change that passes the case. Refactor with the new case and the existing suite running.
4. Record the target run, result, and any separately tracked discrepancy on the ticket. Replace an implementation-coupled test after the behavioral replacement covers its intended outcome.

Use CLI tests for process outcomes, interactive behavior, and public files. Use focused library tests for public API contracts and small independent fixtures or oracles for domain rules. Assert exact serialization where a requirement specifies it. Where the contract allows equivalent results, compare truth values, counts, sets, or fields. Keep expected values independent of the production search and data structures.

Use this milestone to replace tests for behaviors that already work and to prepare the harness and oracles for fixes. Add a test that demonstrates a known code discrepancy on the branch for its corresponding fix. That branch takes the test from failing to passing before integration.

### TST-001 — Restore agreed DNF and repeated trace-key expectations

- **Status** Ready
- **Depends on** None
- **Scope** Replace the stale expectations in the [analysis test](../../src/analysis_test.cpp#L222) and the [missing-system trace test](../../src/cli_regression_test.cpp#L618). Use [EXPR-003](../requirements/expressions.md#expr-003--expression-output), [FEATURE-008](../requirements/feature-space.md#feature-008--raw-and-simplified-terms), and [TRACE-001](../requirements/traces.md#trace-001--repeated-trace-keys).
- **Done when** The analysis case checks DNF and the approved truth values. The grade-1 trace case checks that both expressions sharing `2,3` are present as separate rows and the missing ID is absent from the parsed keys. `make test_analysis test_cli` passes those cases. The opt-in isolation discrepancies remain visible in the [verification notes](../requirements/verification.md#known-discrepancies).

### TST-002 — Isolate CLI test runs

- **Status** Planned
- **Depends on** [TST-001](#tst-001--restore-agreed-dnf-and-repeated-trace-key-expectations)
- **Scope** Have the [Make test target](../../Makefile#L95) supply the CLI binary built for that invocation to the [CLI harness](../../src/cli_regression_test.cpp#L37). Give each case a temporary output directory and capture standard output, standard error, and process status independently for noninteractive tests. Retain a pseudo-terminal for cases that exercise guided behavior.
- **Done when** `make test_cli` passes with default and custom output directories and each run exercises its own CLI binary. Two runs can execute without sharing generated reports or fixtures. A timeout or signal yields a visible test failure, and a case can distinguish a diagnostic from report or expression output.

### TST-003 — Verify SPL, arithmetic, and feature-space contracts

- **Status** Planned
- **Depends on** [TST-001](#tst-001--restore-agreed-dnf-and-repeated-trace-key-expectations)
- **Scope** Review the [SPL tests](../../src/spl_test.cpp), [math tests](../../src/mathhelper_test.cpp), and [feature analysis tests](../../src/analysis_test.cpp) against their public contracts. Use table-driven arithmetic values and boundary cases, small SPL truth tables for valid system IDs and availability, and hand-computed feature-space cases for masks, category counts, and lookup behavior. Keep the serialization assertions required by the SPL contract. Replace compiled feature-index checks and cache assumptions with observable outcomes. Track unresolved cases in [M2](correctness.md).
- **Done when** `make test_spl test_math test_analysis` exercises accepted and rejected inputs, Boolean and cardinality constraints, feature ID and membership rules, arithmetic boundaries, and public errors. Expected system sets and small feature spaces come from fixture truth tables or an independent test calculation.

### TST-004 — Verify CLI, planning, and report contracts

- **Status** Planned
- **Depends on** [TST-002](#tst-002--isolate-cli-test-runs), [TST-003](#tst-003--verify-spl-arithmetic-and-feature-space-contracts)
- **Scope** Exercise help and dispatch, guided choices, option precedence, planning, approval and cancellation, and report modes from the executable. Assert agreed statuses, diagnostics, and destinations. Verify report totals, sections, columns, and filter semantics. Use semantic comparisons for estimates and data where rendering is flexible. Keep output schema and ordering checks required by the specification.
- **Done when** `make test_cli` covers the decided CLI, planning, and report scenarios. A report case can check counts directly from its output without accessing derived-feature registry state. The same checks can be used for the exact ArgoUML case in SCALE-001.

### TST-005 — Verify expression and trace behavior by truth table

- **Status** Planned
- **Depends on** [TST-002](#tst-002--isolate-cli-test-runs), [TST-003](#tst-003--verify-spl-arithmetic-and-feature-space-contracts)
- **Scope** Evaluate emitted expressions on small independently enumerated system sets. Check selected and unselected valid IDs, constants, DNF, and agreed representation rules. For traces, check grade limits, formula families, keys as sets of available IDs, repeated-key rows, and the file schema. Use explicit expected lines for required serialization examples. Apply the same contract cases to any expression backend trial.
- **Done when** `make test_analysis test_cli` includes valid, missing, and invalid-ID cases as specified, simple and complex trace examples, and the shared-key fixture. An incorrect expression truth value or dropped required row fails a case. Search order and the spelling of equivalent formulas remain flexible where the specification leaves them open.

### TST-006 — Verify isolation candidates and witnesses

- **Status** Planned
- **Depends on** [TST-003](#tst-003--verify-spl-arithmetic-and-feature-space-contracts)
- **Scope** Give the [isolation suite](../../src/isolation_test.cpp) fixture-driven cases for candidates, witness labels, and single and minimal modes. On small systems, generate witness subsets independently and verify consistency and minimality for cases covered by the agreed rules. FIX-004 adds the outstanding missing-system, unknown-target, and caller-supplied membership cases once DEC-001 settles their expected outcomes.
- **Done when** `make test_isolation` asserts the agreed candidate sets and permitted witnesses without relying on search traversal. The existing failing theory cases link to DEC-001 and FIX-004.

### TST-007 — Integrate coverage and retire superseded tests

- **Status** Planned
- **Depends on** [TST-001](#tst-001--restore-agreed-dnf-and-repeated-trace-key-expectations), [TST-002](#tst-002--isolate-cli-test-runs), [TST-003](#tst-003--verify-spl-arithmetic-and-feature-space-contracts), [TST-004](#tst-004--verify-cli-planning-and-report-contracts), [TST-005](#tst-005--verify-expression-and-trace-behavior-by-truth-table), [TST-006](#tst-006--verify-isolation-candidates-and-witnesses)
- **Scope** Review every existing suite against the [coverage map](coverage.md). Preserve checks for specified public C++ behavior. Replace assertions about storage, construction, search order, and compiled indexes where a behavioral case now provides coverage. Exercise supported platforms in [CI](../../.github/workflows/test.yml) and link pending decisions or known failures to tickets. Run the benchmark migration independently under SCALE-002.
- **Done when** The current `make test_*` entry points and `make test` use the correct build and pass on macOS and Linux for decided scenarios. Each mapped requirement has a named case or a linked gap, and every superseded test has a behavioral replacement or a documented reason to keep it.
