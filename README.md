# Feature-space analysis CLI

This project analyzes an SPL specification, derives the valid systems,
constructs the derived feature space over those systems, and writes reports
about the resulting derived features.

The CLI supports two usage styles:

- **direct mode** for users who know exactly what they want
- **guided mode** for users who want the tool to ask for the required inputs
  and recommend an appropriate report plan

## Build

Build the CLI:

```bash
make cli
```

Run the full regression-tested suite:

```bash
make test
```

Run the benchmark executables:

```bash
make bench
```

Show the Makefile target overview:

```bash
make help
```


## Dependencies

Install the build and documentation toolchain for the current platform with:

```bash
make deps
```

The bootstrap target dispatches automatically to the platform-specific script:

- macOS -> `scripts/install-deps-macos.sh`
- Debian -> `scripts/install-deps-debian.sh`
- Ubuntu -> `scripts/install-deps-ubuntu.sh`
- Arch Linux -> `scripts/install-deps-arch.sh`
- Windows -> `scripts/install-deps-windows.ps1`

The documentation build requires both `doxygen` and Graphviz `dot`.

## CLI quick start

The CLI binary is written to:

```text
testing/generated/cli
```

### Guided mode

Start the wizard explicitly:

```bash
./testing/generated/cli guided
```

### Planning mode

Inspect the derived feature-space estimate and the recommended first step
without writing a report:

```bash
./testing/generated/cli plan spl_specs/new_dsl/valid/unconstrained_2f.spl
```

### Expression mode

Directly output the simplified expression into the cli for the spl spec and given systems:

```bash
./testing/generated/cli expr spl_specs/new_dsl/valid/factorial_paper.spl [Numbers of occuring systems separated by whitespace e.g. 0 1 2]
```

### Direct report generation

Write a summary report:

```bash
./testing/generated/cli analyze spl_specs/new_dsl/valid/unconstrained_2f.spl report.txt -min-diffs=false -feature-names=false --mode=summary
```

Write a focused full report for selected categories:

```bash
./testing/generated/cli analyze spl_specs/new_dsl/valid/factorial_paper.spl report.txt -min-diffs=true -feature-names=true --categories=fundamental,not
```

The relevant flags are documented in:

```bash
./testing/generated/cli --help
./testing/generated/cli analyze --help
./testing/generated/cli plan --help
./testing/generated/cli guided --help
```

## Testing

The project now includes external CLI regression tests in addition to the
existing C++ unit tests.

Run them through:

```bash
make test_cli
```

Or as part of the full suite:

```bash
make test
```
