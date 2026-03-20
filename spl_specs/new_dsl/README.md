# New DSL fixtures

This directory contains the active SPL fixtures for the theory-native Boolean
DSL.

- `valid/` contains canonical positive fixtures used by parser, semantic, and
  isolation tests.
- `invalid/` contains negative fixtures that are expected to fail during
  parsing or semantic validation.

Canonical paper fixtures:
- `valid/unconstrained_2f.spl` — unconstrained two-feature running example
- `valid/fx_implies_fy.spl` — constrained two-feature example with `fx -> fy`
- `valid/fx_implies_fy_missing_s0.spl` — missing-system paper example where
  `s0` is unavailable
- `valid/factorial_paper.spl` — factorial example from the paper

Compatibility note:
- `valid/missing_systems_example.spl` is kept as an alias of the paper scenario.

Current concrete syntax:
- `SPL`
- `FEATURES`
- `CONSTRAINTS` with `ASSERT` formulas
- `AVAILABLE_SYSTEMS`
- `MISSING_SYSTEMS`
- `CATEGORY_FILTER`

The active DSL intentionally covers only the Boolean core described in the paper.
Attributes and repeated-feature multiplicity declarations are out of scope.
