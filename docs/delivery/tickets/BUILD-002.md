---
title: "Separate reusable isolation definitions"
milestone: M1
status: Draft
assignee: ""
---

# BUILD-002 — Separate reusable isolation definitions

- **Depends on** [TST-006](TST-006.md)
- **Scope** Fix the duplicate-symbol link failure when two translation units include `isolation.hpp`. Move non-inline function definitions into a compiled source file while preserving the public isolation contract. [Header definitions](../../../include/isolation.hpp#L342) · [Isolation requirements](../../requirements/isolation.md)

**Done when**

- A link check includes the header from two translation units and succeeds.
- The selected isolation and CLI acceptance tests still pass after the move.
