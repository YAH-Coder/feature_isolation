# Feature isolation

Isolation uses available systems to find derived features consistent with a target's observed presence and absence. Each result contains candidate derived features and system pairs that witness the result. See [system and feature semantics](feature-space.md) for the underlying membership model.

## ISO-001 — Targets and system membership

When isolation partitions a target feature, the service shall return `I` containing the available systems in which the target is present and `U` containing the available systems in which it is absent. The pair shall partition the available system set. When a report applies a category selection, the service shall apply the selection to target rows and consider candidates across all derived feature categories.

The public C++ isolation methods shall use one-based IDs. When `sid_to_fids` receives a system ID with membership data, the service shall return that system's derived feature IDs, whether the system is available or missing. If the supplied system ID has no membership data, then `sid_to_fids` shall throw `std::out_of_range`. If a caller requests the name or report row of an unknown feature, then the service shall throw `std::out_of_range`. Unknown IDs supplied to partition and witness queries are covered by [ISO-D03](#open-decisions).

**Scenario — Partition available systems for a target**

- **Given** the [missing-system implication SPL](../../spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl) and target `t1`, which is present only in `s3`
- **When** the caller asks to partition one-based feature ID `2`
- **Then** the returned `I` is `{4}`, the returned `U` is `{3}`, and the missing system with ID `1` is omitted from both

