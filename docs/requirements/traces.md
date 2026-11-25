# Trace export requirements

Trace export writes a grade-bounded lookup of Boolean expressions by their available-system IDs. Callers can also request an on-demand [expression](expressions.md).

## TRACE-001 — Repeated trace keys

When several trace entries have the same available-system key, the trace exporter shall write and count each entry on its own line. This includes expressions that agree on the available systems but differ on a missing system. A literal true in all valid systems can also share the core entry's key. [Output projection](../../src/feature_isolation.cpp#L2995) · [TRACE-D01](#trace-d01--key-multiplicity)

**Scenario — A missing system hides a difference**

- **Given** the [missing-system fixture](../../spl_specs/new_dsl/valid/missing_systems_example.spl)
- **When** the caller exports simple traces up to grade 1
- **Then** the file contains both `2,3: ⊤` and `2,3: fy` and the reported row count includes both.

## TRACE-002 — Command inputs and defaults

The trace CLI shall accept the command form below. The input is a readable SPL specification and the output is a file path. The maximum grade is a positive integer and the trace kind is `simple` or `complex`, in lowercase.

```text
cli traces <spl-file> <output-file> [--max-grade=N] [--trace-kind=simple|complex]
```

When standard input is noninteractive, the trace CLI shall require a grade and shall default an omitted kind to `simple`. When valid options are repeated, the trace CLI shall use the last value for each option.

