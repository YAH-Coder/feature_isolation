---
title: "Make builds reproducible"
milestone: M1
status: Draft
assignee: ""
---

# BUILD-001 — Make builds reproducible

- **Depends on** [TST-002](TST-002.md)
- **Scope** Make the build track header changes for tests and benchmarks. Keep debug and optimized builds in distinct output directories. Coordinate CLI executable selection with [TST-002](TST-002.md). [Makefile](../../../Makefile#L59) · [Build contract](../../requirements/verification.md#verify-001--build-and-platform-coverage)

**Done when**

- A header change rebuilds the relevant test and benchmark binaries.
- A clean default build and a separate optimized build both produce and test their own artifacts.
- The supported macOS and Linux commands and output locations are documented.
