---
title: "Remove measured waste from the native path"
milestone: M3
status: Draft
assignee: ""
---

# SCALE-003 — Remove measured waste from the native path

- **Depends on** [SCALE-002](SCALE-002.md), [TST-005](TST-005.md)
- **Scope** Start with the discarded CNF calculation in the DNF expression path. Measure the difference on the same fixed workloads. Open focused follow-up tickets for any additional bottlenecks confirmed by profiling, such as candidate generation or trace serialization. [Current simplifier](../../../src/feature_analysis.cpp#L2951) · [Expression output contract](../../requirements/expressions.md#expr-003--expression-output)
- **Done when** Expression and report checks still pass, including any public format assertions, and the measured effect is recorded against the optimized baseline. Each follow-up identifies its workload and measured bottleneck.
