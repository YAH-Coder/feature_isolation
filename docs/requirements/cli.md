# CLI and guided mode

The CLI accepts requests from the command line or terminal prompts. Its workflow pages specify the results for each command.

## Commands

### CLI-001 — Direct invocation

When a user invokes a supported command or alias, the CLI shall perform the corresponding workflow with the supplied inputs.

When a user invokes direct `analyze`, the CLI shall accept the two positional switches in the order `-min-diffs` then `-feature-names`, with values `true` or `false`. When a switch is repeated in the following options, the CLI shall use its last value.

**Scenario outline — Select a workflow**

- **Given** a readable `model.spl` and a writable destination where the command requires one.
- **When** the user enters an invocation below.
- **Then** the CLI produces the corresponding result.

| Invocation | Result |
| --- | --- |
| `cli analyze model.spl report.txt -min-diffs=false -feature-names=false` | Generates the requested [report](reports.md) if admitted. |
| `cli model.spl report.txt -min-diffs=false -feature-names=false` | Generates the same report and emits a legacy-invocation note to standard error. |
| `cli plan model.spl` | Prints [estimates and recommendations](planning.md) to standard output. |
| `cli expr model.spl 0 1` or `cli expression model.spl 0 1` | Prints an [expression](expressions.md) to standard output. |
| `cli traces model.spl traces.txt --max-grade=1` | Writes a [trace lookup](traces.md) to `traces.txt`. |

