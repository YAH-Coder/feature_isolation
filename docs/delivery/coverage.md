# Requirements coverage

This map assigns each EARS requirement to the ticket that will verify it. A link shows planned coverage. Test names and results belong in the ticket's acceptance evidence as work is completed. Requirements with code discrepancies get their failing scenario and correction on the fix branch.

## Requirements

| Requirement | Verification ticket |
| --- | --- |
| [CLI-001](../requirements/cli.md#cli-001--direct-invocation) | [TST-004](tickets/TST-004.md) |
| [CLI-002](../requirements/cli.md#cli-002--help) | [TST-004](tickets/TST-004.md) |
| [CLI-003](../requirements/cli.md#cli-003--report-options) | [TST-004](tickets/TST-004.md) |
| [CLI-004](../requirements/cli.md#cli-004--entry-and-inputs) | [TST-004](tickets/TST-004.md) |
| [CLI-005](../requirements/cli.md#cli-005--report-choices) | [TST-004](tickets/TST-004.md) |
| [CLI-006](../requirements/cli.md#cli-006--status-and-diagnostics) | [TST-004](tickets/TST-004.md) |
| [CLI-007](../requirements/cli.md#cli-007--report-destination-side-effects) | [TST-004](tickets/TST-004.md) |
| [EXPR-001](../requirements/expressions.md#expr-001--command-inputs) | [TST-005](tickets/TST-005.md) |
| [EXPR-002](../requirements/expressions.md#expr-002--matching-system-ids) | [TST-005](tickets/TST-005.md) |
| [EXPR-003](../requirements/expressions.md#expr-003--expression-output) | [TST-001](tickets/TST-001.md), [TST-005](tickets/TST-005.md) |
| [EXPR-004](../requirements/expressions.md#expr-004--simplification-preference) | [TST-005](tickets/TST-005.md) |
| [EXPR-005](../requirements/expressions.md#expr-005--failure-and-help-behavior) | [TST-005](tickets/TST-005.md) |
| [EXPR-006](../requirements/expressions.md#expr-006--guided-entry) | [TST-005](tickets/TST-005.md) |
| [EXPR-007](../requirements/expressions.md#expr-007--c-expression-entry-points) | [TST-005](tickets/TST-005.md) |
| [FEATURE-001](../requirements/feature-space.md#feature-001--system-identifiers-and-counts) | [TST-003](tickets/TST-003.md) |
| [FEATURE-002](../requirements/feature-space.md#feature-002--complete-partition-required-for-direct-analysis) | [TST-003](tickets/TST-003.md) |
| [FEATURE-003](../requirements/feature-space.md#feature-003--a-derived-feature-is-identified-by-its-support) | [TST-003](tickets/TST-003.md) |
| [FEATURE-004](../requirements/feature-space.md#feature-004--derived-feature-numbering-and-bit-order) | [TST-003](tickets/TST-003.md) |
| [FEATURE-005](../requirements/feature-space.md#feature-005--per-system-membership) | [TST-003](tickets/TST-003.md) |
| [FEATURE-006](../requirements/feature-space.md#feature-006--observable-categories) | [TST-003](tickets/TST-003.md) |
| [FEATURE-007](../requirements/feature-space.md#feature-007--equivalent-simple-forms) | [TST-003](tickets/TST-003.md) |
| [FEATURE-008](../requirements/feature-space.md#feature-008--raw-and-simplified-terms) | [TST-001](tickets/TST-001.md), [TST-003](tickets/TST-003.md) |
| [FEATURE-009](../requirements/feature-space.md#feature-009--naming-and-optional-details) | [TST-003](tickets/TST-003.md) |
| [FEATURE-010](../requirements/feature-space.md#feature-010--lookup-behavior) | [TST-003](tickets/TST-003.md) |
| [ISO-001](../requirements/isolation.md#iso-001--targets-and-system-membership) | [TST-006](tickets/TST-006.md) |
| [ISO-002](../requirements/isolation.md#iso-002--candidate-set) | [TST-006](tickets/TST-006.md) |
| [ISO-003](../requirements/isolation.md#iso-003--witness-modes) | [TST-006](tickets/TST-006.md) |
| [ISO-004](../requirements/isolation.md#iso-004--caller-supplied-system-memberships) | [TST-006](tickets/TST-006.md) |
| [ISO-005](../requirements/isolation.md#iso-005--report-rows) | [TST-006](tickets/TST-006.md) |
| [MATH-001](../requirements/math.md#math-001--unsigned-arithmetic) | [TST-003](tickets/TST-003.md) |
| [MATH-002](../requirements/math.md#math-002--counts) | [TST-003](tickets/TST-003.md) |
| [MATH-003](../requirements/math.md#math-003--selection-and-bit-values) | [TST-003](tickets/TST-003.md) |
| [PLAN-001](../requirements/planning.md#plan-001--planning-output) | [TST-004](tickets/TST-004.md) |
| [PLAN-002](../requirements/planning.md#plan-002--risk-labels) | [TST-004](tickets/TST-004.md) |
| [PLAN-003](../requirements/planning.md#plan-003--recommendations-and-drill-down) | [TST-004](tickets/TST-004.md) |
| [PLAN-004](../requirements/planning.md#plan-004--report-approval) | [TST-004](tickets/TST-004.md) |
| [PLAN-005](../requirements/planning.md#plan-005--guardrail-checks) | [TST-004](tickets/TST-004.md) |
| [REPORT-001](../requirements/reports.md#report-001--report-modes-and-sections) | [TST-004](tickets/TST-004.md) |
| [REPORT-002](../requirements/reports.md#report-002--specification-and-tabular-format) | [TST-004](tickets/TST-004.md) |
| [REPORT-003](../requirements/reports.md#report-003--totals-and-category-selection) | [TST-004](tickets/TST-004.md) |
| [REPORT-004](../requirements/reports.md#report-004--systems-section) | [TST-004](tickets/TST-004.md) |
| [REPORT-005](../requirements/reports.md#report-005--feature-tables) | [TST-004](tickets/TST-004.md) |
| [REPORT-006](../requirements/reports.md#report-006--terms-and-isolation-output) | [TST-004](tickets/TST-004.md) |
| [REPORT-007](../requirements/reports.md#report-007--output-destination) | [TST-004](tickets/TST-004.md) |
| [REPORT-008](../requirements/reports.md#report-008--exact-argouml-summary) | [SCALE-001](tickets/SCALE-001.md) |
| [SPL-001](../requirements/spl.md#spl-001--optional-name-and-feature-basis) | [TST-003](tickets/TST-003.md) |
| [SPL-002](../requirements/spl.md#spl-002--words-numbers-and-comments) | [TST-003](tickets/TST-003.md) |
| [SPL-003](../requirements/spl.md#spl-003--supported-sections-and-their-boundaries) | [TST-003](tickets/TST-003.md) |
| [SPL-004](../requirements/spl.md#spl-004--category-hint) | [TST-003](tickets/TST-003.md) |
| [SPL-005](../requirements/spl.md#spl-005--boolean-formulas) | [TST-003](tickets/TST-003.md) |
| [SPL-006](../requirements/spl.md#spl-006--cardinality-formulas) | [TST-003](tickets/TST-003.md) |
| [SPL-007](../requirements/spl.md#spl-007--candidate-system-numbering) | [TST-003](tickets/TST-003.md) |
| [SPL-008](../requirements/spl.md#spl-008--valid-systems-are-the-satisfying-assignments) | [TST-003](tickets/TST-003.md) |
| [SPL-009](../requirements/spl.md#spl-009--available-and-missing-systems-partition-the-valid-set) | [TST-003](tickets/TST-003.md) |
| [SPL-010](../requirements/spl.md#spl-010--parsed-sections-and-resolved-system-space) | [TST-003](tickets/TST-003.md) |
| [SPL-011](../requirements/spl.md#spl-011--normalized-output) | [TST-003](tickets/TST-003.md) |
| [SPL-012](../requirements/spl.md#spl-012--error-reporting-and-coverage) | [TST-003](tickets/TST-003.md) |
| [TRACE-001](../requirements/traces.md#trace-001--repeated-trace-keys) | [TST-001](tickets/TST-001.md), [TST-005](tickets/TST-005.md) |
| [TRACE-002](../requirements/traces.md#trace-002--command-inputs-and-defaults) | [TST-005](tickets/TST-005.md) |
| [TRACE-003](../requirements/traces.md#trace-003--grade-and-formula-family) | [TST-005](tickets/TST-005.md) |
| [TRACE-004](../requirements/traces.md#trace-004--valid-system-membership-and-representative-expressions) | [TST-005](tickets/TST-005.md) |
| [TRACE-005](../requirements/traces.md#trace-005--file-format-and-order) | [TST-005](tickets/TST-005.md) |
| [TRACE-006](../requirements/traces.md#trace-006--interactive-choices-and-candidate-counts) | [TST-005](tickets/TST-005.md) |
| [TRACE-007](../requirements/traces.md#trace-007--error-behavior) | [TST-005](tickets/TST-005.md) |
| [TRACE-008](../requirements/traces.md#trace-008--c-trace-api) | [TST-005](tickets/TST-005.md) |
| [VERIFY-001](../requirements/verification.md#verify-001--build-and-platform-coverage) | [BUILD-001](tickets/BUILD-001.md) |
| [VERIFY-002](../requirements/verification.md#verify-002--test-entry-points) | [TST-007](tickets/TST-007.md) |
| [VERIFY-003](../requirements/verification.md#verify-003--benchmarks) | [SCALE-002](tickets/SCALE-002.md) |
| [VERIFY-004](../requirements/verification.md#verify-004--reference-documentation) | [REL-001](tickets/REL-001.md) |

## Decisions and discrepancies

| Topic | Decision and fix tickets |
| --- | --- |
| [Theory source](../requirements/README.md#req-d01--theory-reference), [isolation with missing systems and witness rules](../requirements/isolation.md#open-decisions) | [DEC-001](tickets/DEC-001.md), [FIX-004](tickets/FIX-004.md) |
| [DNF optimality and ties](../requirements/expressions.md#expr-d01--normal-form-and-optimality), [complex report terms](../requirements/feature-space.md#feature-d01--complex-expression-contract) | [DEC-002](tickets/DEC-002.md), [FIX-005](tickets/FIX-005.md), [ORT-001](tickets/ORT-001.md) |
| [CLI diagnostics and guided expression input](../requirements/cli.md#open-decisions), [trace errors and file handling](../requirements/traces.md#trace-d03--validation-and-write-errors), [report destination on refusal](../requirements/reports.md#open-decisions) | [DEC-003](tickets/DEC-003.md), [FIX-002](tickets/FIX-002.md), [FIX-003](tickets/FIX-003.md) |
| [SPL declaration and lexical rules](../requirements/spl.md#open-decisions), [trace API edge cases](../requirements/traces.md#deviations-and-open-decisions), [arithmetic boundaries](../requirements/math.md#open-decisions) | [DEC-004](tickets/DEC-004.md), [FIX-001](tickets/FIX-001.md), [FIX-005](tickets/FIX-005.md), [FIX-006](tickets/FIX-006.md) |
| [Supported counts](../requirements/feature-space.md#feature-d02--supported-counts) and [report admission](../requirements/planning.md#open-decisions) | [DEC-005](tickets/DEC-005.md), [FIX-002](tickets/FIX-002.md), [FIX-003](tickets/FIX-003.md) |
| [Stale CNF and trace tests](../requirements/verification.md#known-discrepancies) | [TST-001](tickets/TST-001.md) |
| [Guided category menu](../requirements/cli.md#known-discrepancies) | [FIX-002](tickets/FIX-002.md) |
| [ArgoUML summary overflow](../requirements/reports.md#known-discrepancies) | [SCALE-001](tickets/SCALE-001.md) |
