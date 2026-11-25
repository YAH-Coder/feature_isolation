# Exact summary and solver evaluation

## M3 — Exact ArgoUML summary and native baseline

M3 is complete when the ArgoUML summary passes its acceptance test and a reproducible release measurement covers the main CLI workflows. Native optimizations are evaluated against that measurement and the relevant acceptance tests.

### SCALE-001 — Compute the exact ArgoUML summary

- **Status** Planned
- **Depends on** [TST-002](verification.md#tst-002--isolate-cli-test-runs)
- **Scope** Add the end-to-end test for `REPORT-008` before changing the counting path. The test should reproduce the current integer overflow and then pass through the normal `analyze --mode=summary` entry point. Give the summary a counting path independent of exhaustive feature generation. [ArgoUML scenario](../requirements/reports.md#report-008--exact-argouml-summary) · [Registry construction](../../src/feature_analysis.cpp#L3088)
- **Done when** The CLI exits successfully and reports eight fundamental features, 256 valid and available systems, zero missing systems, exactly $2^{256}$ derived features, and all seven exact category counts. The expected value for each of `And` and `Or` is $3^8 - 1 - 2\cdot8 = 6{,}544$, counting distinct conjunctions or disjunctions of two or more literals on an unconstrained basis. Compare exact summary counts with exhaustive reports on small unconstrained and constrained fixtures, including a fixture with missing systems. Check the existing summary format and category filter behavior. [Summary contract](../requirements/reports.md#report-003--totals-and-category-selection)

### SCALE-002 — Measure the CLI end to end

- **Status** Planned
- **Depends on** [BUILD-001](release.md#build-001--make-builds-reproducible)
- **Scope** Provide a repeatable optimized build and benchmark inputs. Cover `plan`, `analyze --mode=summary`, a full report small enough to produce, `expr` with fixed selections, bounded `traces`, and an isolation report with a fixed witness mode. Use ArgoUML for planning, exact summary, and a fixed query, BusyBox for a sparse constrained query, and the small paper fixtures for full reports and isolation. Include a fixed trace grade and kind. Record failures for cases that still overflow or exceed the experiment's timeout and rerun those cases after the associated fix. [ArgoUML fixture](../../spl_specs/new_dsl/valid/argo_uml_test.spl) · [BusyBox fixture](../../spl_specs/new_dsl/valid/busybox.spl)
- **Done when** A second run can use the same fixture, command, build flags, and measurement procedure. Results record the revision, machine, OS, compiler, repetitions, elapsed-time median and spread, peak resident memory, and output bytes for completed commands. For any workload considered for optimization, time its major phases separately, such as building candidate terms, selecting the expression, and serializing output. The existing theory benchmark's system-space case measures access to a computed result. Earlier ArgoUML timings inform fixture selection. These measurements establish a comparison baseline and the planner continues to report estimates. [Build flags](../../Makefile#L8) · [Existing benchmark](../../src/theory_benchmark.cpp#L137) · [Resource limits](../requirements/verification.md#open-decisions)

### SCALE-003 — Remove measured waste from the native path

- **Status** Planned
- **Depends on** [SCALE-002](#scale-002--measure-the-cli-end-to-end), [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table)
- **Scope** Start with the discarded CNF calculation in the DNF expression path. Measure the difference on the same fixed workloads. Open focused follow-up tickets for any additional bottlenecks confirmed by profiling, such as candidate generation or trace serialization. [Current simplifier](../../src/feature_analysis.cpp#L2951) · [Expression output contract](../requirements/expressions.md#expr-003--expression-output)
- **Done when** Expression and report checks still pass, including any public format assertions, and the measured effect is recorded against the optimized baseline. Each follow-up identifies its workload and measured bottleneck.

## M4 — OR-Tools C++ trial

M4 is complete when the native and trial paths have been checked against the same contract and the project has recorded whether to adopt the trial based on end-to-end results.

### ORT-001 — Prototype CP-SAT for expression minimization

- **Status** Planned
- **Depends on** [DEC-002](correctness.md#dec-002--dnf-quality-and-ties), [SCALE-002](#scale-002--measure-the-cli-end-to-end), [TST-005](verification.md#tst-005--verify-expression-and-trace-behavior-by-truth-table)
- **Scope** Put the C++ CP-SAT prototype behind an opt-in build or execution path and retain the native path for comparison. Use the valid system rows as the constraint domain. A candidate DNF covers every selected valid system and excludes every unselected valid system. Invalid assignments remain outside the truth-table contract. Apply the approved simplification objective and tie behavior. Handle solver statuses according to that policy, including the case where the solver finds an expression without proving optimality. [System membership contract](../requirements/expressions.md#expr-002--matching-system-ids) · [CP-SAT guide](https://developers.google.com/optimization/cp/cp_solver/) · [OR-Tools C++ installation](https://developers.google.com/optimization/install/cpp)
- **Done when** The native and opt-in builds both succeed on macOS and Linux. Both paths pass the shared input, output, and truth-table checks on small fixtures they can execute. The trial is also checked against all valid BusyBox rows, with any native timeout or failure reported in the comparison. Include a small independent exhaustive oracle for any required optimum and tests for the solver outcomes exposed by the approved policy. Document a reproducible invocation and the OR-Tools version used.

### ORT-002 — Compare both paths and decide whether to adopt CP-SAT

- **Status** Planned
- **Depends on** [ORT-001](#ort-001--prototype-cp-sat-for-expression-minimization), [SCALE-003](#scale-003--remove-measured-waste-from-the-native-path)
- **Scope** Run the shared acceptance suite and the optimized workload matrix with equivalent inputs and output scope. Compare CLI totals as well as time spent building the model, solving, and rendering the result. Record solver version and settings. Check the required truth tables and formatting, and compare expression costs under the approved policy. Report the unaffected workloads alongside expression lookup so any whole-system claim reflects those measurements.
- **Done when** The comparison records run failures, elapsed time, peak memory, output size, and correctness for each path, with an explicit adoption decision. Count a timing improvement as comparable when both runs satisfy the same output and quality guarantees. Record newly supported cases separately. Favor adoption with a reproducible end-to-end benefit and an acceptable maintenance cost. Document the chosen behavior for supported builds.

### ORT-003 — Reassess the case for solver-backed system enumeration

- **Status** Planned
- **Depends on** [ORT-002](#ort-002--compare-both-paths-and-decide-whether-to-adopt-cp-sat)
- **Scope** Revisit constrained valid-system enumeration only if phase timings show it remains a material bottleneck. If warranted, compare a separate feasibility and enumeration experiment with the native enumerator on sparse and dense models. [Valid system contract](../requirements/spl.md#spl-008--valid-systems-are-the-satisfying-assignments) · [Native enumeration](../../src/spl.cpp#L870)
- **Done when** The timing evidence supports closing the ticket or leads to a prototype comparison that verifies identical valid IDs, availability partitions, and ordering wherever the contract requires it. Any adoption has its own measured evidence.
