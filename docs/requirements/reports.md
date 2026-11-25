# Analysis reports

An analysis report describes the input SPL, its derived feature space, and the requested isolation results. The [CLI](cli.md) and [planner](planning.md) determine the approved request. See [system and feature semantics](feature-space.md) for membership, IDs, and categories.

## REPORT-001 — Report modes and sections

When an approved report is written, the report shall place the sections for its mode in the order below and enclose the section titles in `===` markers.

| Mode | Sections in output order |
| --- | --- |
| `summary` | `SPL SPECIFICATION`, `THEORY SUMMARY` with model totals and category counts |
| `full` | `SPL SPECIFICATION`, `SYSTEMS`, `FEATURES BY CATEGORY`, `FEATURES BY INDEX`, followed by `ISOLATION RESULTS` when enabled |
| `isolation-only` | `SPL SPECIFICATION`, `ISOLATION RESULTS` |

While the report mode is `summary`, the two summary sections shall be the entire report, regardless of term or isolation flags. When an isolation-only report is requested with isolation disabled, the report shall retain the isolation section and print `Isolation output disabled by the approved report request.` in it. If the caller supplies `--mode=selected-features`, then the CLI shall reject that mode.

**Scenario — Summary with detail options enabled**

- **Given** the [missing-system implication SPL](../../spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl)
- **When** the caller runs `analyze` with `--mode=summary --raw-terms=true --isolation=true`
- **Then** the output contains the specification and theory summary sections in that order, with no feature tables or isolation section

