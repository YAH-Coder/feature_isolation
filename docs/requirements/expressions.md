# Expression lookup requirements

Expression lookup returns a Boolean expression that selects a supplied set of systems within an SPL's valid system space.

## EXPR-001 — Command inputs

The expression CLI shall accept the command forms below. The SPL path identifies the fundamental features and constraints. The system ID list can be empty and uses one argument per ID.

```text
cli expr <spl-file> [<system-id> ...]
cli expression <spl-file> [<system-id> ...]
```

Where an SPL declares `CATEGORY_FILTER`, the expression CLI shall give the same result as the equivalent SPL with the filter omitted. Arguments after the path belong to the system ID list.

**Scenario — Alias and report hint**

- **Given** an SPL with `FEATURES 2 fx fy` and `CATEGORY_FILTER Core`
- **When** the caller runs `expression` for system IDs `1 3`
- **Then** the output is `fx`, as it is with `expr` or with the filter removed.

[Command parser](../../src/feature_isolation.cpp#L1453) · [Expression entry point](../../src/feature_isolation.cpp#L2769) · [Expression behavior](../../src/feature_analysis.cpp#L3230)

## EXPR-002 — Matching system IDs

When given system IDs, the expression lookup shall use zero-based system numbering and treat the IDs as a set. Its expression shall evaluate to true on every selected valid system and false on every other valid system. Missing systems remain valid. The expression can have either truth value on a constraint-invalid assignment. [System numbering](feature-space.md)

If a supplied numeric ID is outside the valid set, then the expression lookup shall warn on standard error and continue with the other IDs.

**Scenario outline — Valid and missing systems**

- **Given** the [missing-system fixture](../../spl_specs/new_dsl/valid/missing_systems_example.spl) with valid IDs `0`, `2`, and `3` in zero-based notation and missing ID `0`
- **When** the caller runs `expr` with the IDs in a row below
- **Then** the CLI prints the expression and warnings in that row.

| IDs | Expression | Warnings |
| --- | --- | --- |
| `0` | `¬fy` | None |
| `2 3` | `fy` | None |
| `1 9 2` | `fy ∧ ¬fx` | One each for IDs `1` and `9` |

[ID conversion](../../src/feature_analysis.cpp#L3253) · [Warnings](../../src/feature_analysis.cpp#L3269) · [Truth values](../../src/feature_analysis.cpp#L2951)

## EXPR-003 — Expression output

When expression lookup succeeds, the expression CLI shall exit with status `0` and print exactly one newline-terminated expression on standard output. The expression shall be `⊥` for an empty selection, including an empty valid system space, and `⊤` for a selection of every system in a nonempty valid system space. For a proper nonempty subset of the valid systems, it shall be in disjunctive normal form (DNF).

When rendering DNF, the expression lookup shall use `¬` before a negated name and spaces around `∧` and `∨`. Each term lists positive names first and sorts each polarity group by name. A term with several literals is parenthesized in a multi-term expression. Terms appear in ascending literal count. For equal-length terms, compare the literal at each feature position in declaration order, with absent before negated before positive.

Where the C++ expression interface receives an omitted display name, expression lookup shall use the positional fallback `f1`, `f2`, and so on.

**Scenario outline — Constants and DNF**

- **Given** the [unconstrained two-feature fixture](../../spl_specs/new_dsl/valid/unconstrained_2f.spl)
- **When** the caller runs `expr` with the IDs in a row below
- **Then** standard output is exactly the expression plus a newline and the exit status is `0`.

| IDs | Expression |
| --- | --- |
| None | `⊥` |
| `0 1 2 3` | `⊤` |
| `2 1 2` or `1 2` | `(fy ∧ ¬fx) ∨ (fx ∧ ¬fy)` |

[Command return status](../../src/feature_isolation.cpp#L2786) · [Constants and rendering](../../src/feature_analysis.cpp#L2211) · [Term order](../../src/feature_analysis.cpp#L2199) · [Generated names](../../src/feature_analysis.cpp#L2557) · [Standard output](../../src/feature_analysis.cpp#L3287)

## EXPR-004 — Simplification preference

When choosing among eligible expressions equivalent on all valid systems, the expression lookup shall prefer fewer total literal occurrences, then fewer conjunction terms, then fewer negated literals. Whether output must reach a global minimum remains open. [EXPR-D01](#expr-d01--normal-form-and-optimality) · [Preference comparison](../../src/feature_analysis.cpp#L1947)

**Scenario — Prefer a positive literal**

- **Given** two features `x` and `y` constrained by `ASSERT x <-> !y`
- **When** the caller requests the expression for system ID `1`, where only `x` is selected
- **Then** the result is `x`. The equivalent alternative `¬y` has the same literal and term counts and one negated literal.

## EXPR-005 — Failure and help behavior

If the SPL path cannot be opened, then the expression CLI shall report a diagnostic on standard error and exit with status `2`. If the invocation has no SPL path, the SPL is invalid, or an ID is malformed or unrepresentable, then the expression CLI shall report the error and return a nonzero exit status. The exact status and accepted numeric spellings remain open for these cases. [EXPR-D02](#expr-d02--invalid-arguments-and-guided-input) · [File opening](../../src/feature_isolation.cpp#L2771)

**Scenario outline — Expression invocation errors**

- **Given** the cause in a row below
- **When** the caller invokes `expr`
- **Then** the CLI reports the error without an expression and returns the stated status.

| Cause | Expected status |
| --- | --- |
| Path points to a nonexistent SPL | `2` |
| The SPL path argument is omitted | Nonzero |
| Valid SPL path followed by ID `abc` | Nonzero |
| SPL contains a constraint over an undeclared name | Nonzero |

## EXPR-006 — Guided entry

When the interactive command prompt receives `expr` or `e`, the guided CLI shall ask for an SPL path and support an empty selection. The behavior for supplying IDs through the wizard is open. [EXPR-D02](#expr-d02--invalid-arguments-and-guided-input) · [Command prompt](../../src/feature_isolation.cpp#L916) · [Guided inputs](../../src/feature_isolation.cpp#L1367)

**Scenario — Guided lookup with an empty selection**

- **Given** the interactive command prompt
- **When** the caller enters `e`, supplies the two-feature fixture path, and supplies no system IDs
- **Then** standard output contains `⊥` plus a newline.

## EXPR-007 — C++ expression entry points

When a caller uses the expression overload of `feature_analysis_t` or `feature_isolation_t` with model input and selected IDs as strings, the overload shall print the corresponding expression on standard output. When a caller invokes the expression overload of `initialize_feature_registry`, it shall print the expression and return an empty registry vector. [Analysis API](../../include/feature_analysis.hpp#L684) · [Isolation API](../../include/isolation.hpp#L211) · [Return value](../../src/feature_analysis.cpp#L3287)

**Scenario — Expression from C++**

- **Given** the unconstrained space for `fx` and `fy` and selection `{"1", "3"}`
- **When** the caller passes that selection to the expression overload of `initialize_feature_registry`
- **Then** standard output receives `fx` plus a newline and the return value contains zero feature records.

## Verification examples

Run from the repository root after `make cli`. Each expected output is followed by a newline.

| Command after `./testing/generated/cli` | Expected standard output | Additional check |
| --- | --- | --- |
| `expr spl_specs/new_dsl/valid/unconstrained_2f.spl` | `⊥` | Exit `0`. |
| `expr spl_specs/new_dsl/valid/unconstrained_2f.spl 0 1 2 3` | `⊤` | All four systems selected. |
| `expr spl_specs/new_dsl/valid/unconstrained_2f.spl 2 1 2` | `(fy ∧ ¬fx) ∨ (fx ∧ ¬fy)` | Same output as arguments `1 2`. |
| `expr spl_specs/new_dsl/valid/missing_systems_example.spl 0` | `¬fy` | Valid but missing `s0` selected. |
| `expr spl_specs/new_dsl/valid/missing_systems_example.spl 2 3` | `fy` | The expression is false on missing `s0`. |
| `expr spl_specs/new_dsl/valid/missing_systems_example.spl 1 9 2` | `fy ∧ ¬fx` | Standard error contains a warning for each of `1` and `9`. |

## Deviations and open decisions

### EXPR-D01 — Normal form and optimality

**Test discrepancy.** The existing analysis regression expects CNF for some registry expressions that share the simplification behavior. DNF is the output requirement in [EXPR-003](#expr-003--expression-output). [CNF test expectation](../../src/analysis_test.cpp#L222) · [DNF return](../../src/feature_analysis.cpp#L2983)

**Open decision.** Decide whether the simplification preference requires a globally minimum DNF, and whether tied expressions have a required spelling. In an unconstrained `FEATURES 4 a b c d` model for IDs `0 1 2 5 6 7 8 10 11 12`, the current result has 15 literals. This equivalent result has 14.

```text
(¬a ∧ ¬c) ∨ (a ∧ ¬b ∧ ¬d) ∨ (b ∧ c ∧ ¬d) ∨ (b ∧ d ∧ ¬c) ∨ (d ∧ ¬a ∧ ¬b)
```

### EXPR-D02 — Invalid arguments and guided input

**Code deviation.** The direct command can terminate abnormally without a diagnostic when the path argument is missing. SPL parsing and numeric conversion failures can also terminate the command abnormally. Numeric prefixes such as `2abc` are accepted as `2`, and the conversion accepts a leading sign. Guided lookup currently supplies an empty selection and has no ID prompt. `cli expr --help` treats `--help` as a path and returns `2` if that file is absent. General and guided help omit expression lookup. [Argument collection](../../src/feature_isolation.cpp#L1453) · [Help dispatch](../../src/feature_isolation.cpp#L3050) · [Guided inputs](../../src/feature_isolation.cpp#L1367) · [Numeric conversion](../../src/feature_analysis.cpp#L3269)

**Open decision.** Define accepted ID spellings and exact exit statuses for invalid input. Decide how to offer system IDs in guided lookup and whether to give expression lookup a dedicated help screen.

### EXPR-D03 — Resource expectations

Expression lookup has no specified execution-time or peak-memory limit. The direct expression command runs outside the report approval step. [Expression entry point](../../src/feature_isolation.cpp#L2771)
