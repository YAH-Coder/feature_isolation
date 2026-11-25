# Planning and report admission

The planner describes report cost and scope. Report admission determines whether to generate the requested or selected report.

## PLAN-001 — Planning output

When a user runs `plan` for a valid SPL and report request, the CLI shall print the requested and recommended plans, alternatives, and applicable drill-down commands to standard output without writing a report file.

When the CLI displays a requested or recommended plan, it shall show the fundamental-feature and valid-system counts, total derived-feature count or its saturated value, bounds on included and complex features, estimated runtime and output size, risk, and whether the estimates fit the configured thresholds. When the request is a summary, the planner shall show an included-feature upper bound of zero.

**Scenario — Plan a two-feature report**

- **Given** the unconstrained two-feature SPL.
- **When** the user runs `cli plan spl_specs/new_dsl/valid/unconstrained_2f.spl`.
- **Then** the CLI reports two fundamental features, four valid systems, and 16 derived features.
- **And** it labels the requested plan `safe`, recommends keeping it, and writes no report.

[Fixture](../../spl_specs/new_dsl/valid/unconstrained_2f.spl) · [Plan output regression](../../src/cli_regression_test.cpp#L378) · [Feasibility summary](../../src/feature_analysis.cpp#L535)

## PLAN-002 — Risk labels

When a report has a feasibility estimate, the planner shall assign the most severe applicable risk label in the following table. Time and output comparisons use the estimates. The risk classification shall remain independent of user overrides for admission thresholds.

| Risk | Conditions |
| --- | --- |
| `impractical` | The derived count saturates, estimated runtime exceeds 5 minutes, or estimated output exceeds 1 GiB. |
| `risky` | Estimated runtime exceeds 2 minutes, estimated output exceeds 256 MiB, or the derived count exceeds 2²⁴. |
| `large` | Estimated runtime exceeds 15 seconds, estimated output exceeds 64 MiB, or the derived count exceeds 2²⁰. |
| `safe` | The estimate meets none of the preceding conditions. |

**Scenario outline — Classify by estimated time**

- **Given** an estimate of 16 derived features and 2 KiB of output.
- **When** the planner assigns risk for the estimated runtime below.
- **Then** the assigned label matches the table.

| Estimated runtime | Risk |
| --- | --- |
| 15 s | `safe` |
| 16 s | `large` |
| 121 s | `risky` |
| 301 s | `impractical` |

[Risk thresholds](../../src/feature_analysis.cpp#L431) · [Feasibility tests](../../src/analysis_test.cpp#L380)

## PLAN-003 — Recommendations and drill-down

When the planner presents a recommendation, it shall include its rationale and retain the requested scope as an alternative.

When the request is safe and there is no uncertainty about complex-feature cost, the planner shall recommend the requested scope. When the request is expensive or the complex cost is uncertain, the planner shall consider applicable alternatives that omit complex details, omit complex and isolation details, or provide a summary. It shall prefer lower risk and normally prefer lower estimated runtime at the same risk. With uncertain complex cost, it can retain the original mode with complex details omitted over a different mode at the same risk.

When the planner offers follow-up options, it shall distinguish executable scopes from suggestions requiring a separate change. Executable drill-downs can include a summary, all non-complex categories, fundamental and negated fundamental features, or isolation only. Selected feature IDs and tighter input constraints shall appear as suggestions outside the executable scopes.

**Scenario — Recommend a lighter report**

- **Given** an unconstrained four-feature SPL and the default full report request.
- **When** the user runs `plan`.
- **Then** the CLI recommends a summary and explains the adjustment.
- **And** it keeps the original request as an option and identifies the selected-feature suggestion as unavailable for direct execution.

[Recommendations](../../src/feature_analysis.cpp#L658) · [Recommendation tests](../../src/analysis_test.cpp#L543) · [Drill-down choices](../../src/feature_isolation.cpp#L1911)

## PLAN-004 — Report approval

When an `analyze` request is safe, the CLI shall evaluate the requested scope against the admission thresholds without requiring approval.

When approval is required, the CLI shall retain the requested scope for `--yes` and adopt the recommended scope for `--accept-recommended`. While standard input is a terminal and neither flag is supplied, when approval is required, the CLI shall show the requested, recommended, and other executable choices with the recommendation as the default.

If approval is required and the user cancels, input closes, or the noninteractive request supplies no approval flag, then the CLI shall report the decision and exit with status `6`.

**Scenario outline — Approve an expensive request noninteractively**

- **Given** an unconstrained four-feature SPL whose full request needs approval and whose summary fits admission thresholds.
- **When** the user invokes `analyze --non-interactive` with the policy below.
- **Then** the approval step produces the corresponding result.

| Additional flag | Result |
| --- | --- |
| None | Refuses approval with exit status `6`. |
| `--yes` | Sends the full request to the admission threshold check. |
| `--accept-recommended` | Sends the recommended summary to the admission threshold check. |

[Approval choices](../../src/feature_isolation.cpp#L2444) · [Approval and guardrail regression](../../src/cli_regression_test.cpp#L454)

## PLAN-005 — Guardrail checks

The CLI shall default to estimated admission thresholds of 300,000 milliseconds and 1,073,741,824 bytes. When `plan` displays a scope, the CLI shall indicate whether either estimate strictly exceeds its effective threshold.

When `analyze` reaches admission, the CLI shall check the chosen scope against its effective thresholds. When either estimate exceeds its threshold, the CLI shall allow generation only with `--force` or explicit interactive override. Selecting the original over-limit request in the interactive approval menu qualifies as an override. A separate force prompt shall default to cancellation.

If an over-limit request reaches admission without an applicable override or the user declines the override, then `analyze` shall decline generation and return status `8`. The `--force` flag shall affect threshold admission only. The approval step in [PLAN-004](#plan-004--report-approval) still applies.

**Scenario outline — Apply a runtime threshold**

- **Given** the two-feature unconstrained model and a safe summary request with a positive estimated runtime.
- **When** a noninteractive user invokes `analyze` with `--max-runtime-ms=0` and the option below.
- **Then** the admission result matches the table.

| Additional option | Result |
| --- | --- |
| None | Declines the request with status `8`. |
| `--force` | Generates the summary with status `0`. |

These requirements concern estimated admission. See [PLAN-D01](#open-decisions) for runtime and output enforcement. [Threshold comparison](../../src/feature_isolation.cpp#L2063) · [Admission behavior](../../src/feature_isolation.cpp#L2569) · [Guardrail regression](../../src/cli_regression_test.cpp#L454)

## Open decisions

- **PLAN-D01 — Resource enforcement.** The CLI help describes hard run-time and output limits. The current checks govern admission using estimates, and generation can exceed the estimates. Whether the intended behavior includes limits enforced during generation is unresolved. [Help](../../src/feature_isolation.cpp#L307) · [Report admission](../../src/feature_isolation.cpp#L2711)
- **PLAN-D02 — Recommendation for safe requests.** Decide whether `--accept-recommended` should apply when a safe request has an alternative recommendation. The current approval flow retains the requested scope for safe requests. [Approval flow](../../src/feature_isolation.cpp#L2444)