[Report modes](../../src/feature_isolation.cpp#L2259) · [Mode parsing](../../src/feature_isolation.cpp#L623) · [Summary regression](../../src/cli_regression_test.cpp#L394)

## REPORT-002 — Specification and tabular format

When a report is written, the specification section shall reproduce the SPL name, fundamental features, constraints, availability declarations, and original category hint when present, in that order. The availability declarations shall retain the one-based SPL IDs. The report shall omit a separate DSL coverage section.

The report shall separate table columns with tabs by default and label systems and features using paper-style names such as `s0` and `t0`. The numeric `SYSTEM ID` column shall retain the one-based SPL ID.

**Scenario — Missing-system notation**

- **Given** the implication SPL with `MISSING_SYSTEMS 1 END`
- **When** the caller generates a full report
- **Then** the specification section lists missing-system ID `1` and the system tables show numeric ID `1` with paper label `s0`

[Specification serialization](../../src/spl.cpp#L2088) · [Paper labels](../../include/feature_analysis.hpp#L469) · [Tab separator](../../src/feature_analysis.cpp#L3317) · [Coverage regression](../../src/cli_regression_test.cpp#L423)

## REPORT-003 — Totals and category selection

When a report includes model totals, the report shall show the counts of fundamental features, valid systems, available systems, missing systems, and derived features across the entire model. When a report includes category counts, the report shall show the exact count for each included category, including zero counts. Category counts and grouped tables shall use the order `Impossible`, `Core`, `Fundamental`, `Not`, `And`, `Or`, `Complex`.

When a category selection applies, the report shall restrict the category count table, both feature tables, and isolation targets to that selection and preserve feature IDs. The report shall use the CLI category selection when supplied, otherwise the SPL category hint when present, otherwise all categories. The system tables shall retain membership and counts across all categories.

**Scenario — Core-only summary**

- **Given** the implication SPL with two fundamental features, three valid systems, and one missing system
- **When** the caller requests `--mode=summary --categories=core`
- **Then** the `THEORY` row contains `2`, `3`, `2`, `1`, and `8`, and the category table contains one row for `Core` with count `1`

[Request resolution](../../src/feature_isolation.cpp#L1739) · [Model totals](../../src/feature_isolation.cpp#L2157) · [Category counts](../../src/feature_analysis.cpp#L2749) · [Stable ID test](../../src/analysis_test.cpp#L269)

## REPORT-004 — Systems section

While the report mode is `full`, the `SYSTEMS` section shall show model totals, category counts, and both system tables. Each system table shall start with `SYSTEM ID`, `SYSTEM`, and `STATUS` and include one row per valid system in ascending ID order, with status `available` or `missing`. Each table shall expose all feature categories.

| Table | Value in each category column | Other column |
| --- | --- | --- |
| System statistics | Number of derived features present in the system | `T`, the total number of derived features present |
| System feature membership | Ascending set of paper-style feature labels or `∅` | |

If the valid system space is empty, then both system tables shall show their headers with zero data rows.

**Scenario — Expose all valid systems in a filtered report**

- **Given** the implication SPL with availability statuses `s0` missing and `s2` and `s3` available
- **When** the caller requests a full report with `--categories=core`
- **Then** each system table contains `s0`, `s2`, and `s3` in that order, and the `Fundamental` statistics for `s3` still show `2`

[Systems section](../../src/feature_analysis.cpp#L2895) · [Statistics](../../src/feature_analysis.cpp#L2813) · [Membership](../../src/feature_analysis.cpp#L2879) · [Missing-system test](../../src/analysis_test.cpp#L203)

## REPORT-005 — Feature tables

While the report mode is `full`, both feature tables shall contain the same selected features. `FEATURES BY CATEGORY` shall begin with `CATEGORY`, `i`, and `FEATURE`, group by category, and order each group's rows by ascending feature ID. `FEATURES BY INDEX` shall begin with `i`, `FEATURE`, and `CATEGORY` and order its rows by ascending feature ID. In both tables, `i` shall be the zero-based index and `FEATURE` shall be the corresponding `t` label.

Both tables shall follow those columns with `t_i`, the ascending set of paper-style system labels or `∅`, and `β(t_i)`, a binary presence string in descending valid system ID order. Both columns shall include membership for all valid systems, including missing systems.

**Scenario — Feature membership includes missing systems**

- **Given** the implication SPL with valid systems `s0`, `s2`, and `s3`, where `s0` is missing
- **When** the caller generates a full report
- **Then** both feature tables show `t1` with set `{s3}` and presence `100`, and `t4` with set `{s0}` and presence `001`

[Category table](../../src/feature_isolation.cpp#L2169) · [Index table](../../src/feature_isolation.cpp#L2218) · [Presence formatting](../../src/feature_analysis.cpp#L3327)

## REPORT-006 — Terms and isolation output

While the report mode is `full`, the report shall include both term columns and the isolation section by default. The `--simplified-terms` and `--raw-terms` flags shall independently control the `ℓ_T(t_i)(simplified)` and `ℓ_T(t_i)(raw term)` columns, with the simplified term first when both are present. When complex details are disabled, the report shall leave term values blank for selected complex features. When `--isolation=false` is supplied, the full report shall omit the isolation section.

While the report mode is `isolation-only`, the request shall default to raw terms, simplified terms, and complex details disabled. When an explicit term flag is enabled, names for selected non-complex features shall use the corresponding term when available. When isolation rows are written, the report shall choose the simplified term, raw term, or membership string for each display name in that order of preference. It shall always show the target name and include candidate names when requested by `-feature-names`. The `-min-diffs` flag shall select the witness mode. See [isolation requirements](isolation.md) for the output contract.

**Scenario — Simplified terms without raw terms or isolation**

- **Given** the implication SPL
- **When** the caller requests `--mode=full --raw-terms=false --isolation=false`
- **Then** both feature tables include the simplified term column and omit the raw term column, and the report omits the isolation section

[Mode defaults](../../src/feature_isolation.cpp#L1700) · [Detail controls](../../src/feature_analysis.cpp#L3057) · [Isolation section](../../src/feature_isolation.cpp#L2272) · [Names and rows](../../include/isolation.hpp#L788)

## REPORT-007 — Output destination

When `analyze` completes an approved summary, full, or isolation-only request, the command shall write the report to the supplied output path and return a successful status. If the command cannot open the SPL input, then it shall return status `2` without creating or changing the report file. If the command cannot open the supplied report destination, then it shall return status `3`.

**Scenario — A summary replaces the previous report**

- **Given** a writable file containing a previous report and the implication SPL
- **When** the caller runs `analyze` with the supplied file as the destination and mode `summary`
- **Then** the command exits with status `0` and the file contains the new summary

[Analyze execution](../../src/feature_isolation.cpp#L2668) · [Writer dispatch](../../src/feature_isolation.cpp#L2314)

## REPORT-008 — Exact ArgoUML summary

When the caller requests an approved summary with all categories for the checked-in ArgoUML fixture, the CLI shall complete successfully and write exact system, derived-feature, and category counts.

**Scenario — Summarize all ArgoUML features**

- **Given** the [ArgoUML fixture](../../spl_specs/new_dsl/valid/argo_uml_test.spl) with eight unconstrained fundamental features
- **When** the caller runs `analyze` with `--mode=summary --yes --non-interactive` and no category filter
- **Then** the CLI exits with status `0` and the summary reports 8 fundamental features, 256 valid systems, 256 available systems, 0 missing systems, and exactly $2^{256}$ derived features
- **And** the summary contains all seven exact category counts below

| Category | Exact count |
| --- | ---: |
| `Impossible` | 1 |
| `Core` | 1 |
| `Fundamental` | 8 |
| `Not` | 8 |
| `And` | 6,544 |
| `Or` | 6,544 |
| `Complex` | $2^{256} - 13{,}106$ |

The counts follow from the fixture and the [derived feature and category rules](feature-space.md#feature-006--observable-categories).

## Verification example

```sh
./testing/generated/cli analyze spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl /tmp/feature-summary.txt -min-diffs=false -feature-names=false --mode=summary --categories=core
```

This command reproduces the core-only summary in REPORT-003. [Summary regression](../../src/cli_regression_test.cpp#L394)

## Known discrepancies

- **REPORT-D03 — ArgoUML summary overflow.** The current CLI aborts with an integer overflow when asked for the summary in REPORT-008. [Current failure site](../../src/feature_analysis.cpp#L3088)

## Scope decision

- **REPORT-D01 Category filter scope.** The target preserves model-wide system tables and restricts the category counts, feature tables, and isolation targets. See [REPORT-003](#report-003--totals-and-category-selection) and [REPORT-004](#report-004--systems-section).

## Open decisions

- **REPORT-D02 Output on refusal.** The baseline creates or truncates the destination before SPL parsing and report approval. A refusal can leave an empty output file. Whether a failed or refused report should preserve the destination needs a decision. [Analyze execution](../../src/feature_isolation.cpp#L2668) · [CLI open decisions](cli.md#open-decisions)
