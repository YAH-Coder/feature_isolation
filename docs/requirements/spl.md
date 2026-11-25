# SPL input and validation

The SPL parser accepts a text specification and supplies its Boolean features, constraints, and system availability. The numbered requirements define the accepted input and its observable results.

## File and section rules

### SPL-001 — Optional name and feature basis

When the SPL contains a `FEATURES` declaration, the parser shall return the declared count and the corresponding names in declaration order. It shall accept a count of zero.

If the `FEATURES` declaration is missing, then the parser shall reject the specification.

Where the `SPL` section is present, the parser shall use its single-token name. When the section is absent, the parser shall report the name `NOT SPECIFIED`.

If a constraint refers to a feature whose exact case-sensitive name has yet to be declared, then the parser shall reject the reference.

**Example — An unnamed product line**

- **Given** a file with `FEATURES 2 fx fy` and no `SPL` section.
- **When** the file is parsed.
- **Then** the product line name is `NOT SPECIFIED` and the ordered feature names are `fx` and `fy`.

**Example — Missing feature basis**

- **Given** a file containing only `SPL Example`.
- **When** the file is parsed.
- **Then** validation fails because the feature declaration is missing.

**Code mismatch.** The parser accepts that second example and uses a default count of three. See [SPL-D01](#spl-d01--feature-declarations). [Authoring guide](../../include/spl_authoring.dox#L72) · [Parser](../../src/spl.cpp#L1472) · [Optional sections test](../../src/spl_test.cpp#L125)

### SPL-002 — Words, numbers, and comments

The SPL parser shall recognize word tokens made of letters, digits, `_`, `#`, and `.`, and shall read integer operands as nonnegative decimal numbers.

When the parser encounters whitespace between tokens or a `//` line comment, the parser shall ignore it. Section names, Boolean keyword literals, and `IN` shall be recognized regardless of case. Symbolic operators shall be accepted adjacent to their operands.

If an integer operand is negative, then the parser shall reject it.

**Example — Equivalent formatting**

- **Given** the declarations `features 2 fx fy // feature names` and `constraints assert fx->fy end` on separate lines.
- **When** the file is parsed.
- **Then** its constraint is the same as `CONSTRAINTS ASSERT fx -> fy END` and only one-based systems `1`, `3`, and `4` are valid.

**Documentation mismatch.** The EBNF requires specific spaces and an assertion-ending newline and permits signed integers. See [SPL-D02](#spl-d02--lexical-grammar) for the open question about unrecognized punctuation. [Authoring guide](../../include/spl_authoring.dox#L39) · [EBNF](../../external_documentation/spl_dsl.ebnf) · [Tokenizer test](../../src/spl_test.cpp#L195)

### SPL-003 — Supported sections and their boundaries

The parser shall accept the top-level sections `SPL`, `FEATURES`, `CONSTRAINTS`, `AVAILABLE_SYSTEMS`, `MISSING_SYSTEMS`, and `CATEGORY_FILTER`. Each may occur at most once. The accepted feature model shall be Boolean.

When the parser encounters an `AVAILABLE_SYSTEMS`, `MISSING_SYSTEMS`, or `CATEGORY_FILTER` section, the parser shall accept a list ending at `END`, the next recognized section, or the end of the file.

If a `CONSTRAINTS` section lacks `END`, a top-level section is repeated, or an unsupported section such as `MAX_RANK`, `CATEGORIES`, or `EXPLICIT_DEPENDENCIES` is supplied, then the parser shall reject the specification.

**Example — Section boundaries**

- **Given** an otherwise valid SPL with the sections listed below.
- **When** the file is parsed.
- **Then** the parser accepts or rejects the input as shown.

| Sections | Expected outcome |
| --- | --- |
| `FEATURES 1 fx MISSING_SYSTEMS 1` followed by end of file | Accepted with `1` marked missing. |
| `FEATURES 1 fx CONSTRAINTS END` | Accepted with no assertions. |
| `FEATURES 1 fx CONSTRAINTS` followed by end of file | Rejected because `END` is missing. |
| `FEATURES 1 fx FEATURES 1 fy` | Rejected because the feature section is repeated. |

**Code mismatch.** A repeated top-level section currently replaces the earlier value. **Documentation mismatch.** The EBNF requires `END` for availability lists, which the authoring guide makes optional. [Section rules](../../include/spl_authoring.dox#L72) · [Availability authoring rules](../../include/spl_authoring.dox#L131) · [Availability parser](../../src/spl.cpp#L2020) · [Legacy input tests](../../src/spl_test.cpp#L219)

### SPL-004 — Category hint

Where `CATEGORY_FILTER` is present, the parser shall interpret the case-insensitive category names `Impossible`, `Core`, `Fundamental`, `Not`, `And`, `Or`, and `Complex` as a reporting hint. An empty section shall produce an empty category list, and its absence shall leave the hint unspecified.

If a category is unknown or repeated, then the parser shall reject the specification.

When the category hint changes, the analysis shall preserve the valid-system set and derived-feature identities.

**Example — Category spelling and effect**

- **Given** `FEATURES 2 fx fy` with no system constraints.
- **When** the parser reads one of the category declarations below.
- **Then** it returns the result shown.

| Declaration | Expected outcome |
| --- | --- |
| `CATEGORY_FILTER core FUNDAMENTAL` | The hint contains `Core` and `Fundamental`. All four systems and 16 derived features still exist. |
| `CATEGORY_FILTER core Core` | Rejected because the category is repeated. |

[Filter parser](../../src/spl.cpp#L1643) · [Filter test](../../src/spl_test.cpp#L183) · [Report request behavior](./reports.md)

## Constraint rules

### SPL-005 — Boolean formulas

When an SPL has a `CONSTRAINTS` section, the parser shall accept zero or more `ASSERT <formula>` entries. When assertions are present, the analyzer shall require all of them to hold in each valid system. When there are no assertions, the analyzer shall treat all candidate systems as valid.

The parser shall accept declared feature references, `TRUE`, `FALSE`, parenthesized formulas, and `COUNT` expressions as formula atoms. It shall apply operator precedence from strongest to weakest as `!`, `&&`, `||`, `->`, and `<->`. Implication shall associate to the right and be false only for true implying false. Equivalence shall associate to the left and be true when its operands have the same truth value.

**Example — Conjoined assertions**

- **Given** `FEATURES 2 fx fy` with the assertions `fx -> fy` and `fy`.
- **When** the valid systems are requested.
- **Then** exactly the one-based IDs `3` and `4` are valid.

[Formula syntax and precedence](../../include/spl_authoring.dox#L193) · [Evaluation](../../src/spl.cpp#L437) · [Combined constraints test](../../src/spl_test.cpp#L79)

### SPL-006 — Cardinality formulas

When evaluating `COUNT(f1, f2, ...)` for a system, the analyzer shall count the listed features present in that system. The parser shall accept one or more distinct declared feature names and a comparison of `=`, `!=`, `<`, `<=`, `>`, or `>=` to a nonnegative integer. It shall accept `IN [lower,upper]` as an inclusive interval.

If a list is empty, repeats a name, contains an undeclared name, or has an interval with its lower endpoint above its upper endpoint, then the parser shall reject the expression.

**Example — Count comparisons**

- **Given** three Boolean features `fx`, `fy`, and `fz`.
- **When** the valid systems are requested with the named assertion.
- **Then** the analyzer returns the listed one-based IDs or reports the listed error.

| Assertion | Expected outcome |
| --- | --- |
| `COUNT(fx, fy, fz) = 1` | `{2, 3, 5}` |
| `COUNT(fx, fy, fz) IN [1,2]` | `{2, 3, 4, 5, 6, 7}` |
| `COUNT(fx, fx) = 1` | Duplicate `COUNT` feature error. |
| `COUNT(fx, fy) IN [2,1]` | Invalid `COUNT` interval error. |

**Documentation mismatch.** The parser overview lists `==`. The EBNF, authoring guide, and executable use `=`. [Parser overview](../../include/spl.dox#L42) · [Authoring guide](../../include/spl_authoring.dox#L208) · [Comparator tests](../../src/spl_test.cpp#L238) · [Malformed COUNT tests](../../src/spl_test.cpp#L274)

## Validity and availability

### SPL-007 — Candidate system numbering

When an SPL declares `n` fundamental features and the candidate count is representable, the analyzer shall assign a candidate ID to each of the `2^n` Boolean assignments. The one-based ID used in availability sections and the C++ API shall equal the assignment bit mask plus one. The first declared feature shall be the least significant bit, and the paper-style label `sN` shall use the bit mask as its suffix.

**Example — Two-feature IDs**

- **Given** `FEATURES 2 fx fy` with no assertions.
- **When** the assignments are identified by system ID and label.
- **Then** each of the following identifies the same assignment on both interfaces.

| One-based ID | Label | Present features |
| --- | --- | --- |
| `1` | `s0` | None |
| `2` | `s1` | `fx` |
| `3` | `s2` | `fy` |
| `4` | `s3` | `fx` and `fy` |

[Assignment evaluation](../../src/spl.cpp#L380) · [System labels](../../include/feature_analysis.hpp#L481) · [Unconstrained fixture test](../../src/spl_test.cpp#L64) · [Analysis counts](./feature-space.md#feature-001--system-identifiers-and-counts)

### SPL-008 — Valid systems are the satisfying assignments

When an SPL is analyzed, the analyzer shall include exactly the candidate systems that satisfy every assertion in the valid-system set. Availability declarations and category hints shall preserve that set.

When the C++ caller passes a candidate system ID to `system_satisfies_constraints`, the query shall return whether the assignment satisfies the assertions.

**Example — Availability does not determine validity**

- **Given** declared features `fx` and `fy` with one of the sets of declarations below.
- **When** the valid-system IDs are requested.
- **Then** the result matches the listed valid IDs.

| Declarations | Valid IDs |
| --- | --- |
| `CONSTRAINTS ASSERT fx -> fy END MISSING_SYSTEMS 1 END` | `{1, 3, 4}` |
| `CONSTRAINTS ASSERT FALSE END` | `{}` |

[Implication fixture](../../spl_specs/new_dsl/valid/fx_implies_fy.spl) · [System partition test](../../src/spl_test.cpp#L143) · [Constraint query](../../src/spl.cpp#L538)

### SPL-009 — Available and missing systems partition the valid set

When the analyzer resolves system availability, the analyzer shall apply the following rules to the valid-system set. It shall accept explicitly empty lists and interpret repeated IDs within one list as one member of the resolved set.

| Sections present | Available systems | Missing systems |
| --- | --- | --- |
| Neither | All valid systems | Empty set |
| `AVAILABLE_SYSTEMS` only | Listed systems | Complement within the valid set |
| `MISSING_SYSTEMS` only | Complement within the valid set | Listed systems |
| Both | Listed available systems | Listed missing systems |

If either list contains an invalid system ID or both lists fail to partition the valid set into disjoint subsets, then the parser shall reject the specification.

**Example — Availability declarations**

- **Given** the valid one-based IDs `{1, 3, 4}`.
- **When** the analyzer reads the declarations below.
- **Then** it resolves the named sets or rejects the input as shown.

| Declarations | Expected outcome |
| --- | --- |
| None | Available `{1, 3, 4}`, missing `{}` |
| `AVAILABLE_SYSTEMS END` | Available `{}`, missing `{1, 3, 4}` |
| `MISSING_SYSTEMS 1 1 END` | Available `{3, 4}`, missing `{1}` |
| `AVAILABLE_SYSTEMS 3 4 END MISSING_SYSTEMS 1 END` | Available `{3, 4}`, missing `{1}` |
| `AVAILABLE_SYSTEMS 2 END` | Rejected because `2` is invalid. |
| `AVAILABLE_SYSTEMS 1 3 END MISSING_SYSTEMS 3 4 END` | Rejected because the lists overlap at `3`. |
| `AVAILABLE_SYSTEMS 3 END MISSING_SYSTEMS 1 END` | Rejected because both lists omit `4`. |

[Partition behavior](../../src/spl.cpp#L965) · [Validation tests](../../src/spl_test.cpp#L258) · [Missing-system fixture](../../spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl)

## Parsed data, serialization, and errors

### SPL-010 — Parsed sections and resolved system space

When a C++ caller reads a parsed SPL, the specification shall expose its filename, name, feature count and names, assertions, optional category hint, and section presence. The explicitly declared availability lists shall preserve order and repetitions. The resolved system-space query shall return the valid, available, and missing sets, including any implied complements.

**Example — Explicit input and resolved sets**

- **Given** an SPL whose valid IDs are `{1, 3, 4}` and which declares only `MISSING_SYSTEMS 1 END`.
- **When** a C++ caller inspects its parsed and resolved availability.
- **Then** the available section is absent, the explicit available list is empty, and the resolved available set is `{3, 4}`.

[Parsed specification API](../../include/spl.hpp#L208) · [Explicit and derived spaces test](../../src/spl_test.cpp#L168)

### SPL-011 — Normalized output

When a caller serializes a parsed SPL, the serializer shall emit its supplied sections in the order `SPL`, `FEATURES`, `CONSTRAINTS`, `AVAILABLE_SYSTEMS`, `MISSING_SYSTEMS`, and `CATEGORY_FILTER`. It shall use normalized whitespace, omit source comments, and close constraint and availability blocks with `END`.

When a serialized SPL is read and serialized again, the serializer shall return the same normalized text.

**Example — Read and write an SPL twice**

- **Given** the combined Boolean-and-cardinality fixture.
- **When** the fixture is parsed and printed, and the printed SPL is parsed and printed again.
- **Then** both printed texts match.

[Combined fixture](../../spl_specs/new_dsl/valid/constraints_boolean_and_count.spl) · [Serializer](../../src/spl.cpp#L2088) · [Round-trip test](../../src/spl_test.cpp#L44)

### SPL-012 — Error reporting and coverage

If the C++ SPL processor cannot open the input or encounters invalid input, including a malformed section, invalid formula, or invalid availability declaration, then the processor shall report the failure through an exception. Availability diagnostics shall identify the affected section and ID or the violated partition rule. Syntax errors with a source location shall report the line and column.

When a caller requests `theory_dsl_coverage()`, the processor shall report the recognized capabilities and unsupported feature dimensions for the parsed specification. See [CLI status and diagnostics](./cli.md#cli-006--status-and-diagnostics) for the command-level contract.

**Example — Invalid available system**

- **Given** the invalid available-system fixture that declares one-based system `2` available despite a constraint that rules it out.
- **When** a C++ caller loads the specification.
- **Then** the processor raises an exception that identifies system `2` and `AVAILABLE_SYSTEMS`.

[Invalid fixture](../../spl_specs/new_dsl/invalid/invalid_available_systems.spl) · [Coverage query](../../src/spl.cpp#L1037) · [Diagnostic test](../../src/spl_test.cpp#L258)

## Open decisions

### SPL-D01 — Feature declarations

The authoring guide requires `FEATURES` and permits each top-level section at most once. [SPL-001](#spl-001--optional-name-and-feature-basis) and [SPL-003](#spl-003--supported-sections-and-their-boundaries) follow that contract. The current parser accepts an empty file or a file with only a name and uses three default features. It also accepts repeated declarations. Decide whether duplicate feature names within a single declaration should be rejected. The current parser accepts them and a constraint resolves the first matching name. [Authoring guide](../../include/spl_authoring.dox#L72) · [Parser declaration behavior](../../src/spl.cpp#L1472) · [Name lookup](../../src/spl.cpp#L1567)

### SPL-D02 — Lexical grammar

Decide whether unrecognized punctuation should produce a diagnostic. The current parser ignores some characters, such as `@` in `ASSERT @x`. The supported number, whitespace, availability, and equality syntax is specified in SPL-002, SPL-003, and SPL-006. Earlier documentation variants are identified with those requirements. [Tokenizer](../../src/spl.cpp#L1254) · [EBNF](../../external_documentation/spl_dsl.ebnf)
