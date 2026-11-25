---
title: "Compute the exact ArgoUML summary"
milestone: M3
status: Draft
assignee: ""
---

# SCALE-001 — Compute the exact ArgoUML summary

- **Depends on** [TST-002](TST-002.md)
- **Scope** Add the end-to-end test for `REPORT-008` before changing the counting path. The test should reproduce the current integer overflow and then pass through the normal `analyze --mode=summary` entry point. Give the summary a counting path independent of exhaustive feature generation. [ArgoUML scenario](../../requirements/reports.md#report-008--exact-argouml-summary) · [Registry construction](../../../src/feature_analysis.cpp#L3088)
- **Done when** The CLI exits successfully and reports eight fundamental features, 256 valid and available systems, zero missing systems, exactly $2^{256}$ derived features, and all seven exact category counts. The expected value for each of `And` and `Or` is $3^8 - 1 - 2\cdot8 = 6{,}544$, counting distinct conjunctions or disjunctions of two or more literals on an unconstrained basis. Compare exact summary counts with exhaustive reports on small unconstrained and constrained fixtures, including a fixture with missing systems. Check the existing summary format and category filter behavior. [Summary contract](../../requirements/reports.md#report-003--totals-and-category-selection)