When invoked as `traces --help` or `--help traces`, the trace CLI shall print trace help. Where the SPL declares `CATEGORY_FILTER`, the exporter shall produce the same traces as for the equivalent SPL with that filter omitted. Report-planning flags apply to report commands. [Argument parser](../../src/feature_isolation.cpp#L1465) · [Grade validation](../../src/feature_isolation.cpp#L588) · [Noninteractive defaults](../../src/feature_isolation.cpp#L2880) · [Help dispatch](../../src/feature_isolation.cpp#L3050)

**Scenario — Default to simple traces with redirected input**

- **Given** the [two-feature fixture](../../spl_specs/new_dsl/valid/unconstrained_2f.spl) and standard input redirected from `/dev/null`
- **When** the caller exports traces with `--max-grade=2` and omits the trace kind
- **Then** the file is identical to an export with `--trace-kind=simple`.

## TRACE-003 — Grade and formula family

When a caller requests simple traces, the trace exporter shall include eligible single literals, conjunctions, and disjunctions through the requested grade. Where the caller selects complex traces, it shall additionally include eligible combinations of conjunction and disjunction from grade 3 through the requested grade. Complex requests are limited to grade 5. Simple grades above the number of fundamental features add no candidates.

A grade is the count of distinct fundamental features in the formula. Eligible formulas use a single polarity per feature. A complex formula may repeat a feature with that polarity in multiple terms. [Trace type and grade](../../include/feature_analysis.hpp#L611) · [Complex formula behavior](../../src/feature_analysis.cpp#L1331) · [Grade cap](../../src/feature_isolation.cpp#L2931) · [Possible grades](../../src/feature_analysis.cpp#L2414)

**Scenario — Include mixed formulas at grade three**

- **Given** the [unconstrained three-feature fixture](../../spl_specs/new_dsl/valid/factorial_three_features.spl)
- **When** the caller exports complex traces up to grade 3
- **Then** the file contains `1,3,5,6,7: Logging ∨ (Checked ∧ Iterative)`, which is absent from the simple grade-3 export.

## TRACE-004 — Valid-system membership and representative expressions

When eligible expressions have identical membership over all valid systems, including missing systems, the trace exporter shall retain one ordinary representative. It shall prefer lower grade, then shorter rendered text in bytes, then the lexicographically earlier label.

The trace API shall include a separate grade-0 entry for the core tautology `⊤`. [Representative preference](../../src/feature_analysis.cpp#L947) · [Core entry](../../src/feature_analysis.cpp#L2385)

**Scenario — Select the shorter representative**

- **Given** `FEATURES 2 long_name a` with `ASSERT long_name <-> a`
- **When** the caller exports simple traces up to grade 1
- **Then** the file labels key `3` as `a` and contains a single ordinary entry for that key.

## TRACE-005 — File format and order

When a trace matches available systems, the trace exporter shall write one headerless UTF-8 line in the form `<ids>: <expression>` followed by a newline. The key shall list only the matching available systems as ascending zero-based IDs separated by commas without spaces. If a trace matches zero available systems, then the trace exporter shall omit it. This rule also applies to the core.

When writing rows, the trace exporter shall order them by ascending grade, then lexicographic expression order. The core comes first if it has a key. Labels shall use `¬`, `∧`, and `∨`. Complex expressions are disjunctions of terms, with parentheses around conjunctions. Within each term, positive literals precede negated literals, and each polarity group is sorted by name.

When export succeeds, the trace CLI shall replace the designated file, exit with status `0`, and report the candidate and final row counts on standard error. [Trace help](../../src/feature_isolation.cpp#L472) · [Output writer](../../src/feature_isolation.cpp#L2995) · [Literal rendering](../../src/feature_analysis.cpp#L897) · [Complex labels](../../src/feature_analysis.cpp#L1427) · [Trace ordering](../../src/feature_analysis.cpp#L2396) · [Status message](../../src/feature_isolation.cpp#L3036)

**Scenario outline — Project the core to availability**

- **Given** an unconstrained SPL with `FEATURES 2 fx fy` and the availability in a row below
- **When** the caller exports grade-1 traces
- **Then** the file has the content in that row.

| Availability | File content |
| --- | --- |
| All four systems are available | The first line is `0,1,2,3: ⊤`. |
| No system is available because the SPL declares `AVAILABLE_SYSTEMS END` | The file is empty and the reported count is zero. |

## TRACE-006 — Interactive choices and candidate counts

When the trace CLI runs with terminal input and only one or neither option supplied, it shall display a candidate-count table and prompt for each omitted value. The trace-kind prompt shall default to `simple`.

If a complex request exceeds grade 5, then the interactive trace CLI shall prompt for a new grade and the noninteractive trace CLI shall exit with status `4`. When the interactive request is for complex grade 5, the trace CLI shall request confirmation with a default answer of no. A noninteractive request at grade 5 proceeds without a prompt. If a caller declines confirmation or closes input during a required prompt, then the trace CLI shall exit with status `4`. [Candidate table](../../src/feature_isolation.cpp#L2798) · [Prompts and confirmation](../../src/feature_isolation.cpp#L2885)

When presenting candidate counts, the trace CLI shall count formulas before merging equivalent valid-system memberships and omitting empty available-system keys. For a subset of `k` fundamental features, each count shall use the corresponding entry below. The count for a grade covers every subset of that size. The total reported before export includes the one core candidate. [Count contract](../../include/feature_analysis.hpp#L632) · [Candidate counts](../../src/feature_analysis.cpp#L2414) · [CLI totals](../../src/feature_isolation.cpp#L2948)

| Grade `k` | Simple | Complex |
| --- | ---: | ---: |
| 1 | 2 | 2 |
| 2 | 8 | 8 |
| 3 | 16 | 72 |
| 4 | 32 | 1,824 |
| 5 | 64 | 220,608 |

**Scenario — Candidate count includes the core**

- **Given** the two-feature fixture and a terminal session
- **When** the caller omits both options, enters grade `2`, and accepts the default trace kind
- **Then** the trace CLI displays the candidate table, uses simple traces, and reports 13 candidate formulas and 13 written rows.

## TRACE-007 — Error behavior

If the input or output path cannot be opened, then the trace CLI shall report a diagnostic and exit with status `2`. If the output path is omitted, an option or kind is unknown, the grade is invalid, or a noninteractive invocation omits the grade, then the trace CLI shall report a usage or option error and exit with status `4`.

If the SPL is invalid or the export fails while writing the destination, then the trace CLI shall report the failure and return a nonzero exit status. These cases have no fixed exit status in this specification. [Argument validation](../../src/feature_isolation.cpp#L1465) · [File handling](../../src/feature_isolation.cpp#L2862) · [TRACE-D03](#trace-d03--validation-and-write-errors)

**Scenario outline — Trace invocation failures**

- **Given** a noninteractive trace invocation and the condition in a row below
- **When** the caller invokes `traces`
- **Then** the CLI reports the error and returns the stated status.

| Cause | Expected status |
| --- | --- |
| The specified SPL path does not exist | `2` |
| The destination's parent directory does not exist | `2` |
| The grade is `0` | `4` |
| The grade is omitted | `4` |
| The kind is `Complex` | `4` |

## TRACE-008 — C++ trace API

When `enumerate_bounded_traces` receives a feature count, ordered one-based valid system IDs, feature names, and maximum grade, it shall return trace labels, presence vectors in the supplied system order, categories, and grades. The list shall include the grade-0 core and be ordered by grade, then label. The kind defaults to `simple` and complex calls take a maximum grade at most 5. Availability affects file keys under [TRACE-005](#trace-005--file-format-and-order). [Bounded trace interface](../../include/feature_analysis.hpp#L648) · [Return value and order](../../src/feature_analysis.cpp#L2365)

When `estimated_trace_count` receives an exact positive grade within the feature count, it shall report that grade's candidate count, saturating at the largest representable value if the count overflows. If the grade is `0` or exceeds the feature count, then it shall return zero. Complex estimates have a precondition of grade at most 5. [Count interface](../../include/feature_analysis.hpp#L632) · [Count result](../../src/feature_analysis.cpp#L2414)

**Scenario — Respect system order in an API result**

- **Given** two fundamental features, `fx` and `fy`, and valid IDs supplied as `4, 3, 2, 1`
- **When** the caller requests traces through grade 1 using the default kind
- **Then** the grade-0 core has presence vector `[true, true, true, true]`, and `fx` has grade 1 and presence vector `[true, false, true, false]`.

## Verification examples

Run after `make cli`. These invocations use noninteractive input and replace their respective destination files.

```sh
./testing/generated/cli traces spl_specs/new_dsl/valid/unconstrained_2f.spl testing/generated/two-feature-traces.txt --max-grade=2 </dev/null
./testing/generated/cli traces spl_specs/new_dsl/valid/missing_systems_example.spl testing/generated/missing-system-traces.txt --max-grade=1 </dev/null
./testing/generated/cli traces spl_specs/new_dsl/valid/factorial_three_features.spl testing/generated/complex-traces.txt --max-grade=3 --trace-kind=complex </dev/null
```

| File | Checks |
| --- | --- |
| `two-feature-traces.txt` | 13 rows. Starts with `0,1,2,3: ⊤` and includes `3: fx ∧ fy` and `1,2,3: fx ∨ fy`. |
| `missing-system-traces.txt` | Four rows in order, `2,3: ⊤`, `3: fx`, `2,3: fy`, `2: ¬fx`. |
| `complex-traces.txt` | 103 rows. Includes `1,3,5,6,7: Logging ∨ (Checked ∧ Iterative)`. |

[Two-feature regression](../../src/cli_regression_test.cpp#L591) · [Missing-system fixture](../../spl_specs/new_dsl/valid/missing_systems_example.spl) · [Three-feature fixture](../../spl_specs/new_dsl/valid/factorial_three_features.spl)

## Deviations and open decisions

### TRACE-D01 — Key multiplicity

**Test discrepancy.** The older missing-system regression expects `fy` to be absent when it shares the core key. The current exporter implements [TRACE-001](#trace-001--repeated-trace-keys) and writes both rows. The preference a consumer uses to choose among rows with the same key is unspecified. [Output writer](../../src/feature_isolation.cpp#L3027) · [Conflicting regression](../../src/cli_regression_test.cpp#L618)

### TRACE-D02 — Tautology and empty keys

**Help discrepancy.** Trace help says the core is always included and also says that empty keys are omitted. The core is always present in the API result under [TRACE-008](#trace-008--c-trace-api). The file contains zero lines if no trace has an available match, as required by [TRACE-005](#trace-005--file-format-and-order). [Help text](../../src/feature_isolation.cpp#L472) · [Empty-key handling](../../src/feature_isolation.cpp#L3011)

### TRACE-D03 — Validation and write errors

**Code deviation.** The grade conversion can accept a negative spelling as a large unsigned value. The direct command can terminate abnormally on SPL parse failure or for a parser-default feature count without feature names. The file writer can report success despite an error after a successful open. [Grade conversion](../../src/feature_isolation.cpp#L588) · [SPL loading](../../src/feature_isolation.cpp#L2862) · [Feature name lookup](../../src/feature_analysis.cpp#L1043) · [Final output status](../../src/feature_isolation.cpp#L3033)

**Open decision.** Define valid grade spellings, how the C++ trace API should respond to too few feature names, and exact statuses for parse and post-open write failures. The CLI follows [SPL-001](spl.md#spl-001--optional-name-and-feature-basis) for missing feature declarations. Decide whether input and output may use the same path. The current CLI accepts that path pair and can overwrite the source. [Positional arguments](../../src/feature_isolation.cpp#L1477) · [Export path](../../src/feature_isolation.cpp#L2974)

### TRACE-D04 — Resource expectations

Trace export has a specified complex-grade limit and reports candidate counts. Its execution-time and peak-memory limits are unspecified. [Interactive cost indication](../../src/feature_isolation.cpp#L2948)
