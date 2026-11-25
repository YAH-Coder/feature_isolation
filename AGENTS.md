# Feature Isolation

This C++23 CLI analyzes software product line (SPL) specifications. The
supported platforms are macOS and Linux. Start with the [README](README.md)
for build instructions and usage examples.

## Branch workflow

- Use `development` to integrate work before release.
- Create a branch from `development` for each feature or fix. Use a descriptive
  name such as `feature/argouml-summary`.
- Merge a finished branch into `development` after its relevant checks pass.
- Merge `development` into `main` when the intended work is complete and the
  full test suite passes on macOS and Linux.

## Requirements and tests

- Use the [requirements index](docs/requirements/README.md) to find the
  expected behavior and its stable ID. The EARS statements describe observable
  outcomes. The Given/When/Then examples provide acceptance cases.
- Update the relevant requirement and its linked test in the same change when
  an approved behavior changes. Include the requirement ID in the test name
  or description.
- When requirements, code, and tests disagree, use the documented decision or
  discrepancy to determine the intended outcome. Ask for a decision when
  the intended behavior is unresolved.
- Build the CLI with `make cli`. Run its regressions with `make test_cli` and
  the full suite with `make test`. Use `make help` for individual targets.
- Record verification results and distinguish existing failures from
  regressions introduced by the change. See the
  [verification notes](docs/requirements/verification.md#known-discrepancies).
