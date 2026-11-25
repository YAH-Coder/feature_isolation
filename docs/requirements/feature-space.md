# Valid systems and derived features

The analysis identifies systems and derived features, gives them stable identifiers, and supplies their membership and descriptions. The [SPL input requirements](./spl.md) define validity and availability. The [report requirements](./reports.md) define which details appear in a report.

## Systems and feature identity

### FEATURE-001 — System identifiers and counts

When analysis receives a model with `n` fundamental features and a representable candidate count, the analysis shall return `2^n` from `system_count()` and the number of valid systems from `valid_system_count()`. Its system space shall expose the valid IDs and their disjoint available and missing subsets.

**Example — Candidate count and valid count**

- **Given** the two-feature implication fixture with `s0` marked missing.
- **When** a caller requests the counts and the system space.
- **Then** there are four candidate systems and three valid systems, of which two are available and one is missing.

[SPL numbering](./spl.md#spl-007--candidate-system-numbering) · [Count and partition test](../../src/analysis_test.cpp#L71) · [Missing-system fixture](../../spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl)

### FEATURE-002 — Complete partition required for direct analysis

When a C++ caller constructs analysis directly, the analysis shall accept a fundamental-feature count, a valid/available/missing partition, optional feature names, and optional report controls.

If a valid ID lies outside the candidate space, an available or missing ID lies outside the valid set, or the available and missing sets fail to form a disjoint partition of the valid set, then the analysis shall raise `std::invalid_argument`.

**Example — Partition validation**

- **Given** two fundamental features and valid IDs `{1, 3, 4}`.
- **When** a caller constructs analysis with the sets below.
- **Then** the construction has the listed outcome.

| Available IDs | Missing IDs | Expected outcome |
| --- | --- | --- |
| `{3, 4}` | `{1}` | Accepted. |
| `{2, 3, 4}` | `{1}` | `std::invalid_argument` because `2` is invalid. |
| `{3, 4}` | `{1, 3}` | `std::invalid_argument` because the sets overlap. |
| `{3}` | `{1}` | `std::invalid_argument` because the sets omit `4`. |

[Constructor contract](../../include/feature_analysis.hpp#L698) · [Validation](../../src/feature_analysis.cpp#L1560) · [Partition rejection test](../../src/analysis_test.cpp#L366)

### FEATURE-003 — A derived feature is identified by its support

When a model has `v` valid systems and the derived count is representable, the analysis shall expose one derived feature for each subset of those systems, for a total of `2^v`. The subset is the feature's support and may be empty or include all valid systems. Formulas with the same truth values on the valid systems shall identify the same derived feature.

When only availability changes, the analysis shall preserve derived-feature identity and count.

**Example — Availability preserves support**

- **Given** two fundamental features and the implication `fx -> fy`.
- **When** system `s0` is marked missing.
- **Then** all eight derived features keep the same supports and IDs.

[Support behavior](../../src/feature_analysis.cpp#L3087) · [Implication test](../../src/analysis_test.cpp#L234) · [Fixture counts](../../src/isolation_test.cpp#L332)

### FEATURE-004 — Derived feature numbering and bit order

When a caller reads a derived feature's presence vector, the analysis shall return membership bits in ascending order of valid system ID. Its one-based C++ feature ID shall equal the numeric value of that vector, with the first bit most significant, plus one. Its paper-style label `tN` shall use the vector's numeric value as the suffix.

When the reporter displays a presence bit string, the reporter shall order its bits by descending valid system ID. Paper-style system labels shall use the one-based system ID minus one as the suffix.

**Example — Three representations of the same feature**

- **Given** the unconstrained feature basis `fx`, `fy`.
- **When** a caller reads the derived feature for `fx` in the analysis API and a report.
- **Then** the support is `{2, 4}`, the vector is `[0, 1, 0, 1]`, the C++ ID is `6`, the report label is `t5`, and the displayed bit string is `1010`.

[Registry numbering](../../src/feature_analysis.cpp#L3118) · [Label conversion](../../include/feature_analysis.hpp#L469) · [Bit-string rendering](../../src/feature_analysis.cpp#L3327)

### FEATURE-005 — Per-system membership

Where per-system details are available, the analysis shall return every derived-feature ID whose support contains the requested valid system. For `v > 0`, each valid system shall contain `2^(v - 1)` derived features, whether it is available or missing.

If an ID does not belong to the valid system space, then the analysis shall provide no system record for it.

**Example — Missing versus invalid**

- **Given** the implication fixture with valid IDs `{1, 3, 4}` and missing ID `1`.
- **When** a caller requests system records outside summary mode.
- **Then** each of `1`, `3`, and `4` has four derived features, and `2` has no system record.

[Membership API](../../include/feature_analysis.hpp#L487) · [Missing-system fixture](../../spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl) · [Valid-only report test](../../src/analysis_test.cpp#L203)

## Feature metadata

### FEATURE-006 — Observable categories

The analysis shall assign exactly one category to each derived feature. When a support is empty, the analysis shall classify it as `Impossible`. When a support covers a nonempty valid-system set in full, the analysis shall classify it as `Core`. When another support matches one or more simple forms on the valid systems, the analysis shall use the category of the form selected by [FEATURE-007](#feature-007--equivalent-simple-forms). An unmatched support shall be `Complex`.

A literal is a fundamental feature or its negation. In a simple form, each fundamental feature can appear once.

| Category | Form representing the support |
| --- | --- |
| `Fundamental` | One positive literal |
| `Not` | One negated literal |
| `And` | A conjunction of literals |
| `Or` | A disjunction of literals |
| `Complex` | No simple form matches |

**Example — Category counts**

- **Given** two unconstrained fundamental features.
- **When** the derived features are classified.
- **Then** the counts for `Impossible`, `Core`, `Fundamental`, `Not`, `And`, `Or`, and `Complex` are `1, 1, 2, 2, 4, 4, 2`, respectively.

[Categories](../../src/feature_analysis.cpp#L3133) · [Category tests](../../src/analysis_test.cpp#L102)

### FEATURE-007 — Equivalent simple forms

When several eligible simple forms represent the same support on all valid systems, the analysis shall prefer fewer literals, then fewer bytes of rendered text, then the lexicographically earliest rendered text. The chosen form shall determine the category and, where requested, the simplified term.

When the analysis renders a simple term, the analysis shall place positive literals before negative literals and sort each group by feature name.

**Example — One support with two names**

- **Given** `FEATURES 2 beta a` and `ASSERT beta <-> a`, so both names select the same system.
- **When** the matching support is requested with simplified terms.
- **Then** the category is `Fundamental` and the simplified term is `a`.

[Literal rendering](../../src/feature_analysis.cpp#L898) · [Form preference](../../src/feature_analysis.cpp#L947) · [Expected implication terms](../../src/analysis_test.cpp#L234)

### FEATURE-008 — Raw and simplified terms

Where raw terms are requested for a nonempty support, the analysis shall return the disjunction of the complete assignments for the systems in that support. Where simplified terms are requested, the analysis shall return a formula that is true on the support and false on the remaining valid systems. Invalid systems impose no truth-value constraint on a simplified term.

When the feature is `Impossible`, the analysis shall use `⊥` for either requested term. When the feature is `Core`, the analysis shall use `⊤` for the simplified term. When the feature is `Complex`, the analysis shall express the simplified term in disjunctive normal form (DNF).

**Example — Exclusive disjunction**

- **Given** the unconstrained feature basis `fx`, `fy` and a support of one-based systems `{2, 3}`.
- **When** the caller requests its raw and simplified terms.
- **Then** the raw term includes the two complete assignments, and the simplified term is a DNF true exactly when one of `fx` or `fy` is present.

**Test mismatch.** The older regression at `analysis_test.cpp:222` expects CNF for equally concise alternatives. The current DNF policy was recorded in commit `a00bad8`. See [FEATURE-D01](#feature-d01--complex-expression-contract) for the open minimality and rendering questions. [Simplified expression](../../src/feature_analysis.cpp#L2951) · [Constant terms](../../src/feature_analysis.cpp#L3133) · [CNF regression](../../src/analysis_test.cpp#L222)

### FEATURE-009 — Naming and optional details

When the caller supplies fewer names than the fundamental-feature count, the analysis shall use `f1`, `f2`, and so on by position for the unsupplied names.

When a display name is available, the analysis shall use the simplified term in preference to the raw term. When neither term is available, the display name shall be empty.

While a report filter is active or detail is disabled, the analysis shall preserve derived-feature IDs, categories, and supports. The analysis shall leave unrequested term fields empty, including terms for complex features when complex detail is disabled.

**Example — Excluding complex details**

- **Given** two unconstrained fundamental features and a report filter that excludes `Complex`.
- **When** a caller asks for the derived feature with support `{2, 3}`.
- **Then** it still has C++ ID `7` and category `Complex`, and both its term fields and display name are empty.

[Feature-name defaults](../../src/feature_analysis.cpp#L2480) · [Detail rules](../../src/feature_analysis.cpp#L3057) · [Filter and identity test](../../src/analysis_test.cpp#L269)

### FEATURE-010 — Lookup behavior

When a caller queries a valid derived-feature ID, the analysis shall return the presence vector through `at(id)`, the category code and name through `category(id)` and `category_name(id)`, and category-filter inclusion through `includes_feature(id)`.

If the caller supplies an ID outside the available feature registry, then those queries shall raise `std::out_of_range`.

When per-system membership is available, the analysis shall return it through `system_by_id(id)` for valid IDs. While summary mode is active, the analysis shall expose derived-feature queries and category counts, with no per-system membership records.

**Example — A lookup in summary mode**

- **Given** a summary of the unconstrained `fx`, `fy` model.
- **When** a caller queries derived-feature IDs `1` and `16`, then tries ID `17`.
- **Then** IDs `1` and `16` have category names `Impossible` and `Core`, respectively, and ID `17` raises `std::out_of_range`.
- **And** the summary exposes no per-system membership records.

[Accessors](../../src/feature_analysis.cpp#L2618) · [Summary constructor](../../src/feature_analysis.cpp#L2536) · [Lookup test](../../src/analysis_test.cpp#L159)

## Open decisions

### FEATURE-D01 — Complex expression contract

[FEATURE-008](#feature-008--raw-and-simplified-terms) specifies DNF and its truth values on the valid system space. Determine whether compatibility also requires exact rendered text and which optimality guarantee applies. The same decision affects [on-demand expressions](./expressions.md). [Current normal form](../../src/feature_analysis.cpp#L2951) · [Conflicting CNF test](../../src/analysis_test.cpp#L222)

### FEATURE-D02 — Supported counts

Candidate and derived counts grow as powers of two. Determine the supported model sizes and failure behavior when a count or output cannot be represented or produced. [Count API](../../src/feature_analysis.cpp#L2682) · [Planning and report admission](./planning.md)
