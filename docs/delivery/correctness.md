# M2 — Resolve and satisfy behavior contracts

Close the documented differences between requirements and observed behavior. Agreed cases can enter test-driven development while open choices are reviewed. For each resolved choice, update the EARS statement and its Given/When/Then example, write the failing acceptance test, then change the behavior.

## Decisions

### DEC-001 — Isolation and missing systems

- **Status** Ready
- **Depends on** None
- **Scope** Decide the candidate set, result label, and witnesses when the target occurs in no available system, including when every system is missing. Define minimality for witness pairs with incomparable sides and the outcome for an unknown target ID. Identify the referenced theory source or record the agreed rule with a fixture if the source is unavailable. [ISO-D01–D03](../requirements/isolation.md#open-decisions) · [REQ-D01](../requirements/README.md#req-d01--theory-reference)
- **Done when** The specification states the expected candidate, witness, and label outcomes for the missing-system implication fixture and the all-missing case, plus the expected results for incomparable witnesses and unknown IDs.

### DEC-002 — DNF quality and ties

- **Status** Ready
- **Depends on** None
- **Scope** Decide whether expression lookup and complex report terms require a globally minimum DNF under the published objective order and whether equally good expressions need identical text. DNF and truth values over the valid systems are already specified. [EXPR-D01](../requirements/expressions.md#expr-d01--normal-form-and-optimality) · [FEATURE-D01](../requirements/feature-space.md#feature-d01--complex-expression-contract)
- **Done when** Examples state how to judge a truth-equivalent expression with a better known literal count and two expressions that tie, including which text or result comparisons the tests can require.

### DEC-003 — CLI errors and file safety

- **Status** Ready
- **Depends on** None
- **Scope** Decide the accepted numeric spellings and exact statuses for malformed or missing arguments, SPL parse errors, and write failures. Decide how refusal, cancellation, and failure affect existing destinations, whether input and output can refer to the same file, and whether guided `expr` asks for IDs and has dedicated help. [CLI-D01–D03](../requirements/cli.md#open-decisions) · [EXPR-D02](../requirements/expressions.md#expr-d02--invalid-arguments-and-guided-input) · [TRACE-D03](../requirements/traces.md#trace-d03--validation-and-write-errors) · [REPORT-D02](../requirements/reports.md#open-decisions)
- **Done when** The affected pages specify the exit status, diagnostic channel, and file effect for concrete failure and cancellation cases, plus the supported guided expression flow.

### DEC-004 — Remaining language and library edge cases

- **Status** Ready
- **Depends on** None
- **Scope** Decide how SPL handles duplicate feature names and stray punctuation, whether the trace C++ API rejects too few feature names, and whether a consumer needs an agreed way to pick one of several rows with the same trace key. Decide whether combination initialization resets an existing selection and which mathematically representable arithmetic counts the helper API must return. [SPL-D01–D02](../requirements/spl.md#open-decisions) · [TRACE-D01](../requirements/traces.md#trace-d01--key-multiplicity) · [TRACE-D03](../requirements/traces.md#trace-d03--validation-and-write-errors) · [MATH-D01–D02](../requirements/math.md#open-decisions)
- **Done when** Each case has an agreed observable result or an explicitly unspecified result. The exporter's existing guarantee to write each qualifying trace row remains the acceptance rule.

### DEC-005 — Report scope and admission

- **Status** Ready
- **Depends on** None
- **Scope** Decide how exhaustive requests beyond the supported scope are declined, how unrepresentable counts are reported, whether the CLI ever enforces limits during generation, and how `--accept-recommended` behaves for a safe request with an alternative recommendation. Exact ArgoUML summary counts are already required. Measured execution-time and peak-memory limits are unspecified. [FEATURE-D02](../requirements/feature-space.md#feature-d02--supported-counts) · [PLAN-D01–D02](../requirements/planning.md#open-decisions) · [REPORT-008](../requirements/reports.md#report-008--exact-argouml-summary)
- **Done when** Examples define an accepted request and an oversized request, with the outcome and any suggestion displayed to the caller, and specify the decision for safe requests with a recommendation.

## Implementation

### FIX-001 — Align SPL validation and grammar

- **Status** Planned
- **Depends on** [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts) for reusable test coverage. Apply [DEC-004](#dec-004--remaining-language-and-library-edge-cases) to duplicate names and punctuation once resolved.
- **Scope** Reject missing `FEATURES` and repeated top-level sections as specified. Align the grammar and authoring examples with the published lexical and section rules, including optional `END` for availability lists and `COUNT` equality. [SPL-001–003](../requirements/spl.md#file-and-section-rules) · [SPL-006](../requirements/spl.md#spl-006--cardinality-formulas)
- **Done when** Valid fixtures still parse, the missing and repeated section cases fail with a diagnostic, and the grammar and examples agree with the exercised contract.

### FIX-002 — Align guided choices and help

- **Status** Planned
- **Depends on** [TST-002](verification.md#tst-002--isolate-cli-test-runs) and [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts) for the CLI harness and workflow coverage. Apply [DEC-003](#dec-003--cli-errors-and-file-safety) to guided `expr` and [DEC-005](#dec-005--report-scope-and-admission) to report recommendation behavior once resolved.
- **Scope** Make every published guided category choice dispatch the expected request, including entering manual selection. Keep help and actual dispatch aligned. Preserve documented direct options and report choices. Implement the resolved behavior for a supported safe request with an alternative recommendation. [CLI-001–005](../requirements/cli.md#commands) · [CLI-D04](../requirements/cli.md#known-discrepancies) · [PLAN-D02](../requirements/planning.md#open-decisions)
- **Done when** End-to-end tests select every advertised category workflow and observe the corresponding output or prompt. The decided expression and recommendation cases have passing CLI examples.

### FIX-003 — Report CLI errors and protect output files

- **Status** Planned
- **Depends on** [TST-002](verification.md#tst-002--isolate-cli-test-runs) and [TST-004](verification.md#tst-004--verify-cli-planning-and-report-contracts). The choice of exact statuses and destination rules depends on [DEC-003](#dec-003--cli-errors-and-file-safety). Oversized request handling follows [DEC-005](#dec-005--report-scope-and-admission).
- **Scope** Route malformed input and output failures through the documented diagnostic and exit status contract, including paths that currently terminate abnormally. Apply the chosen file behavior on failure, refusal, cancellation, and an input/output path collision. Apply the agreed admission or enforcement policy to oversized requests. [CLI-006–007](../requirements/cli.md#results-and-failures) · [EXPR-005](../requirements/expressions.md#expr-005--failure-and-help-behavior) · [TRACE-007](../requirements/traces.md#trace-007--error-behavior) · [REPORT-007](../requirements/reports.md#report-007--output-destination)
- **Done when** Real CLI processes exercise invalid SPL, malformed arguments, denied and oversized requests, destination collisions, and post-open write failures. Tests check exit status, stderr, and destination contents. The already-specified diagnostic and unsuccessful exit cases can be fixed before exact new codes are decided.

### FIX-004 — Align isolation candidates and witnesses

- **Status** Planned
- **Depends on** [DEC-001](#dec-001--isolation-and-missing-systems) and [TST-006](verification.md#tst-006--verify-isolation-candidates-and-witnesses)
- **Scope** Implement the agreed outcome for absent targets, all-missing systems, unknown IDs, and minimal witness pairs. Preserve candidate membership and report labels for cases already specified. [Isolation contract](../requirements/isolation.md)
- **Done when** Public tests compare missing-system results with the agreed fixture expectations, check witnesses by the agreed minimality rule, and assert report labels and rows. The affected theory cases pass.

### FIX-005 — Match expression and trace contracts at their interfaces

- **Status** Planned
- **Depends on** [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table). Apply [DEC-002](#dec-002--dnf-quality-and-ties) to objective and tie cases and [DEC-004](#dec-004--remaining-language-and-library-edge-cases) to trace C++ input and consumer behavior once resolved.
- **Scope** Address expression and trace output gaps found by the contract tests. Apply the chosen quality guarantee in the default expression workflow and align trace CLI help with the API and file behavior for the core and empty matches. Preserve DNF, valid-system semantics, and repeated keys. [Expression contract](../requirements/expressions.md) · [Trace contract](../requirements/traces.md) · [TRACE-D02](../requirements/traces.md#trace-d02--tautology-and-empty-keys)
- **Done when** A bounded truth-table oracle confirms `expr` and trace terms on valid systems. Tests cover the decided objective or tie rules and API versus file behavior for no available matches.

### FIX-006 — Apply confirmed arithmetic boundary behavior

- **Status** Planned
- **Depends on** [DEC-004](#dec-004--remaining-language-and-library-edge-cases) and [TST-003](verification.md#tst-003--verify-spl-arithmetic-and-feature-space-contracts)
- **Scope** Update arithmetic or selection behavior only where the agreed boundary cases require it. Keep the existing public count and exception behavior covered. [MATH-001–003](../requirements/math.md)
- **Done when** Public API tests cover the agreed reset and count boundaries. If the current outcomes are accepted, those tests and requirements record the decision.
