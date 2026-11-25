# Build and verification

These requirements cover the project's public build and verification commands. The behavioral examples in this directory provide acceptance criteria for the CLI and library.

## VERIFY-001 — Build and platform coverage

When a developer runs `make cli` or `make feature_isolation_cli` with the default output configuration, the build shall produce the C++23 command-line application at `testing/generated/cli`.

When a developer runs `make deps` on macOS, Debian, Ubuntu, or Arch Linux, the build tooling shall provide the corresponding dependency bootstrap. When a pull request is opened or updated, or the `main` branch receives a push, CI shall build and run the default tests on Linux and macOS.

**Scenario — Build the CLI**

- **Given** the repository root and the required toolchain.
- **When** a developer runs `make cli`.
- **Then** `testing/generated/cli --help` prints the usage information.

[Build target](../../Makefile#L75) · [Dependency bootstrap](../../scripts/install-deps.sh) · [CI workflow](../../.github/workflows/test.yml)

## VERIFY-002 — Test entry points

When a developer runs `make test`, the build tooling shall build and run the default arithmetic, SPL, analysis, isolation, and CLI test suites. Each suite shall have an individual target.

When a developer requests the isolation theory tests explicitly, the test executable shall include the tagged theory cases. The default test selection shall exclude cases marked hidden.

**Scenario outline — Select isolation tests**

- **Given** the built isolation test executable.
- **When** a developer runs a command below from the repository root.
- **Then** the test runner selects the corresponding cases.

| Command | Selected cases |
| --- | --- |
| `make test_isolation` | Default isolation cases. |
| `./testing/isolation/test_isolation '[theory]'` | Theory cases, including the missing-system ambiguity cases. |

[Default targets](../../Makefile#L81) · [Theory cases](../../src/isolation_test.cpp#L332)

The current default CLI suite covers simple trace exports at grades 1 and 2. The [expression](expressions.md) and [complex trace](traces.md) examples can be exercised directly. [CLI regression cases](../../src/cli_regression_test.cpp#L650)

## VERIFY-003 — Benchmarks

When a developer runs `make bench`, the build shall produce `testing/mathhelper/benchmark` and `testing/analysis/theory_benchmark` as separate executables.

When a developer runs the theory benchmark from the repository root, the benchmark shall report the median time per iteration for its configured scenarios. The current rows cover access to a computed system space, construction of feature analysis, and one isolation workload. [Benchmark targets](../../Makefile#L98) · [Theory scenarios](../../src/theory_benchmark.cpp#L123)

**Scenario — Run a theory benchmark**

- **Given** the repository and its build toolchain.
- **When** a developer runs `make bench` followed by `./testing/analysis/theory_benchmark`.
- **Then** the output reports a scenario, operation, iterations per sample, and median milliseconds per iteration.

## VERIFY-004 — Reference documentation

When a developer runs `make docs` with Doxygen and Graphviz available, the documentation build shall produce HTML under `docs/html`. The requirements reference shall be available as Markdown under `docs/requirements/`.

**Scenario — Generate the reference site**

- **Given** the repository and documentation tools.
- **When** a developer runs `make docs`.
- **Then** `docs/html/index.html` is available as the generated reference entry point.

[Documentation target](../../Makefile#L102) · [Doxygen settings](../../Doxyfile#L1)

## Known discrepancies

- The default analysis suite expects CNF for some complex terms. The current DNF behavior is the target defined in [expressions](expressions.md).
- The default CLI suite expects a unique trace key for one missing-system example. Multiple entries for that key are the target defined in [traces](traces.md).
- The opt-in isolation theory suite fails two assertions about the missing-system ambiguity example. The intended empty-intersection result is an open decision in [isolation](isolation.md).

## Open decisions

- **VERIFY-D01 — Resource limits.** The specification sets no measured execution-time or peak-memory limit. The [exact ArgoUML summary](reports.md#report-008--exact-argouml-summary) defines the model and required result. [Planning estimates](planning.md)

## Scope decisions

- **VERIFY-D02 — Supported platforms.** The current scope is macOS and Linux. [VERIFY-001](#verify-001--build-and-platform-coverage) defines the dependency bootstrap and CI coverage. [CI matrix](../../.github/workflows/test.yml#L9)