[Partition and membership](../../include/isolation.hpp#L342) · [System lookup](../../src/feature_analysis.cpp#L2702) · [Feature names](../../include/isolation.hpp#L876)

## ISO-002 — Candidate set

When the target has at least one available occurrence, the isolation service shall return every derived feature with the same presence and absence as the target over all available systems. Missing systems may differ. When the report has one candidate, it shall show `UNIQUE`. When the report has multiple candidates, it shall show `AMBIGUOUS`. The candidate set and result label for targets with no available occurrence are covered by [ISO-D01](#open-decisions).

When a caller supplies explicit system sets to the public flattening methods, `flatten_by_intersection` shall return the derived features present in every supplied system, `flatten_by_union` shall return the derived features present in at least one supplied system, and `flattened_difference` shall return the former set with the latter removed. Either flattening method shall return an empty set for an empty input set.

**Scenario — Candidates differ only in a missing system**

- **Given** the implication SPL where `t1` occurs in `s3` and `t5` occurs in both `s0` and `s3`, with `s0` missing
- **When** the caller asks for the candidates for `t1`
- **Then** the service returns exactly `t1` and `t5`, and the report labels the result `AMBIGUOUS`

[Flattening operations](../../include/isolation.hpp#L365) · [Report labels](../../include/isolation.hpp#L793)

## ISO-003 — Witness modes

While the target occurs in at least one available system, when a caller requests a single witness, the isolation service shall return one pair of available system subsets that yields the target's candidate set. While that condition holds, when a caller requests minimal witnesses, the service shall return the minimal pairs that yield the same candidate set. When a caller requests either mode for a target in the full derived feature space with at least one available occurrence, the service shall return the full partition `(I, U)` as the only witness. Behavior when `I` is empty is covered by [ISO-D01](#open-decisions).

**Scenario — Witness for a feature with an available occurrence**

- **Given** the implication SPL and target `t1`
- **When** the caller requests minimal witnesses with `-min-diffs=true`
- **Then** the report shows one witness, the pair with `s3` included and `s2` excluded
- **And** the same pair is returned in single-witness mode with `-min-diffs=false`

[Minimal witness results](../../include/isolation.hpp#L465) · [Single witness results](../../include/isolation.hpp#L685)

## ISO-004 — Caller-supplied system memberships

When `search_minimal_differences` receives a target feature-ID set, two eligible system sets, and a membership map, the public isolation API shall return an ordered set of minimal witness pairs whose memberships yield the supplied target. The intersection side of each returned pair shall be nonempty. The API shall support feature IDs above `1024`. If an eligible system lacks a membership entry, then the API shall throw `std::out_of_range`. The meaning of minimality for witnesses with incomparable sides is covered by [ISO-D02](#open-decisions).

**Scenario — Witness using feature IDs above 1024**

- **Given** target feature IDs `{2048}`, eligible sets `I={1}` and `U={2}`, and memberships `1 → {2048, 4096}` and `2 → {4096}`
- **When** the caller requests minimal witnesses
- **Then** the API returns exactly the pair `({1}, {2})`

[Witness search API](../../include/isolation.hpp#L236) · [Membership lookup](../../include/isolation.hpp#L495) · [High-ID regression](../../src/isolation_test.cpp#L410)

## ISO-005 — Report rows

When an isolation report is written, the report shall order target rows by ascending feature ID and begin each row with `i`, `FEATURE`, `ℓ_T(t_i)`, `CANDIDATE FEATURES`, and `RESULT TYPE`. It shall display candidate feature labels in ascending ID order inside braces and always include the target display name. When `-feature-names=true` is selected, the report shall append ` = ` and the display name to each candidate label.

When single-witness mode is selected, the report shall append `SOME DIFFERENCE`. When minimal-witness mode is selected, it shall append `#WITNESSES` and `MINIMAL DIFFERENCES`. The report shall represent each pair as an inclusion set, a backslash, and an exclusion set. Labels within a set shall be in ascending order and joined by `&` for inclusion and `|` for exclusion. The report shall represent an empty witness side as `{}` and list multiple witnesses as tab-separated values ordered by inclusion set, then exclusion set.

**Scenario — Named candidates and one minimal witness**

- **Given** the implication SPL, isolation-only mode, and the names represented by presence strings
- **When** the caller runs `analyze` with `-min-diffs=true -feature-names=true`
- **Then** the row for `t1` shows target name `100`, candidates `{ t1 = 100, t5 = 101 }`, result `AMBIGUOUS`, witness count `1`, and witness `{ s3 } \ { s2 }`

[Row and header output](../../include/isolation.hpp#L771) · [Witness serialization](../../include/isolation.hpp#L918)

## Verification example

```sh
./testing/generated/cli analyze spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl /tmp/feature-isolation.txt -min-diffs=true -feature-names=false --mode=isolation-only
```

This command reproduces the `t1` candidate set and minimal witness from ISO-002 and ISO-003. Use `-feature-names=true` to reproduce ISO-005.

## Open decisions

- **ISO-D01 Empty intersection and ambiguity.** The baseline returns zero candidates and labels the result `AMBIGUOUS` when a target has no available occurrence. For the implication SPL, minimal mode returns zero witnesses for `t0` and `t4` and yields three two-candidate ambiguity classes overall. Single-witness mode returns `{} \ { s2 }` for both targets. When no systems are available, the baseline returns one pair with two empty sides in single-witness mode and zero pairs in minimal mode. The optional `[theory]` tests expect two candidates and a witness for `t0`, and four two-candidate ambiguity classes. The target for this case needs a decision. [Theory tests](../../src/isolation_test.cpp#L378) · [Baseline result](../../include/isolation.hpp#L465)
- **ISO-D02 Minimality for caller-supplied memberships.** For target `{1}`, eligible sides `I = {1, 2}` and `U = {3, 4}`, and memberships `1 → {1, 3, 4}`, `2 → {1, 2, 4}`, `3 → {3, 4}`, `4 → {4}`, both `({1}, {3})` and `({1, 2}, {4})` yield the target. Both pairs are minimal if comparison requires containment on both sides and strict containment on at least one. The baseline returns only `({1}, {3})`. The criterion for these cases needs a decision. [Witness comparison](../../include/isolation.hpp#L731)
- **ISO-D03 Unknown target IDs.** The baseline treats an unknown feature ID as absent in each available system when partitioning or requesting witnesses. The name and row APIs reject it. Decide whether partition and witness queries should accept an unknown feature ID. [Partition](../../include/isolation.hpp#L342) · [Feature name validation](../../include/isolation.hpp#L876)
