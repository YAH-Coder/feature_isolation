# Requirements coverage

This map assigns each EARS requirement to the ticket that will verify it. A link shows planned coverage. Test names and results belong in the ticket's acceptance evidence as work is completed. Requirements with code discrepancies get their failing scenario and correction on the fix branch.

## Requirements

| Requirement | Verification ticket |
| --- | --- |
| [CLI-001](../requirements/cli.md#cli-001--direct-invocation) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [CLI-002](../requirements/cli.md#cli-002--help) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [CLI-003](../requirements/cli.md#cli-003--report-options) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [CLI-004](../requirements/cli.md#cli-004--entry-and-inputs) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [CLI-005](../requirements/cli.md#cli-005--report-choices) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [CLI-006](../requirements/cli.md#cli-006--status-and-diagnostics) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [CLI-007](../requirements/cli.md#cli-007--report-destination-side-effects) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [EXPR-001](../requirements/expressions.md#expr-001--command-inputs) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [EXPR-002](../requirements/expressions.md#expr-002--matching-system-ids) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [EXPR-003](../requirements/expressions.md#expr-003--expression-output) | [TST-001](verification.md#tst-001--restore-agreed-dnf-and-repeated-trace-key-expectations), [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [EXPR-004](../requirements/expressions.md#expr-004--simplification-preference) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [EXPR-005](../requirements/expressions.md#expr-005--failure-and-help-behavior) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [EXPR-006](../requirements/expressions.md#expr-006--guided-entry) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [EXPR-007](../requirements/expressions.md#expr-007--c-expression-entry-points) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [FEATURE-001](../requirements/feature-space.md#feature-001--system-identifiers-and-counts) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [FEATURE-002](../requirements/feature-space.md#feature-002--complete-partition-required-for-direct-analysis) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [FEATURE-003](../requirements/feature-space.md#feature-003--a-derived-feature-is-identified-by-its-support) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [FEATURE-004](../requirements/feature-space.md#feature-004--derived-feature-numbering-and-bit-order) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [FEATURE-005](../requirements/feature-space.md#feature-005--per-system-membership) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [FEATURE-006](../requirements/feature-space.md#feature-006--observable-categories) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [FEATURE-007](../requirements/feature-space.md#feature-007--equivalent-simple-forms) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [FEATURE-008](../requirements/feature-space.md#feature-008--raw-and-simplified-terms) | [TST-001](verification.md#tst-001--restore-agreed-dnf-and-repeated-trace-key-expectations), [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [FEATURE-009](../requirements/feature-space.md#feature-009--naming-and-optional-details) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [FEATURE-010](../requirements/feature-space.md#feature-010--lookup-behavior) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [ISO-001](../requirements/isolation.md#iso-001--targets-and-system-membership) | [TST-006](verification.md#tst-006--verify-isolation-candidates-and-witnesses) |
| [ISO-002](../requirements/isolation.md#iso-002--candidate-set) | [TST-006](verification.md#tst-006--verify-isolation-candidates-and-witnesses) |
| [ISO-003](../requirements/isolation.md#iso-003--witness-modes) | [TST-006](verification.md#tst-006--verify-isolation-candidates-and-witnesses) |
| [ISO-004](../requirements/isolation.md#iso-004--caller-supplied-system-memberships) | [TST-006](verification.md#tst-006--verify-isolation-candidates-and-witnesses) |
| [ISO-005](../requirements/isolation.md#iso-005--report-rows) | [TST-006](verification.md#tst-006--verify-isolation-candidates-and-witnesses) |
| [MATH-001](../requirements/math.md#math-001--unsigned-arithmetic) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [MATH-002](../requirements/math.md#math-002--counts) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [MATH-003](../requirements/math.md#math-003--selection-and-bit-values) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [PLAN-001](../requirements/planning.md#plan-001--planning-output) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [PLAN-002](../requirements/planning.md#plan-002--risk-labels) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [PLAN-003](../requirements/planning.md#plan-003--recommendations-and-drill-down) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [PLAN-004](../requirements/planning.md#plan-004--report-approval) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [PLAN-005](../requirements/planning.md#plan-005--guardrail-checks) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [REPORT-001](../requirements/reports.md#report-001--report-modes-and-sections) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [REPORT-002](../requirements/reports.md#report-002--specification-and-tabular-format) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [REPORT-003](../requirements/reports.md#report-003--totals-and-category-selection) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [REPORT-004](../requirements/reports.md#report-004--systems-section) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [REPORT-005](../requirements/reports.md#report-005--feature-tables) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [REPORT-006](../requirements/reports.md#report-006--terms-and-isolation-output) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [REPORT-007](../requirements/reports.md#report-007--output-destination) | [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) |
| [REPORT-008](../requirements/reports.md#report-008--exact-argouml-summary) | [SCALE-001](scalability.md#scale-001--compute-the-exact-argouml-summary) |
| [SPL-001](../requirements/spl.md#spl-001--optional-name-and-feature-basis) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [SPL-002](../requirements/spl.md#spl-002--words-numbers-and-comments) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [SPL-003](../requirements/spl.md#spl-003--supported-sections-and-their-boundaries) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [SPL-004](../requirements/spl.md#spl-004--category-hint) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [SPL-005](../requirements/spl.md#spl-005--boolean-formulas) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [SPL-006](../requirements/spl.md#spl-006--cardinality-formulas) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [SPL-007](../requirements/spl.md#spl-007--candidate-system-numbering) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [SPL-008](../requirements/spl.md#spl-008--valid-systems-are-the-satisfying-assignments) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [SPL-009](../requirements/spl.md#spl-009--available-and-missing-systems-partition-the-valid-set) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [SPL-010](../requirements/spl.md#spl-010--parsed-sections-and-resolved-system-space) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [SPL-011](../requirements/spl.md#spl-011--normalized-output) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [SPL-012](../requirements/spl.md#spl-012--error-reporting-and-coverage) | [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) |
| [TRACE-001](../requirements/traces.md#trace-001--repeated-trace-keys) | [TST-001](verification.md#tst-001--restore-agreed-dnf-and-repeated-trace-key-expectations), [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [TRACE-002](../requirements/traces.md#trace-002--command-inputs-and-defaults) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [TRACE-003](../requirements/traces.md#trace-003--grade-and-formula-family) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [TRACE-004](../requirements/traces.md#trace-004--valid-system-membership-and-representative-expressions) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [TRACE-005](../requirements/traces.md#trace-005--file-format-and-order) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [TRACE-006](../requirements/traces.md#trace-006--interactive-choices-and-candidate-counts) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [TRACE-007](../requirements/traces.md#trace-007--error-behavior) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [TRACE-008](../requirements/traces.md#trace-008--c-trace-api) | [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table) |
| [VERIFY-001](../requirements/verification.md#verify-001--build-and-platform-coverage) | [BUILD-001](release.md#build-001--make-builds-reproducible) |
| [VERIFY-002](../requirements/verification.md#verify-002--test-entry-points) | [TST-007](verification.md#tst-007--integrate-coverage-and-retire-superseded-tests) |
| [VERIFY-003](../requirements/verification.md#verify-003--benchmarks) | [SCALE-002](scalability.md#scale-002--measure-the-cli-end-to-end) |
| [VERIFY-004](../requirements/verification.md#verify-004--reference-documentation) | [REL-001](release.md#rel-001--verify-the-release-candidate) |

## Decisions and discrepancies

| Topic | Decision and fix tickets |
| --- | --- |
| [Theory source](../requirements/README.md#req-d01--theory-reference), [isolation with missing systems and witness rules](../requirements/isolation.md#open-decisions) | [DEC-001](correctness.md#dec-001--isolation-and-missing-systems), [FIX-004](correctness.md#fix-004--align-isolation-candidates-and-witnesses) |
| [DNF optimality and ties](../requirements/expressions.md#expr-d01--normal-form-and-optimality), [complex report terms](../requirements/feature-space.md#feature-d01--complex-expression-contract) | [DEC-002](correctness.md#dec-002--dnf-quality-and-ties), [FIX-005](correctness.md#fix-005--match-expression-and-trace-contracts-at-their-interfaces), [ORT-001](scalability.md#ort-001--prototype-cp-sat-for-expression-minimization) |
| [CLI diagnostics and guided expression input](../requirements/cli.md#open-decisions), [trace errors and file handling](../requirements/traces.md#trace-d03--validation-and-write-errors), [report destination on refusal](../requirements/reports.md#open-decisions) | [DEC-003](correctness.md#dec-003--cli-errors-and-file-safety), [FIX-002](correctness.md#fix-002--align-guided-choices-and-help), [FIX-003](correctness.md#fix-003--report-cli-errors-and-protect-output-files) |
| [SPL declaration and lexical rules](../requirements/spl.md#open-decisions), [trace API edge cases](../requirements/traces.md#deviations-and-open-decisions), [arithmetic boundaries](../requirements/math.md#open-decisions) | [DEC-004](correctness.md#dec-004--remaining-language-and-library-edge-cases), [FIX-001](correctness.md#fix-001--align-spl-validation-and-grammar), [FIX-005](correctness.md#fix-005--match-expression-and-trace-contracts-at-their-interfaces), [FIX-006](correctness.md#fix-006--apply-confirmed-arithmetic-boundary-behavior) |
| [Supported counts](../requirements/feature-space.md#feature-d02--supported-counts) and [report admission](../requirements/planning.md#open-decisions) | [DEC-005](correctness.md#dec-005--report-scope-and-admission), [FIX-002](correctness.md#fix-002--align-guided-choices-and-help), [FIX-003](correctness.md#fix-003--report-cli-errors-and-protect-output-files) |
| [Stale CNF and trace tests](../requirements/verification.md#known-discrepancies) | [TST-001](verification.md#tst-001--restore-agreed-dnf-and-repeated-trace-key-expectations) |
| [Guided category menu](../requirements/cli.md#known-discrepancies) | [FIX-002](correctness.md#fix-002--align-guided-choices-and-help) |
| [ArgoUML summary overflow](../requirements/reports.md#known-discrepancies) | [SCALE-001](scalability.md#scale-001--compute-the-exact-argouml-summary) |
