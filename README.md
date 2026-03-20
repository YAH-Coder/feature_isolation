# Feature-space analysis CLI

This project analyzes an SPL specification, derives the valid systems,
constructs the derived feature space over those systems, and writes reports
about the resulting derived features.

The CLI supports two usage styles:

- **direct mode** for users who know exactly what they want
- **guided mode** for users who want the tool to ask for the required inputs
  and recommend an appropriate report plan

## Terminology

The documentation follows the terminology of `main.pdf` wherever practical.
The CLI still exposes a few serialized labels for compatibility.

- **fundamental features**: the Boolean features declared in the SPL
  specification
- **valid systems**: the systems that satisfy the asserted constraints of the
  SPL specification
- **derived feature space**: the feature space induced from the valid system
  space
- **derived features**: the elements of that derived feature space
- **feature categories**: impossible, core, fundamental, not, and, or,
  complex; these are the CLI/report serialization labels corresponding to the
  paper's category names
- **complex-feature details**: expensive per-feature detail generation that is
  often unnecessary for a first report

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

Or simply run the tool without enough positional arguments and let it enter
guided mode automatically:

```bash
./testing/generated/cli
```

### Planning mode

Inspect the derived feature-space estimate and the recommended first step
without writing a report:

```bash
./testing/generated/cli plan spl_specs/new_dsl/valid/unconstrained_2f.spl
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

## Recommended workflow

For a new SPL specification, the recommended workflow is usually:

1. run `plan` or guided mode
2. inspect the number of fundamental features, valid systems, and derived
   features
3. start with a **summary** or **non-complex** report if the derived feature
   space is large
4. drill down into selected categories or isolation output only when needed

The CLI planner and guided mode already help with this decision and can
recommend lighter first steps when a full report would be expensive.

## Safety and approval model

The CLI distinguishes between:

- the **requested** report plan
- the **recommended** report plan

For expensive runs, the CLI can:

- recommend a lighter first report
- ask for confirmation before continuing
- refuse oversized non-interactive runs unless explicitly overridden

The relevant flags are documented in:

```bash
./testing/generated/cli --help
./testing/generated/cli analyze --help
./testing/generated/cli plan --help
./testing/generated/cli guided --help
```

## Notes on DSL hints and CLI overrides

The SPL specification may contain a `CATEGORY_FILTER` hint. The CLI treats that
as a compatibility input, not as the final execution policy.

If the current command line requests a different report mode or category scope,
the CLI settings take precedence for the current run and the tool explains that
override explicitly.

The repository DSL also supports `COUNT(...)` as a surface syntax for
cardinality-style constraints. That syntax is a repository encoding of the
paper's cardinality-constraint idea; it is not the notation used in `main.pdf`.

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
