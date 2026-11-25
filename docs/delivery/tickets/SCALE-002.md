---
title: "Measure the CLI end to end"
milestone: M3
status: Draft
assignee: ""
---

# SCALE-002 — Measure the CLI end to end

- **Depends on** [BUILD-001](BUILD-001.md)
- **Scope** Provide a repeatable optimized build and benchmark inputs. Cover `plan`, `analyze --mode=summary`, a full report small enough to produce, `expr` with fixed selections, bounded `traces`, and an isolation report with a fixed witness mode. Use ArgoUML for planning, exact summary, and a fixed query, BusyBox for a sparse constrained query, and the small paper fixtures for full reports and isolation. Include a fixed trace grade and kind. Record failures for cases that still overflow or exceed the experiment's timeout and rerun those cases after the associated fix. [ArgoUML fixture](../../../spl_specs/new_dsl/valid/argo_uml_test.spl) · [BusyBox fixture](../../../spl_specs/new_dsl/valid/busybox.spl)
- **Done when** A second run can use the same fixture, command, build flags, and measurement procedure. Results record the revision, machine, OS, compiler, repetitions, elapsed-time median and spread, peak resident memory, and output bytes for completed commands. For any workload considered for optimization, time its major phases separately, such as building candidate terms, selecting the expression, and serializing output. The existing theory benchmark's system-space case measures access to a computed result. Earlier ArgoUML timings inform fixture selection. These measurements establish a comparison baseline and the planner continues to report estimates. [Build flags](../../../Makefile#L8) · [Existing benchmark](../../../src/theory_benchmark.cpp#L137) · [Resource limits](../../requirements/verification.md#open-decisions)