[Argument contract](../../src/feature_isolation.cpp#L1426) · [Legacy invocation scenario](../../src/cli_regression_test.cpp#L509)

### CLI-002 — Help

When a user invokes `--help`, `-h`, or `help`, the CLI shall print general help to standard output and exit with status `0`.

When a user places a help token before or after `analyze`, `plan`, `traces`, `guided`, or `interactive`, the CLI shall print help for that command. The [expression help decision](expressions.md#expr-d02--invalid-arguments-and-guided-input) covers `expr`.

**Scenario — Get help for a command**

- **Given** the CLI is available and the user has supplied no SPL file.
- **When** the user enters `cli help traces` or `cli traces --help`.
- **Then** standard output describes the trace command and the exit status is `0`.

[Help dispatch](../../src/feature_isolation.cpp#L3046) · [Help regression](../../src/cli_regression_test.cpp#L360)

### CLI-003 — Report options

When a user supplies report options to `analyze` or `plan`, the CLI shall apply them to the report request as shown below.

If a category list contains an empty or unknown item, then the CLI shall reject the list and report an argument error. When the list repeats a category, the CLI shall treat it as one selection.

| Option | Values and effect |
| --- | --- |
| `--mode=<mode>` | `full`, `summary`, or `isolation-only`. See [report modes](reports.md). |
| `--categories=<list>` | Lowercase category names separated by commas, with no spaces. The list replaces the SPL category hint. |
| `--raw-terms=<bool>`, `--simplified-terms=<bool>` | Set whether applicable report tables include the corresponding term columns. |
| `--isolation=<bool>` | Sets whether applicable reports print isolation results. |
| `-min-diffs=<bool>`, `-feature-names=<bool>` | Select the witness mode and whether to show names in [isolation results](isolation.md). |
| `--max-runtime-ms=<number>`, `--max-output-bytes=<number>` | Set non-negative numeric [admission thresholds](planning.md). |
| `--yes`, `--accept-recommended`, `--non-interactive`, `--force` | Control [report admission](planning.md) for `analyze` only. `--yes` and `--accept-recommended` are mutually exclusive. |

The category names are `impossible`, `core`, `fundamental`, `not`, `and`, `or`, and `complex`. The aliases `not-feature`, `and-feature`, and `or-feature` select their corresponding categories.

When the SPL has a nonempty category hint and the invocation supplies a mode, category, term, or isolation override, the CLI shall emit a compatibility note. The note goes to standard error for `analyze` and standard output for `plan`.

**Scenario — Replace a category hint**

- **Given** a model with a category hint for `Core`.
- **When** the user runs `plan` with `--categories=fundamental,fundamental,not-feature`.
- **Then** the request selects `Fundamental` and `Not` and the compatibility note appears on standard output.

[Category values](../../src/feature_isolation.cpp#L162) · [Option validation](../../src/feature_isolation.cpp#L1556) · [Compatibility evidence](../../src/cli_regression_test.cpp#L509)

## Guided mode

### CLI-004 — Entry and inputs

While standard input is a terminal, when the CLI receives no arguments, an explicit `guided` or `interactive` request, or an incomplete `analyze` or `plan` request, the CLI shall offer guided input.

When guided input starts, the CLI shall prompt on standard error for the workflow and the required paths. It shall use supplied command, path, and Boolean values as prompt defaults where applicable. Without a command seed, the default shall be `analyze`. Without an output path seed, the default for `analyze` shall be `report.txt`. The user can select `analyze` with `a` and `plan` with `p`. The [expression page](expressions.md) covers the open guided `expr` behavior.

**Scenario — Start a seeded planning session**

- **Given** a terminal and a readable `model.spl`.
- **When** the user invokes `cli guided plan model.spl` and accepts the command and path defaults.
- **Then** the wizard offers a report scope for planning and asks for no output path.

[Guided entry and seeds](../../src/feature_isolation.cpp#L745) · [Guided inputs](../../src/feature_isolation.cpp#L1350) · [Terminal regression](../../src/cli_regression_test.cpp#L543)

### CLI-005 — Report choices

When the guided workflow has a valid model for `analyze` or `plan`, the CLI shall show a model overview and offer `Summary`, `Full without complex features`, `Full detailed`, and `Isolation only`, with a recommended default.

When a user chooses a full preset, the CLI shall default to simplified terms enabled and raw terms and isolation disabled. When a user opens advanced options for a full preset, the CLI shall offer category, term, and isolation choices and apply the selected category option as labelled. When advanced options apply to a report with isolation, the CLI shall offer the minimal-witness and feature-name switches.

When a user submits a blank reply to a prompt with a default, the CLI shall apply the default. If the user supplies an invalid prompt answer, then the CLI shall request another answer. If terminal input closes before the wizard completes, then the CLI shall exit the wizard. When the request is ready, the CLI shall summarize it on standard error before dispatching the selected workflow.

**Scenario outline — Apply a guided category choice**

- **Given** a readable model with no category hint and advanced options enabled for a full report.
- **When** the user selects an advertised category option.
- **Then** the wizard applies the corresponding selection and includes it in the run summary.

| Preset | Advertised option | Expected selection |
| --- | --- | --- |
| `Full detailed` | All feature categories | All seven categories |
| Either full preset | All non-complex feature categories | All categories except `Complex` |
| Either full preset | Fundamental and NOT features only | `Fundamental`, `Not` |
| Either full preset | Choose feature categories manually | The choices entered at the category prompts |

The current category menu fails some of these examples. See [CLI-D04](#known-discrepancies). [Guided report workflow](../../src/feature_isolation.cpp#L1085)

## Results and failures

### CLI-006 — Status and diagnostics

When help or a supported workflow completes successfully, the CLI shall return `0`.

If a command cannot proceed for one of the conditions below, then the CLI shall return the corresponding status and report the reason on standard error. Invalid arguments may also produce usage help.

| Condition | Exit status |
| --- | --- |
| A workflow cannot open its SPL file, or `traces` cannot open its output path | `2` |
| `analyze` cannot open its output path | `3` |
| Unknown report or trace option, unsuccessful guided input, or a trace grade or option error | `4` |
| Required report approval is absent or cancelled | `6` |
| Report generation is declined at the admission-threshold step | `8` |

**Scenario — Reject an unknown option**

- **Given** a readable `model.spl`.
- **When** the user runs `cli plan model.spl --unknown`.
- **Then** the CLI returns `4` and identifies the unknown option on standard error.

Direct parse and numeric conversion failures have inconsistent outcomes. See [CLI-D01](#open-decisions). The report writer's reserved status `7` is outside the supported CLI modes. [Status dispatch](../../src/feature_isolation.cpp#L3046) · [Report writer](../../src/feature_isolation.cpp#L2315) · [Guardrail regression](../../src/cli_regression_test.cpp#L454)

### CLI-007 — Report destination side effects

When an admitted `analyze` request completes successfully, the CLI shall write the selected report to the specified path and replace any previous report at that path.

When `plan` or `expr` completes, the CLI shall return the result on standard output without creating a report file.

**Scenario — Write an approved report**

- **Given** a valid model, an admissible summary request, and a writable `report.txt` containing an earlier report.
- **When** the user runs `analyze` for that summary.
- **Then** `report.txt` contains the new report.

[Report destination evidence](../../src/feature_isolation.cpp#L2670) · [Planning output](../../src/feature_isolation.cpp#L2730)

## Open decisions

- **CLI-D01 — Invalid input status.** Define a consistent outcome for missing arguments, invalid SPL content, malformed numeric options, and failures after a file is opened. Direct workflows can currently terminate with an uncaught exception. Guided parsing reports failure with status `4`. [Entry points](../../src/feature_isolation.cpp#L1350)
- **CLI-D02 — Destination on failure.** Decide what must happen to an existing destination when analysis is cancelled, declined, or fails, and whether the input and output paths may coincide. Current `analyze` runs can truncate the destination before approval and accept identical paths. [Report execution](../../src/feature_isolation.cpp#L2670)
- **CLI-D03 — Guided expression input.** Decide whether guided `expr` should ask for IDs and provide help. Current `expr` and `e` answers accept a path and submit an empty selection. [Expression requirements](expressions.md)

## Known discrepancies

- **CLI-D04 — Code discrepancy.** The advertised guided category choices define the target in [CLI-005](#cli-005--report-choices). The full detailed menu currently keeps all categories for its first two choices, selects all non-complex categories for the third, and selects only `Fundamental` and `Not` for the fourth. The non-complex menu keeps all non-complex categories for its first two choices and selects `Fundamental` and `Not` for the third. Neither menu opens manual selection. [Category selection evidence](../../src/feature_isolation.cpp#L1203)
