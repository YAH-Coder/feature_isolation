/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "../include/isolation.hpp"
#include "../include/spl.hpp"
#include <array>
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

/** @file
 *  @brief Implements the CLI, guided workflow, guardrails, and report-writing entry points.
 */

using namespace std;

using namespace feature_isolation;

namespace
{
  /** @brief Distinguishes the supported top-level CLI subcommands. */
  enum class cli_command_t
  {
      analyze,
      plan,
      expression_from_systems,
      trace_export,
  };

  /** @brief Holds parsed command-line options and guided-mode selections. */
  struct cli_options_t final
  {
      cli_command_t              command { cli_command_t::analyze };
      string                     specification_path {};
      string                     result_path {};
      bool                       requires_result_path { true };
      bool                       min_diffs { false };
      bool                       feature_names { false };
      bool                       assume_yes { false };
      bool                       accept_recommended { false };
      bool                       non_interactive { false };
      bool                       legacy_invocation { false };
      bool                       force { false };
      std::optional<long double> max_runtime_ms {};
      std::optional<long double> max_output_bytes {};
      std::optional<report_mode_t> mode_override {};
      std::optional<std::vector<theory_feature_category_t>> category_override {};
      std::optional<bool>        include_raw_terms_override {};
      std::optional<bool>        include_simplified_terms_override {};
      std::optional<bool>        include_isolation_override {};
      std::vector<string> found_systems_in_alignment {};
      std::optional<unsigned_integer> max_grade {};
  };

  /** @brief Captures hard runtime and output-size limits used before report generation. */
  struct execution_guardrails_t final
  {
      long double max_runtime_ms { 300000.0L };
      long double max_output_bytes { 1024.0L * 1024.0L * 1024.0L };
  };

  /** @brief Returns the stable category order used by planner and report summaries. */
  [[nodiscard]] const std::array<theory_feature_category_t, 7>&
      ordered_theory_categories_for_reports()
  {
    static const std::array<theory_feature_category_t, 7> categories {
      theory_feature_category_t::impossible,
      theory_feature_category_t::core,
      theory_feature_category_t::fundamental,
      theory_feature_category_t::not_feature,
      theory_feature_category_t::and_feature,
      theory_feature_category_t::or_feature,
      theory_feature_category_t::complex
    };
    return categories;
  }

  /** @brief Returns whether stdin is attached to an interactive terminal. */
  [[nodiscard]] bool stdin_is_terminal()
  {
#if defined(_WIN32)
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(fileno(stdin)) != 0;
#endif
  }

  /** @brief Returns a trimmed copy of user input or CLI fragments. */
  [[nodiscard]] std::string trim_copy(const std::string& text)
  {
    const auto is_whitespace = [](const char character) {
      return character == ' ' || character == '\t' || character == '\r'
             || character == '\n';
    };

    std::size_t first = 0;
    while (first < text.size() && is_whitespace(text[first]))
    {
      ++first;
    }
    if (first == text.size())
    {
      return {};
    }

    std::size_t last = text.size();
    while (last > first && is_whitespace(text[last - 1]))
    {
      --last;
    }
    return text.substr(first, last - first);
  }

  /** @brief Maps one theory-feature category to the CLI token used in options and prompts. */
  [[nodiscard]] std::string cli_category_name(theory_feature_category_t category)
  {
    switch (category)
    {
      case theory_feature_category_t::impossible:
        return "impossible";
      case theory_feature_category_t::core:
        return "core";
      case theory_feature_category_t::fundamental:
        return "fundamental";
      case theory_feature_category_t::not_feature:
        return "not";
      case theory_feature_category_t::and_feature:
        return "and";
      case theory_feature_category_t::or_feature:
        return "or";
      case theory_feature_category_t::complex:
        return "complex";
      case theory_feature_category_t::unclassified:
        return "unclassified";
    }
    return "unknown";
  }

  /** @brief Parses one CLI category token into the internal category enum. */
  [[nodiscard]] std::optional<theory_feature_category_t>
      parse_cli_category_name(const std::string& token)
  {
    if (token == "impossible")
    {
      return theory_feature_category_t::impossible;
    }
    if (token == "core")
    {
      return theory_feature_category_t::core;
    }
    if (token == "fundamental")
    {
      return theory_feature_category_t::fundamental;
    }
    if (token == "not" || token == "not-feature")
    {
      return theory_feature_category_t::not_feature;
    }
    if (token == "and" || token == "and-feature")
    {
      return theory_feature_category_t::and_feature;
    }
    if (token == "or" || token == "or-feature")
    {
      return theory_feature_category_t::or_feature;
    }
    if (token == "complex")
    {
      return theory_feature_category_t::complex;
    }
    return std::nullopt;
  }

  /** @brief Formats the valid-system subset represented by one feature row in CLI output. */
  [[nodiscard]] std::string system_membership_set(const feature_t& systems_present,
                                                  const system_space_t& system_space)
  {
    std::vector<unsigned_integer> ordered_valid_system_ids(
        system_space.valid.begin(), system_space.valid.end());
    std::ostringstream os;
    bool first = true;
    os << "{";
    for (auto system_index { zero }; system_index < ordered_valid_system_ids.size();
         ++system_index)
    {
      if (!systems_present.at(system_index))
      {
        continue;
      }
      if (!first)
      {
        os << ", ";
      }
      os << paper_system_label(ordered_valid_system_ids.at(system_index));
      first = false;
    }
    os << "}";
    return first ? std::string { "∅" } : os.str();
  }

  /** @brief Extracts a user-facing program name from argv[0]. */
  [[nodiscard]] std::string program_display_name(const char* argv0)
  {
    if (argv0 == nullptr)
    {
      return "cli";
    }
    const std::string raw { argv0 };
    const auto slash = raw.find_last_of("/\\");
    if (slash == std::string::npos)
    {
      return raw.empty() ? std::string { "cli" } : raw;
    }
    const auto tail = raw.substr(slash + 1);
    return tail.empty() ? std::string { "cli" } : tail;
  }

  /** @brief Returns whether a token requests help output. */
  [[nodiscard]] bool is_help_token(const std::string& token)
  {
    return token == "--help" || token == "-h" || token == "help";
  }

  /** @brief Returns whether a token selects the guided workflow alias. */
  [[nodiscard]] bool is_guided_token(const std::string& token)
  {
    return token == "guided" || token == "interactive";
  }

  /** @brief Writes the top-level CLI help text. */
  void print_general_help(std::ostream& os, const std::string& program_name)
  {
    os << "Derived-feature-space report generator\n\n"
       << "Usage\n"
       << "  " << program_name << " analyze <spl-file> <output-file>"
       << " -min-diffs=<false|true> -feature-names=<false|true> [options]\n"
       << "  " << program_name << " plan <spl-file> [options]\n"
       << "  " << program_name << " traces <spl-file> <output-file> [--max-grade=N]\n"
       << "  " << program_name << " guided [analyze|plan] [seed-inputs]\n"
       << "  " << program_name << " --help\n"
       << "  " << program_name << " analyze --help\n"
       << "  " << program_name << " plan --help\n"
       << "  " << program_name << " traces --help\n"
       << "  " << program_name << " guided --help\n"
       << "\n"
       << "Quick start\n"
       << "  " << program_name << "\n"
       << "      Start guided mode when you want the tool to ask for the SPL path,\n"
       << "      report path, and a suitable first report mode.\n"
       << "  " << program_name << " plan <spl-file>\n"
       << "      Inspect the valid systems, the derived feature space, and the\n"
       << "      recommended first report without writing a file.\n"
       << "  " << program_name << " analyze <spl-file> <output-file> ...\n"
       << "      Generate a report directly when you already know the mode you want.\n"
       << "\n"
       << "Commands\n"
       << "  analyze   Parse the SPL specification, estimate the resulting derived feature space, and write a report file.\n"
       << "  plan      Inspect the valid systems, the derived feature space, and the planner recommendation without writing a report.\n"
       << "  traces    Precompute a bounded, grade-limited lookup file mapping system-id\n"
       << "            combinations to their derived-feature expression.\n"
       << "  guided    Start the interactive wizard explicitly. Optionally seed it with\n"
       << "            'analyze' or 'plan' and partial positional inputs.\n"
       << "  interactive\n"
       << "            Compatibility alias for 'guided'.\n"
       << "\n"
       << "Compatibility\n"
       << "  The legacy positional invocation is still supported and behaves like 'analyze':\n"
       << "    " << program_name << " <spl-file> <output-file>"
       << " -min-diffs=<false|true> -feature-names=<false|true> [options]\n"
       << "  New scripts should prefer the explicit subcommands shown above.\n"
       << "\n"
       << "Common options\n"
       << "  --mode=full|summary|isolation-only\n"
       << "      Select the report mode explicitly. CLI choices override any\n"
       << "      CATEGORY_FILTER hint stored in the SPL specification.\n"
       << "  --categories=<comma-separated-list>\n"
       << "      Restrict the report to feature categories such as\n"
       << "      impossible,core,fundamental,not,and,or,complex.\n"
       << "  --max-runtime-ms=<N>\n"
       << "      Set the hard runtime guardrail used by the planner and executor.\n"
       << "  --max-output-bytes=<N>\n"
       << "      Set the hard output-size guardrail used by the planner and executor.\n"
       << "\nExamples\n"
       << "  " << program_name << " plan spl_specs/new_dsl/valid/unconstrained_2f.spl\n"
       << "  " << program_name
       << " analyze spl_specs/new_dsl/valid/unconstrained_2f.spl report.txt"
       << " -min-diffs=false -feature-names=false --mode=summary\n"
       << "  " << program_name
       << " analyze spl_specs/new_dsl/valid/factorial_paper.spl report.txt"
       << " -min-diffs=true -feature-names=true --categories=fundamental,not\n"
       << "  " << program_name << " guided analyze spl_specs/new_dsl/valid/unconstrained_2f.spl\n"
       << "\nNotes\n"
       << "  Running the program without enough positional arguments starts guided mode.\n"
       << "  You can also start the wizard explicitly with '" << program_name
       << " guided'.\n"
       << "  Prefer '" << program_name << " analyze ...' for direct report generation\n"
       << "  and '" << program_name << " plan ...' for estimation-only runs.\n"
       << "  The planner reports the number of fundamental features, valid systems, and\n"
       << "  derived features induced by those valid systems.\n"
       << "  Large derived feature spaces often benefit from a summary-first or\n"
       << "  non-complex drill-down workflow.\n";
  }


  /** @brief Writes subcommand help for direct report generation. */
  void print_analyze_help(std::ostream& os, const std::string& program_name)
  {
    os << "Usage\n"
       << "  " << program_name << " analyze <spl-file> <output-file>"
       << " -min-diffs=<false|true> -feature-names=<false|true> [options]\n"
       << "\nRequired positional arguments\n"
       << "  <spl-file>      Path to the SPL specification in the current DSL.\n"
       << "  <output-file>   Path of the report file to write.\n"
       << "\nRequired analyze toggles\n"
       << "  -min-diffs=<false|true>\n"
       << "      Control whether only minimal witness differences are printed in isolation output.\n"
       << "  -feature-names=<false|true>\n"
       << "      Control whether feature names are included in the isolation output.\n"
       << "\nAnalyze options\n"
       << "  --mode=full|summary|isolation-only\n"
       << "      Select the report shape written to the output file.\n"
       << "  --categories=<list>\n"
       << "      Restrict the written registry to selected feature categories.\n"
       << "  --yes\n"
       << "      Keep the requested report plan without prompting.\n"
       << "  --accept-recommended\n"
       << "      Accept the planner's lighter recommended report plan without prompting.\n"
       << "  --non-interactive\n"
       << "      Never prompt. If approval is required and no override flag is given, the command aborts.\n"
       << "  --force\n"
       << "      Bypass hard guardrails after explicit approval.\n"
       << "  --max-runtime-ms=<N>\n"
       << "      Override the hard runtime guardrail for this run.\n"
       << "  --max-output-bytes=<N>\n"
       << "      Override the hard output-size guardrail for this run.\n"
       << "\nExamples\n"
       << "  " << program_name
       << " analyze spec.spl report.txt -min-diffs=false -feature-names=false\n"
       << "  " << program_name
       << " analyze spec.spl report.txt -min-diffs=true -feature-names=true --mode=summary\n"
       << "  " << program_name
       << " analyze spec.spl report.txt -min-diffs=true -feature-names=true"
       << " --accept-recommended --non-interactive\n";
  }

  /** @brief Writes subcommand help for planning-only mode. */
  void print_plan_help(std::ostream& os, const std::string& program_name)
  {
    os << "Usage\n"
       << "  " << program_name << " plan <spl-file> [options]\n"
       << "\nRequired positional arguments\n"
       << "  <spl-file>      Path to the SPL specification in the current DSL.\n"
       << "\nOptional planning inputs\n"
       << "  -min-diffs=<false|true>\n"
       << "      Include the isolation witness style in the requested execution plan.\n"
       << "  -feature-names=<false|true>\n"
       << "      Include feature names in any isolation-oriented recommendation.\n"
       << "  --mode=full|summary|isolation-only\n"
       << "      Ask the planner to evaluate a specific report mode.\n"
       << "  --categories=<list>\n"
       << "      Ask the planner to evaluate a category-restricted report.\n"
       << "  --max-runtime-ms=<N>\n"
       << "      Override the hard runtime guardrail shown in the planning output.\n"
       << "  --max-output-bytes=<N>\n"
       << "      Override the hard output-size guardrail shown in the planning output.\n"
       << "\nExamples\n"
       << "  " << program_name << " plan spec.spl\n"
       << "  " << program_name << " plan spec.spl --mode=summary\n"
       << "  " << program_name << " plan spec.spl --categories=fundamental,not\n"
       << "\nNotes\n"
       << "  The planning output is phrased in terms of fundamental features, valid systems, and derived features.\n"
       << "  No report file is written by the plan command.\n";
  }

  /** @brief Writes subcommand help for the grade-bounded trace export. */
  void print_traces_help(std::ostream& os, const std::string& program_name)
  {
    os << "Usage\n"
       << "  " << program_name << " traces <spl-file> <output-file> [--max-grade=N]\n"
       << "\nRequired positional arguments\n"
       << "  <spl-file>      Path to the SPL specification in the current DSL.\n"
       << "  <output-file>   Path of the trace-lookup file to write.\n"
       << "\nOptions\n"
       << "  --max-grade=<N>\n"
       << "      Maximum number of literals combined per AND/OR term (grade 1 = a\n"
       << "      single fundamental feature or its negation). If omitted and the\n"
       << "      program is running interactively, you will be prompted for it.\n"
       << "\nOutput format\n"
       << "  One line per precomputed trace: a comma-separated, ascending list of\n"
       << "  available system ids in the paper-style 0-based convention (matching\n"
       << "  the s0/s1/... labels used elsewhere in report output, i.e. one less\n"
       << "  than the id used in the .spl file's AVAILABLE_SYSTEMS/MISSING_SYSTEMS\n"
       << "  sections), followed by \": \" and the trace's rendered boolean\n"
       << "  expression (e.g. \"0,3,4: A\"). The core tautology (present in every\n"
       << "  valid system) is always included regardless of the selected grade.\n"
       << "  Traces whose available-system set is empty are omitted. This file is\n"
       << "  a bounded, precomputed lookup table intended to be checked first by\n"
       << "  another program before falling back to the 'expr' command for an\n"
       << "  on-demand computation.\n"
       << "\nExamples\n"
       << "  " << program_name << " traces spec.spl traces.txt --max-grade=2\n";
  }

  /** @brief Writes help for the interactive guided workflow. */
  void print_guided_help(std::ostream& os, const std::string& program_name)
  {
    os << "Usage\n"
       << "  " << program_name << " guided [analyze|plan] [seed-inputs]\n"
       << "  " << program_name << " interactive [analyze|plan] [seed-inputs]\n"
       << "\nPurpose\n"
       << "  Start the interactive wizard explicitly instead of reaching it through\n"
       << "  missing positional arguments. 'interactive' is kept as a\n"
       << "  compatibility alias for 'guided'.\n"
       << "\nOptional seeds\n"
       << "  analyze|plan    Preselect the wizard command.\n"
       << "  <spl-file>      Pre-fill the SPL specification path.\n"
       << "  <output-file>   Pre-fill the report path for guided analyze runs.\n"
       << "  -min-diffs=<false|true>\n"
       << "      Pre-fill the minimal-differences isolation toggle.\n"
       << "  -feature-names=<false|true>\n"
       << "      Pre-fill the feature-name isolation toggle.\n"
       << "\nExamples\n"
       << "  " << program_name << " guided\n"
       << "  " << program_name << " guided analyze\n"
       << "  " << program_name << " guided analyze spec.spl report.txt\n"
       << "  " << program_name << " interactive plan spec.spl\n"
       << "\nNotes\n"
       << "  Guided mode still uses the same planner, guardrails, and approval logic\n"
       << "  as the direct command-line workflow.\n";
  }

  /** @brief Parses a boolean option of the form key=true|false. */
  [[nodiscard]] bool parse_bool_toggle(const std::string& option,
                                       const char*        expected_name,
                                       bool&              target,
                                       std::ostream&      errors)
  {
    const std::string prefix = std::string { expected_name } + "=";
    if (option == prefix + "false")
    {
      target = false;
      return true;
    }
    if (option == prefix + "true")
    {
      target = true;
      return true;
    }
    errors << "Unknown option \"" << option << "\".\n"
           << "Expected \"" << expected_name << "=false\" or \""
           << expected_name << "=true\".\n"
           << "Program terminated." << endl;
    return false;
  }

  /** @brief Parses a non-negative floating-point option value. */
  [[nodiscard]] bool parse_nonnegative_long_double(
      const std::string&          option,
      const char*                 expected_name,
      std::optional<long double>& target,
      std::ostream&               errors)
  {
    const std::string prefix = std::string { expected_name } + "=";
    if (!option.starts_with(prefix))
    {
      return false;
    }
    const std::string value_text = option.substr(prefix.size());
    try
    {
      const long double value = std::stold(value_text);
      if (value < 0.0L)
      {
        errors << "Option \"" << expected_name
               << "\" expects a non-negative value.\n"
               << "Program terminated." << endl;
        return false;
      }
      target = value;
      return true;
    }
    catch (const std::exception&)
    {
      errors << "Unknown option \"" << option << "\".\n"
             << "Expected \"" << expected_name
             << "=<non-negative-number>\".\n"
             << "Program terminated." << endl;
      return false;
    }
  }

  /** @brief Parses a positive integer option value, such as "--max-grade=N". */
  [[nodiscard]] bool parse_positive_unsigned_integer(
      const std::string&                 option,
      const char*                        expected_name,
      std::optional<unsigned_integer>&   target,
      std::ostream&                      errors)
  {
    const std::string prefix = std::string { expected_name } + "=";
    if (!option.starts_with(prefix))
    {
      return false;
    }
    const std::string value_text = option.substr(prefix.size());
    try
    {
      std::size_t consumed { 0 };
      const auto  value = std::stoull(value_text, &consumed);
      if (consumed != value_text.size() || value < 1)
      {
        throw std::invalid_argument { "not a positive integer" };
      }
      target = static_cast<unsigned_integer>(value);
      return true;
    }
    catch (const std::exception&)
    {
      errors << "Unknown option \"" << option << "\".\n"
             << "Expected \"" << expected_name << "=<positive-integer>\".\n"
             << "Program terminated." << endl;
      return false;
    }
  }

  /** @brief Parses an explicit report-mode override from the CLI. */
  [[nodiscard]] bool parse_report_mode_override(const std::string& option,
                                                std::optional<report_mode_t>& target,
                                                std::ostream& errors)
  {
    const std::string prefix = "--mode=";
    if (!option.starts_with(prefix))
    {
      return false;
    }
    const auto value = option.substr(prefix.size());
    if (value == "full")
    {
      target = report_mode_t::full;
      return true;
    }
    if (value == "summary")
    {
      target = report_mode_t::summary;
      return true;
    }
    if (value == "isolation-only")
    {
      target = report_mode_t::isolation_only;
      return true;
    }
    errors << "Unknown option \"" << option << "\".\n"
           << "Expected \"--mode=full\", \"--mode=summary\", or"
              " \"--mode=isolation-only\".\n"
           << "Program terminated." << endl;
    return false;
  }

  /** @brief Parses a comma-separated category override list from the CLI. */
  [[nodiscard]] bool parse_category_override(
      const std::string& option,
      std::optional<std::vector<theory_feature_category_t>>& target,
      std::ostream& errors)
  {
    const std::string prefix = "--categories=";
    if (!option.starts_with(prefix))
    {
      return false;
    }

    const auto value = option.substr(prefix.size());
    if (value.empty())
    {
      errors << "Option \"--categories\" expects a comma-separated list.\n"
             << "Program terminated." << endl;
      return false;
    }

    std::vector<theory_feature_category_t> categories {};
    std::size_t start = 0;
    while (start <= value.size())
    {
      const auto comma = value.find(',', start);
      const auto token = value.substr(start,
                                      comma == std::string::npos
                                          ? std::string::npos
                                          : comma - start);
      if (token.empty())
      {
        errors << "Option \"" << option << "\" contains an empty category name.\n"
               << "Program terminated." << endl;
        return false;
      }
      const auto parsed = parse_cli_category_name(token);
      if (!parsed.has_value())
      {
        errors << "Unknown feature category \"" << token << "\" in option \""
               << option << "\".\n"
               << "Program terminated." << endl;
        return false;
      }
      if (std::ranges::find(categories, parsed.value()) == categories.end())
      {
        categories.push_back(parsed.value());
      }
      if (comma == std::string::npos)
      {
        break;
      }
      start = comma + 1;
    }

    target = std::move(categories);
    return true;
  }

  /** @brief Stores partially inferred guided-mode inputs extracted from argv. */
  struct guided_seed_t final
  {
      std::optional<cli_command_t> command {};
      std::string                  specification_path {};
      std::string                  result_path {};
      std::optional<bool>          min_diffs {};
      std::optional<bool>          feature_names {};
  };

  /** @brief Parses guided-mode seed toggles from legacy positional arguments. */
  [[nodiscard]] bool parse_bool_toggle_seed(const std::string& option,
                                            const char*        expected_name,
                                            std::optional<bool>& target)
  {
    const std::string prefix = std::string { expected_name } + "=";
    if (option == prefix + "false")
    {
      target = false;
      return true;
    }
    if (option == prefix + "true")
    {
      target = true;
      return true;
    }
    return false;
  }

  /** @brief Decides whether the program should fall back to guided mode. */
  [[nodiscard]] bool should_enter_guided_mode(int argc, char* argv[])
  {
    if (argc < 2)
    {
      return true;
    }

    const std::string first_argument { argv[1] };
    if (is_help_token(first_argument))
    {
      return false;
    }
    if (is_guided_token(first_argument))
    {
      return true;
    }
    if (first_argument == "analyze")
    {
      return argc < 6;
    }
    if (first_argument == "plan")
    {
      return argc < 3;
    }
    return argc < 5;
  }

  /** @brief Infers initial guided answers from partial positional CLI arguments. */
  [[nodiscard]] guided_seed_t infer_guided_seed(int argc, char* argv[])
  {
    guided_seed_t seed {};
    if (argc < 2)
    {
      return seed;
    }

    int cursor = 1;
    const std::string first_argument { argv[cursor] };
    if (is_guided_token(first_argument))
    {
      ++cursor;
      if (cursor < argc)
      {
        const std::string second_argument { argv[cursor] };
        if (second_argument == "analyze")
        {
          seed.command = cli_command_t::analyze;
          ++cursor;
        }
        else if (second_argument == "plan")
        {
          seed.command = cli_command_t::plan;
          ++cursor;
        }
      }
    }
    else if (first_argument == "analyze")
    {
      seed.command = cli_command_t::analyze;
      ++cursor;
    }
    else if (first_argument == "plan")
    {
      seed.command = cli_command_t::plan;
      ++cursor;
    }
    else if (!is_help_token(first_argument))
    {
      seed.command = cli_command_t::analyze;
    }

    if (cursor < argc)
    {
      seed.specification_path = argv[cursor++];
    }
    if (seed.command.value_or(cli_command_t::analyze) == cli_command_t::analyze && cursor < argc)
    {
      seed.result_path = argv[cursor++];
    }
    if (cursor < argc)
    {
      (void)parse_bool_toggle_seed(argv[cursor], "-min-diffs", seed.min_diffs);
      ++cursor;
    }
    if (cursor < argc)
    {
      (void)parse_bool_toggle_seed(argv[cursor], "-feature-names", seed.feature_names);
    }
    return seed;
  }

  /** @brief Prompts for a free-form text value with optional defaulting and cancellation. */
  [[nodiscard]] std::optional<std::string>
      prompt_text_value(const std::string& prompt,
                        const std::string& default_value = {})
  {
    while (true)
    {
      cerr << prompt;
      if (!default_value.empty())
      {
        cerr << " [" << default_value << "]";
      }
      cerr << ": " << flush;

      std::string line {};
      if (!std::getline(cin, line))
      {
        cerr << "\nInput closed before guided mode finished.\nProgram terminated."
             << endl;
        return std::nullopt;
      }

      const auto trimmed = trim_copy(line);
      if (!trimmed.empty())
      {
        return trimmed;
      }
      if (!default_value.empty())
      {
        return default_value;
      }
      cerr << "A value is required." << endl;
    }
  }

  /** @brief Prompts for a yes/no answer with optional defaulting and cancellation. */
  [[nodiscard]] std::optional<bool>
      prompt_yes_no(const std::string& prompt, const bool default_value)
  {
    while (true)
    {
      cerr << prompt << " [" << (default_value ? "Y/n" : "y/N") << "]: "
           << flush;

      std::string line {};
      if (!std::getline(cin, line))
      {
        cerr << "\nInput closed before guided mode finished.\nProgram terminated."
             << endl;
        return std::nullopt;
      }

      const auto trimmed = trim_copy(line);
      if (trimmed.empty())
      {
        return default_value;
      }
      const auto lowered = static_cast<char>(std::tolower(static_cast<unsigned char>(trimmed.front())));
      if (lowered == 'y')
      {
        return true;
      }
      if (lowered == 'n')
      {
        return false;
      }
      cerr << "Please answer yes or no." << endl;
    }
  }

  /** @brief Prompts for the top-level command used by guided mode. */
  [[nodiscard]] std::optional<cli_command_t>
      prompt_guided_command(const std::optional<cli_command_t> preferred)
  {
    while (true)
    {
      cerr << "Select CLI command";
      if (preferred.has_value())
      {
        cerr << " [" << (preferred.value() == cli_command_t::analyze ? "analyze" : "plan") << "]";
      }
      else
      {
        cerr << " [analyze]";
      }
      cerr << ": " << flush;

      std::string line {};
      if (!std::getline(cin, line))
      {
        cerr << "\nInput closed before guided mode finished.\nProgram terminated."
             << endl;
        return std::nullopt;
      }

      const auto trimmed = trim_copy(line);
      if (trimmed.empty())
      {
        return preferred.value_or(cli_command_t::analyze);
      }
      if (trimmed == "analyze" || trimmed == "a")
      {
        return cli_command_t::analyze;
      }
      if (trimmed == "plan" || trimmed == "p")
      {
        return cli_command_t::plan;
      }
      if (trimmed == "expr" || trimmed == "e")
      {
        return cli_command_t::expression_from_systems;
      }
      cerr << "Please enter 'analyze', 'plan' or 'expr'." << endl;
    }
  }

  [[nodiscard]] report_request_t make_summary_request(const report_request_t& base);
  /** @brief Produces a request restricted to a selected category subset. */
  [[nodiscard]] report_request_t make_category_request(
      const report_request_t& base,
      std::vector<theory_feature_category_t> categories,
      bool include_isolation);
  /** @brief Produces an isolation-focused request for the same model. */
  [[nodiscard]] report_request_t make_isolation_only_request(
      const report_request_t& base);
  [[nodiscard]] std::string request_display_name(const report_request_t& request);
  [[nodiscard]] std::string request_detail_summary(const report_request_t& request);
  void print_feature_theory_overview(std::ostream& os,
                                     const report_feasibility_t& feasibility);
  void print_observations(std::ostream& os,
                          const std::vector<std::string>& observations,
                          std::string_view heading);

  /** @brief Prompts for a numbered menu choice with validation and cancellation. */
  [[nodiscard]] std::optional<std::size_t>
      prompt_numbered_choice(const std::string& heading,
                             const std::vector<std::string>& choices,
                             const std::size_t default_choice)
  {
    while (true)
    {
      cerr << heading << '\n';
      for (std::size_t index = 0; index < choices.size(); ++index)
      {
        cerr << "  " << (index + 1) << ") " << choices.at(index) << '\n';
      }
      cerr << "Select option [" << default_choice << "]: " << flush;

      std::string line {};
      if (!std::getline(cin, line))
      {
        cerr << "\nInput closed before guided mode finished.\nProgram terminated."
             << endl;
        return std::nullopt;
      }

      const auto trimmed = trim_copy(line);
      if (trimmed.empty())
      {
        return default_choice;
      }
      try
      {
        const auto parsed = static_cast<std::size_t>(std::stoul(trimmed));
        if (parsed >= 1 && parsed <= choices.size())
        {
          return parsed;
        }
      }
      catch (...)
      {
      }
      cerr << "Please enter a number between 1 and " << choices.size()
           << "." << endl;
    }
  }

  /** @brief Prompts for a manual category subset when the preset guided choices are insufficient. */
  [[nodiscard]] std::optional<std::vector<theory_feature_category_t>>
      prompt_manual_categories(const bool allow_complex)
  {
    std::vector<theory_feature_category_t> selected {};
    while (selected.empty())
    {
      selected.clear();
      cerr << "Select feature categories for the report.\n";
      for (const auto category : ordered_theory_categories_for_reports())
      {
        if (!allow_complex && category == theory_feature_category_t::complex)
        {
          continue;
        }
        const auto enabled = prompt_yes_no(
            "  Include " + cli_category_name(category) + " features",
            category != theory_feature_category_t::complex);
        if (!enabled.has_value())
        {
          return std::nullopt;
        }
        if (enabled.value())
        {
          selected.push_back(category);
        }
      }
      if (selected.empty())
      {
        cerr << "Select at least one feature category." << endl;
      }
    }
    return selected;
  }

  /** @brief Copies a selected report request back into the mutable CLI option bundle. */
  void apply_request_to_options(const report_request_t& request,
                                cli_options_t&          options)
  {
    options.mode_override = request.mode;
    options.category_override = request.requested_categories;
    options.include_raw_terms_override = request.include_raw_terms;
    options.include_simplified_terms_override = request.include_simplified_terms;
    options.include_isolation_override = request.include_isolation;
    if (!request.include_isolation)
    {
      options.min_diffs = false;
      options.feature_names = false;
    }
  }

  /** @brief Guides the user through selecting a feasible report request for the current model. */
  [[nodiscard]] std::optional<report_request_t>
      prompt_guided_report_request(const spl_processor_t& processor,
                                   const cli_options_t&   seeded_options)
  {
    auto base_request = report_request_t::from_category_hint(
        processor.get_specification().m_category_filter);
    base_request.minimal_differences_only = seeded_options.min_diffs;
    base_request.include_feature_names    = seeded_options.feature_names;

    const auto requested_feasibility = estimate_report_feasibility(
        processor.get_specification().m_F,
        processor.get_specification().system_space(),
        base_request);
    const auto plan = plan_report_execution(processor.get_specification().m_F,
                                            processor.get_specification().system_space(),
                                            base_request);

    auto summary_request = make_summary_request(base_request);
    summary_request.include_raw_terms = false;
    summary_request.include_simplified_terms = false;

    auto non_complex_request = make_category_request(
        base_request,
        { theory_feature_category_t::impossible,
          theory_feature_category_t::core,
          theory_feature_category_t::fundamental,
          theory_feature_category_t::not_feature,
          theory_feature_category_t::and_feature,
          theory_feature_category_t::or_feature },
        false);
    non_complex_request.include_raw_terms = false;
    non_complex_request.include_simplified_terms = true;
    non_complex_request.include_isolation = false;

    auto full_request = report_request_t::unrestricted();
    full_request.include_raw_terms = false;
    full_request.include_simplified_terms = true;
    full_request.include_isolation = false;
    full_request.minimal_differences_only = seeded_options.min_diffs;
    full_request.include_feature_names = seeded_options.feature_names;

    auto isolation_request = make_isolation_only_request(base_request);
    isolation_request.minimal_differences_only = seeded_options.min_diffs;
    isolation_request.include_feature_names = seeded_options.feature_names;

    cerr << "\nModel overview\n";
    print_feature_theory_overview(cerr, requested_feasibility);
    cerr << "Suggested default: " << request_display_name(plan.recommended_request)
         << " (risk=" << to_string(plan.recommended_feasibility.risk) << ")\n";
    if (!plan.rationale.empty())
    {
      cerr << "Reason: " << plan.rationale << "\n";
    }

    const std::vector<report_request_t> mode_requests {
      summary_request,
      non_complex_request,
      full_request,
      isolation_request,
    };
    std::vector<std::string> mode_choices {
      "Summary",
      "Full without complex features",
      "Full detailed",
      "Isolation only",
    };

    std::size_t default_choice = 1;
    if (plan.recommended_request.mode == report_mode_t::summary)
    {
      default_choice = 1;
    }
    else if (plan.recommended_request.mode == report_mode_t::isolation_only)
    {
      default_choice = 4;
    }
    else if (plan.recommended_request.mode == report_mode_t::full
             && !plan.recommended_request.include_complex_details)
    {
      default_choice = 2;
    }
    else if (plan.recommended_request.mode == report_mode_t::full)
    {
      default_choice = 3;
    }
    mode_choices.at(default_choice - 1) += " [recommended]";

    const auto mode_choice = prompt_numbered_choice(
        "Choose a report mode.",
        mode_choices,
        default_choice);
    if (!mode_choice.has_value())
    {
      return std::nullopt;
    }

    auto selected_request = mode_requests.at(mode_choice.value() - 1);

    const auto advanced = prompt_yes_no("Adjust advanced options", false);
    if (!advanced.has_value())
    {
      return std::nullopt;
    }

    if (advanced.value() && selected_request.mode == report_mode_t::full)
    {
      const bool complex_allowed = selected_request.include_complex_details;
      const std::vector<std::string> category_choices = complex_allowed
          ? std::vector<std::string> {
                "All feature categories",
                "All non-complex feature categories",
                "Fundamental and NOT features only",
                "Choose feature categories manually",
            }
          : std::vector<std::string> {
                "All non-complex feature categories",
                "Fundamental and NOT features only",
                "Choose feature categories manually",
            };

      const auto category_choice = prompt_numbered_choice(
          "Choose the feature categories for this report.",
          category_choices,
          1);
      if (!category_choice.has_value())
      {
        return std::nullopt;
      }

      const auto selected_index = category_choice.value() - 1;
      if (complex_allowed)
      {
        if (selected_index == 1)
        {
          // keep the default unrestricted category slice
        }
        else if (selected_index == 2)
        {
          selected_request.requested_categories = std::vector<theory_feature_category_t> {
            theory_feature_category_t::impossible,
            theory_feature_category_t::core,
            theory_feature_category_t::fundamental,
            theory_feature_category_t::not_feature,
            theory_feature_category_t::and_feature,
            theory_feature_category_t::or_feature };
        }
        else if (selected_index == 3)
        {
          selected_request.requested_categories = std::vector<theory_feature_category_t> {
            theory_feature_category_t::fundamental,
            theory_feature_category_t::not_feature };
        }
        else if (selected_index == 4)
        {
          const auto categories = prompt_manual_categories(true);
          if (!categories.has_value())
          {
            return std::nullopt;
          }
          selected_request.requested_categories = categories.value();
        }
      }
      else
      {
        if (selected_index == 1)
        {
          // keep the default non-complex category slice
        }
        else if (selected_index == 2)
        {
          selected_request.requested_categories = std::vector<theory_feature_category_t> {
            theory_feature_category_t::fundamental,
            theory_feature_category_t::not_feature };
        }
        else if (selected_index == 3)
        {
          const auto categories = prompt_manual_categories(false);
          if (!categories.has_value())
          {
            return std::nullopt;
          }
          selected_request.requested_categories = categories.value();
        }
      }

      selected_request.include_complex_details
          = selected_request.requested_categories.has_value()
            ? std::ranges::find(selected_request.requested_categories.value(),
                                theory_feature_category_t::complex)
                  != selected_request.requested_categories->end()
            : complex_allowed;

      const auto raw_terms = prompt_yes_no("Write raw theory terms to the report",
                                           selected_request.include_raw_terms);
      if (!raw_terms.has_value())
      {
        return std::nullopt;
      }
      selected_request.include_raw_terms = raw_terms.value();

      const auto simplified_terms = prompt_yes_no(
          "Write simplified theory terms to the report",
          selected_request.include_simplified_terms);
      if (!simplified_terms.has_value())
      {
        return std::nullopt;
      }
      selected_request.include_simplified_terms = simplified_terms.value();

      const auto isolation = prompt_yes_no("Write isolation results to the report",
                                           selected_request.include_isolation);
      if (!isolation.has_value())
      {
        return std::nullopt;
      }
      selected_request.include_isolation = isolation.value();
    }

    if (advanced.value() && ((selected_request.mode == report_mode_t::full
         && selected_request.include_isolation)
        || selected_request.mode == report_mode_t::isolation_only))
    {
      const auto min_diffs = prompt_yes_no(
          "Limit isolation results to minimal witness differences",
          seeded_options.min_diffs);
      if (!min_diffs.has_value())
      {
        return std::nullopt;
      }
      selected_request.minimal_differences_only = min_diffs.value();

      const auto feature_names = prompt_yes_no(
          "Include feature names in the isolation results",
          seeded_options.feature_names);
      if (!feature_names.has_value())
      {
        return std::nullopt;
      }
      selected_request.include_feature_names = feature_names.value();
    }

    const auto selected_feasibility = estimate_report_feasibility(
        processor.get_specification().m_F,
        processor.get_specification().system_space(),
        selected_request);
    cerr << '\n'
         << "Run summary\n"
         << "  Report: " << request_display_name(selected_request) << '\n'
         << "  Detail: " << request_detail_summary(selected_request) << '\n'
         << "  Estimate: " << selected_feasibility.summary() << "\n\n";

    return selected_request;
  }

  /** @brief Builds a full cli_options_t object by running the guided wizard. */
  [[nodiscard]] std::optional<cli_options_t>
      build_guided_options(int argc, char* argv[])
  {
    if (!stdin_is_terminal())
    {
      return std::nullopt;
    }

    const auto seed = infer_guided_seed(argc, argv);
    cerr << "Guided mode\n"
         << "Configure a derived-feature-space planning or report run interactively.\n";

    cli_options_t options {};
    options.min_diffs = seed.min_diffs.value_or(false);
    options.feature_names = seed.feature_names.value_or(false);

    const auto command = prompt_guided_command(seed.command);
    if (!command.has_value())
    {
      return std::nullopt;
    }
    options.command = command.value();
    options.requires_result_path = options.command == cli_command_t::analyze;

    const auto specification_path = prompt_text_value("Enter SPL specification path",
                                                      seed.specification_path);

    if (!specification_path.has_value())
    {
      return std::nullopt;
    }
    options.specification_path = specification_path.value();

    if (options.command == cli_command_t::analyze)
    {
      const auto result_path = prompt_text_value(
          "Enter report output path",
          seed.result_path.empty() ? std::string { "report.txt" } : seed.result_path);
      if (!result_path.has_value())
      {
        return std::nullopt;
      }
      options.result_path = result_path.value();
    }

    try
    {
      spl_processor_t processor { options.specification_path };
      if (options.command == cli_command_t::expression_from_systems)
      {
        return options;
      }
      const auto selected_request = prompt_guided_report_request(processor, options);
      if (!selected_request.has_value())
      {
        return std::nullopt;
      }
      apply_request_to_options(selected_request.value(), options);
      options.min_diffs = selected_request->minimal_differences_only;
      options.feature_names = selected_request->include_feature_names;
    }
    catch (...)
    {
      cerr << "Cannot parse the SPL specification in guided mode.\n"
           << "Program terminated." << endl;
      return std::nullopt;
    }

    return options;
  }

  /** @brief Parses the command line and validates supported invocation forms. */
  [[nodiscard]] std::optional<cli_options_t>
      parse_cli_options(int argc, char* argv[])
  {
    if (argc < 2)
    {
      return std::nullopt;
    }

    cli_options_t options {};
    int           cursor = 1;

    const string first_argument { argv[cursor] };
    if (is_guided_token(first_argument))
    {
      return std::nullopt;
    }
    if (first_argument == "analyze")
    {
      options.command = cli_command_t::analyze;
      ++cursor;
    }
    else if (first_argument == "plan")
    {
      options.command              = cli_command_t::plan;
      options.requires_result_path = false;
      ++cursor;
    }
    else if (first_argument == "expr" || first_argument == "expression")
    {
      options.command              = cli_command_t::expression_from_systems;
      options.specification_path   = argv[++cursor];
      //options.mode_override           = report_mode_t::isolation_only;
      ++cursor;
      for (; cursor < argc; ++cursor)
      {
        options.found_systems_in_alignment.push_back(argv[cursor]);
      }
      return options;
    }
    else if (first_argument == "traces")
    {
      options.command = cli_command_t::trace_export;
      ++cursor;
      if (argc - cursor < 2)
      {
        cerr << "Usage: " << program_display_name(argv[0])
             << " traces <spl-file> <output-file> [--max-grade=N]\n"
             << "Program terminated." << endl;
        return std::nullopt;
      }
      options.specification_path = argv[cursor++];
      options.result_path        = argv[cursor++];
      for (; cursor < argc; ++cursor)
      {
        const string option { argv[cursor] };
        if (option.rfind("--max-grade=", 0) == 0)
        {
          if (!parse_positive_unsigned_integer(option, "--max-grade", options.max_grade, cerr))
          {
            return std::nullopt;
          }
        }
        else
        {
          cerr << "Unknown option \"" << option << "\".\n"
               << "Program terminated." << endl;
          return std::nullopt;
        }
      }
      return options;
    }
    else
    {
      options.command           = cli_command_t::analyze;
      options.legacy_invocation = true;
    }

    const int remaining_arguments = argc - cursor;
    if (options.command == cli_command_t::analyze)
    {
      if (remaining_arguments < 4)
      {
        return std::nullopt;
      }
      options.specification_path = argv[cursor++];
      options.result_path        = argv[cursor++];
      const string diff_option   = argv[cursor++];
      const string names_option  = argv[cursor++];
      if (!parse_bool_toggle(diff_option, "-min-diffs", options.min_diffs, cerr)
          || !parse_bool_toggle(names_option,
                                "-feature-names",
                                options.feature_names,
                                cerr))
      {
        return std::nullopt;
      }
    }
    else
    {
      if (remaining_arguments < 1)
      {
        return std::nullopt;
      }
      options.specification_path = argv[cursor++];
    }

    for (; cursor < argc; ++cursor)
    {
      const string option { argv[cursor] };
      if (option.rfind("-min-diffs=", 0) == 0)
      {
        if (!parse_bool_toggle(option, "-min-diffs", options.min_diffs, cerr))
        {
          return std::nullopt;
        }
      }
      else if (option.rfind("-feature-names=", 0) == 0)
      {
        if (!parse_bool_toggle(option,
                               "-feature-names",
                               options.feature_names,
                               cerr))
        {
          return std::nullopt;
        }
      }
      else if (option == "--yes")
      {
        options.assume_yes = true;
      }
      else if (option == "--accept-recommended")
      {
        options.accept_recommended = true;
      }
      else if (option == "--non-interactive")
      {
        options.non_interactive = true;
      }
      else if (option == "--force")
      {
        options.force = true;
      }
      else if (option.rfind("--mode=", 0) == 0)
      {
        if (!parse_report_mode_override(option, options.mode_override, cerr))
        {
          return std::nullopt;
        }
      }
      else if (option.rfind("--categories=", 0) == 0)
      {
        if (!parse_category_override(option, options.category_override, cerr))
        {
          return std::nullopt;
        }
      }
      else if (option.rfind("--max-runtime-ms=", 0) == 0)
      {
        if (!parse_nonnegative_long_double(option,
                                           "--max-runtime-ms",
                                           options.max_runtime_ms,
                                           cerr))
        {
          return std::nullopt;
        }
      }
      else if (option.rfind("--max-output-bytes=", 0) == 0)
      {
        if (!parse_nonnegative_long_double(option,
                                           "--max-output-bytes",
                                           options.max_output_bytes,
                                           cerr))
        {
          return std::nullopt;
        }
      }
      else if (option.rfind("--raw-terms=", 0) == 0)
      {
        bool value { false };
        if (!parse_bool_toggle(option, "--raw-terms", value, cerr))
        {
          return std::nullopt;
        }
        options.include_raw_terms_override = value;
      }
      else if (option.rfind("--simplified-terms=", 0) == 0)
      {
        bool value { false };
        if (!parse_bool_toggle(option, "--simplified-terms", value, cerr))
        {
          return std::nullopt;
        }
        options.include_simplified_terms_override = value;
      }
      else if (option.rfind("--isolation=", 0) == 0)
      {
        bool value { false };
        if (!parse_bool_toggle(option, "--isolation", value, cerr))
        {
          return std::nullopt;
        }
        options.include_isolation_override = value;
      }
      else
      {
        cerr << "Unknown option \"" << option << "\".\n"
             << "Program terminated." << endl;
        return std::nullopt;
      }
    }

    if (options.assume_yes && options.accept_recommended)
    {
      cerr << "Conflicting options: --yes and --accept-recommended cannot be"
              " used together.\n"
           << "Program terminated." << endl;
      return std::nullopt;
    }

    if (options.command == cli_command_t::plan
        && (options.assume_yes || options.accept_recommended
            || options.non_interactive || options.force))
    {
      cerr << "Approval and forcing options are only valid for the analyze command.\n"
           << "Program terminated." << endl;
      return std::nullopt;
    }

    return options;
  }

  /** @brief Describes one lighter follow-up report that can be run after an initial overview. */
  struct drill_down_suggestion_t final
  {
      std::string          label {};
      std::string          rationale {};
      report_request_t     request {};
      report_feasibility_t feasibility {};
      bool                 executable_now { true };
  };

  /** @brief Returns whether two report requests encode the same effective output request. */
  [[nodiscard]] bool same_report_request(const report_request_t& lhs,
                                         const report_request_t& rhs)
  {
    return lhs.mode == rhs.mode
           && lhs.requested_categories == rhs.requested_categories
           && lhs.include_complex_details == rhs.include_complex_details
           && lhs.include_raw_terms == rhs.include_raw_terms
           && lhs.include_simplified_terms == rhs.include_simplified_terms
           && lhs.include_isolation == rhs.include_isolation
           && lhs.minimal_differences_only == rhs.minimal_differences_only
           && lhs.include_feature_names == rhs.include_feature_names;
  }

  /** @brief Produces a summary-only request from an existing request. */
  [[nodiscard]] report_request_t make_summary_request(const report_request_t& base)
  {
    auto request { base };
    request.mode                     = report_mode_t::summary;
    request.include_complex_details  = false;
    request.include_raw_terms        = false;
    request.include_simplified_terms = false;
    request.include_isolation        = false;
    return request;
  }

  [[nodiscard]] report_request_t make_category_request(
      const report_request_t& base,
      std::vector<theory_feature_category_t> categories,
      bool include_isolation = false)
  {
    auto request { base };
    request.mode                = report_mode_t::full;
    request.requested_categories = std::move(categories);
    request.include_complex_details
        = std::ranges::find(request.requested_categories.value(),
                            theory_feature_category_t::complex)
          != request.requested_categories->end();
    request.include_isolation = include_isolation;
    return request;
  }

  [[nodiscard]] report_request_t make_isolation_only_request(
      const report_request_t& base)
  {
    auto request { base };
    request.mode                     = report_mode_t::isolation_only;
    request.include_isolation        = true;
    request.include_raw_terms        = false;
    request.include_simplified_terms = false;
    request.include_complex_details  = false;
    return request;
  }

  /** @brief Merges SPL hints and CLI overrides into the request used for the current run. */
  [[nodiscard]] report_request_t requested_report_request(
      const spl_processor_t& processor,
      const cli_options_t&   options)
  {
    report_request_t request = report_request_t::from_category_hint(
        processor.get_specification().m_category_filter);
    request.minimal_differences_only = options.min_diffs;
    request.include_feature_names    = options.feature_names;

    if (options.category_override.has_value())
    {
      request.requested_categories = options.category_override;
      request.include_complex_details
          = std::ranges::find(request.requested_categories.value(),
                              theory_feature_category_t::complex)
            != request.requested_categories->end();
    }

    if (options.mode_override.has_value())
    {
      switch (options.mode_override.value())
      {
        case report_mode_t::full:
          request.mode = report_mode_t::full;
          break;
        case report_mode_t::summary:
          request = make_summary_request(request);
          break;
        case report_mode_t::isolation_only:
          request = make_isolation_only_request(request);
          break;
        case report_mode_t::selected_features:
          request.mode = report_mode_t::selected_features;
          break;
      }
    }

    if (options.include_raw_terms_override.has_value())
    {
      request.include_raw_terms = options.include_raw_terms_override.value();
    }
    if (options.include_simplified_terms_override.has_value())
    {
      request.include_simplified_terms
          = options.include_simplified_terms_override.value();
    }
    if (options.include_isolation_override.has_value())
    {
      request.include_isolation = options.include_isolation_override.value();
    }

    return request;
  }

  /** @brief Joins CLI category names for help text and planner explanations. */
  [[nodiscard]] std::string join_category_names(
      const std::vector<theory_feature_category_t>& categories)
  {
    std::ostringstream os {};
    for (std::size_t index = 0; index < categories.size(); ++index)
    {
      if (index != 0)
      {
        os << ',';
      }
      os << cli_category_name(categories.at(index));
    }
    return os.str();
  }

  /** @brief Renders a request as explicit CLI override flags. */
  [[nodiscard]] std::string request_override_flags(const report_request_t& request)
  {
    std::ostringstream os {};
    os << " --mode=" << to_string(request.mode);
    if (request.requested_categories.has_value())
    {
      os << " --categories=" << join_category_names(request.requested_categories.value());
    }
    return os.str();
  }

  /** @brief Returns a human-readable short label for one report request. */
  [[nodiscard]] std::string request_display_name(const report_request_t& request)
  {
    switch (request.mode)
    {
      case report_mode_t::summary:
        return "summary report";
      case report_mode_t::isolation_only:
        return "isolation-only report";
      case report_mode_t::selected_features:
        return "selected-feature report";
      case report_mode_t::full:
        break;
    }

    if (request.requested_categories.has_value())
    {
      const auto& categories = request.requested_categories.value();
      if (!categories.empty())
      {
        return "category-focused full report (" + join_category_names(categories)
               + ")";
      }
    }

    if (!request.include_complex_details)
    {
      return "full report without complex-feature details";
    }
    return "full detailed report";
  }

  /** @brief Returns a compact description of the detail switches enabled by one request. */
  [[nodiscard]] std::string request_detail_summary(const report_request_t& request)
  {
    std::vector<std::string> parts {};
    parts.push_back(request.include_complex_details ? "complex-feature details written"
                                                   : "complex-feature details skipped");
    parts.push_back(request.include_raw_terms ? "raw theory terms written"
                                              : "raw theory terms skipped");
    parts.push_back(request.include_simplified_terms ? "simplified theory terms written"
                                                     : "simplified theory terms skipped");
    parts.push_back(request.include_isolation ? "isolation results written"
                                              : "isolation results skipped");
    if (request.requested_categories.has_value())
    {
      parts.push_back("categories=" + join_category_names(request.requested_categories.value()));
    }
    else
    {
      parts.push_back("all feature categories");
    }

    std::ostringstream os {};
    for (std::size_t index = 0; index < parts.size(); ++index)
    {
      if (index != 0)
      {
        os << "; ";
      }
      os << parts.at(index);
    }
    return os.str();
  }

  /** @brief Suggests an output path for drill-down reports derived from the current run. */
  [[nodiscard]] std::string suggested_result_path(const cli_options_t& options,
                                                  const std::string&   suffix)
  {
    if (options.result_path.empty())
    {
      return "<result-file-" + suffix + ">";
    }
    return options.result_path + "." + suffix + ".txt";
  }

  /** @brief Builds lighter follow-up report suggestions from the current request and model size. */
  [[nodiscard]] std::vector<drill_down_suggestion_t> build_drill_down_suggestions(
      unsigned_integer      base_feature_count,
      const system_space_t& system_space,
      const report_request_t& base_request)
  {
    std::vector<drill_down_suggestion_t> suggestions {};

    const auto summary_request = make_summary_request(base_request);
    if (!same_report_request(summary_request, base_request))
    {
      suggestions.push_back({
          "summary-first entry point",
          "Generate the compact theory summary first, then choose a narrower follow-up report.",
          summary_request,
          estimate_report_feasibility(base_feature_count, system_space, summary_request),
          true });
    }

    const auto non_complex_request = make_category_request(
        base_request,
        { theory_feature_category_t::impossible,
          theory_feature_category_t::core,
          theory_feature_category_t::fundamental,
          theory_feature_category_t::not_feature,
          theory_feature_category_t::and_feature,
          theory_feature_category_t::or_feature },
        false);
    if (!same_report_request(non_complex_request, base_request))
    {
      suggestions.push_back({
          "all non-complex features",
          "Keep exact feature IDs, but focus the next report on the non-complex categories only.",
          non_complex_request,
          estimate_report_feasibility(base_feature_count, system_space, non_complex_request),
          true });
    }

    const auto fundamental_not_request = make_category_request(
        base_request,
        { theory_feature_category_t::fundamental,
          theory_feature_category_t::not_feature },
        false);
    if (!same_report_request(fundamental_not_request, base_request))
    {
      suggestions.push_back({
          "fundamental and not features only",
          "Use the summary counts to drill down into the first practically useful simple-feature slice.",
          fundamental_not_request,
          estimate_report_feasibility(base_feature_count, system_space, fundamental_not_request),
          true });
    }

    const auto isolation_request = make_isolation_only_request(base_request);
    if (!same_report_request(isolation_request, base_request))
    {
      suggestions.push_back({
          "isolation results only",
          "When witness generation matters more than the full registry listing, isolate directly.",
          isolation_request,
          estimate_report_feasibility(base_feature_count, system_space, isolation_request),
          true });
    }

    return suggestions;
  }

  /** @brief Returns whether the estimated report cost exceeds the configured guardrails. */
  [[nodiscard]] bool exceeds_guardrails(const report_feasibility_t&  feasibility,
                                        const execution_guardrails_t& guardrails);

  /** @brief Prints suggested follow-up commands after planning or a light initial run. */
  void print_drill_down_workflow(std::ostream&                 os,
                                 const char*                   program_name,
                                 const cli_options_t&          options,
                                 unsigned_integer              base_feature_count,
                                 const system_space_t&         system_space,
                                 const report_request_t&       base_request,
                                 const execution_guardrails_t& guardrails,
                                 bool                          include_commands)
  {
    const auto suggestions = build_drill_down_suggestions(
        base_feature_count, system_space, base_request);
    if (suggestions.empty())
    {
      return;
    }

    os << "\nSuggested drill-down workflow\n";
    for (const auto& suggestion : suggestions)
    {
      os << "  - " << suggestion.label << " [risk="
         << to_string(suggestion.feasibility.risk) << "]";
      if (exceeds_guardrails(suggestion.feasibility, guardrails))
      {
        os << " [exceeds current guardrails]";
      }
      os << ": " << suggestion.rationale << "\n"
         << "    Estimate: " << suggestion.feasibility.summary() << "\n";
      if (include_commands)
      {
        os << "    Command: " << program_name << " analyze "
           << options.specification_path << " "
           << suggested_result_path(options,
                                    suggestion.label == "summary-first entry point"
                                        ? "summary"
                                        : (suggestion.label == "all non-complex features"
                                               ? "non_complex"
                                               : (suggestion.label == "fundamental and not features only"
                                                      ? "fundamental_not"
                                                      : "isolation")))
           << " -min-diffs=" << (options.min_diffs ? "true" : "false")
           << " -feature-names=" << (options.feature_names ? "true" : "false")
           << request_override_flags(suggestion.request) << "\n";
      }
    }
    os << "  - selected feature IDs only [not executable now]: use the summary or"
          " category-specific report above to decide which IDs should become the"
          " next explicit target set.\n";
  }

  /** @brief Resolves the effective runtime and output-size limits for one invocation. */
  [[nodiscard]] execution_guardrails_t
      guardrails_for(const cli_options_t& options)
  {
    execution_guardrails_t guardrails {};
    if (options.max_runtime_ms.has_value())
    {
      guardrails.max_runtime_ms = options.max_runtime_ms.value();
    }
    if (options.max_output_bytes.has_value())
    {
      guardrails.max_output_bytes = options.max_output_bytes.value();
    }
    return guardrails;
  }

  [[nodiscard]] bool exceeds_guardrails(const report_feasibility_t&  feasibility,
                                        const execution_guardrails_t& guardrails)
  {
    return feasibility.estimated_total_runtime_ms > guardrails.max_runtime_ms
           || feasibility.estimated_output_bytes > guardrails.max_output_bytes;
  }

  /** @brief Formats how close one feasibility estimate is to the active hard guardrails. */
  [[nodiscard]] std::string
      guardrail_summary(const report_feasibility_t&  feasibility,
                        const execution_guardrails_t& guardrails)
  {
    std::ostringstream os {};
    os << "runtime≈" << human_readable_duration(feasibility.estimated_total_runtime_ms)
       << " (limit " << human_readable_duration(guardrails.max_runtime_ms) << "), output≈"
       << human_readable_size(feasibility.estimated_output_bytes)
       << " (limit " << human_readable_size(guardrails.max_output_bytes) << ")";
    return os.str();
  }

  /** @brief Prints the planner or feasibility observations one per line. */
  void print_observations(std::ostream&                       os,
                          const std::vector<std::string>& observations,
                          const std::string_view            heading)
  {
    if (observations.empty())
    {
      return;
    }

    os << heading << "\n";
    for (const auto& observation : observations)
    {
      os << "  - " << observation << "\n";
    }
  }

  /** @brief Prints the full planning summary for the current request. */
  void print_plan_summary(std::ostream&                  os,
                          const report_execution_plan_t& plan,
                          const execution_guardrails_t&  guardrails)
  {
    os << "Requested plan\n"
       << "  Risk: " << to_string(plan.requested_feasibility.risk) << "\n"
       << "  Estimate: " << plan.requested_feasibility.summary() << "\n"
       << "  Guardrails: "
       << (exceeds_guardrails(plan.requested_feasibility, guardrails)
               ? "exceeds"
               : "fits")
       << " (" << guardrail_summary(plan.requested_feasibility, guardrails)
       << ")\n";
    print_observations(os,
                       plan.requested_feasibility.observations,
                       "  Derived-feature-space signals");

    os << "\nRecommended plan\n"
       << "  Risk: " << to_string(plan.recommended_feasibility.risk) << "\n"
       << "  Estimate: " << plan.recommended_feasibility.summary() << "\n"
       << "  Guardrails: "
       << (exceeds_guardrails(plan.recommended_feasibility, guardrails)
               ? "exceeds"
               : "fits")
       << " (" << guardrail_summary(plan.recommended_feasibility, guardrails)
       << ")\n"
       << "  Summary: " << plan.summary() << "\n";
    print_observations(os,
                       plan.recommended_feasibility.observations,
                       "  Why this plan is attractive");

    if (!plan.alternatives.empty())
    {
      os << "\nAlternatives\n";
      for (const auto& option : plan.alternatives)
      {
        os << "  - " << option.label;
        if (option.feasibility.has_value())
        {
          os << " [risk=" << to_string(option.feasibility->risk) << "]";
        }
        if (!option.executable_now)
        {
          os << " [not executable now]";
        }
        os << ": " << option.rationale << "\n";
      }
    }
  }

  /** @brief Prints one row summarizing model size and derived-feature-space size. */
  void print_model_summary(std::ostream& os,
                           const feature_analysis_t& analysis)
  {
    os << "MODEL" << separator() << "|F|" << separator() << "|S_valid|"
       << separator() << "|S_available|" << separator() << "|S_missing|"
       << separator() << "|T|" << std::endl;
    os << "THEORY" << separator() << analysis.base_feature_count() << separator()
       << analysis.system_space().valid.size() << separator()
       << analysis.system_space().available.size() << separator()
       << analysis.system_space().missing.size() << separator()
       << analysis.feature_count() << std::endl;
  }

  /** @brief Prints the full per-feature report rows for a non-summary report. */
  void print_feature_rows(std::ostream&                os,
                          const feature_analysis_t&    analysis,
                          const report_request_t&      request)
  {
    os << "CATEGORY" << separator() << "i" << separator() << "FEATURE"
       << separator() << "t_i" << separator() << "β(t_i)";
    if (request.include_simplified_terms)
    {
      os << separator() << "ℓ_T(t_i)(simplified)";
    }
    if (request.include_raw_terms)
    {
      os << separator() << "ℓ_T(t_i)(raw term)";
    }
    os << std::endl;

    for (const auto category : ordered_theory_categories_for_reports())
    {
      if (!analysis.includes_category(category))
      {
        continue;
      }
      for (const auto& record : analysis.feature_registry())
      {
        if (record.category != category)
        {
          continue;
        }
        os << to_string(record.category) << separator()
           << paper_feature_index(record.id) << separator()
           << paper_feature_label(record.id) << separator()
           << system_membership_set(record.systems_present, analysis.system_space())
           << separator() << to_string(record.systems_present);
        if (request.include_simplified_terms)
        {
          os << separator() << record.simplified_term;
        }
        if (request.include_raw_terms)
        {
          os << separator() << record.raw_term;
        }
        os << std::endl;
      }
    }
  }

  /** @brief Prints a compact feature index table used by the full report. */
  void print_feature_index_rows(std::ostream&                os,
                                const feature_analysis_t&    analysis,
                                const report_request_t&      request)
  {
    os << "i" << separator() << "FEATURE" << separator() << "CATEGORY"
       << separator() << "t_i" << separator() << "β(t_i)";
    if (request.include_simplified_terms)
    {
      os << separator() << "ℓ_T(t_i)(simplified)";
    }
    if (request.include_raw_terms)
    {
      os << separator() << "ℓ_T(t_i)(raw term)";
    }
    os << std::endl;

    for (const auto& record : analysis.feature_registry())
    {
      if (!analysis.includes_category(record.category))
      {
        continue;
      }
      os << paper_feature_index(record.id) << separator()
         << paper_feature_label(record.id) << separator()
         << feature_isolation::to_string(record.category) << separator()
         << system_membership_set(record.systems_present, analysis.system_space())
         << separator() << to_string(record.systems_present);
      if (request.include_simplified_terms)
      {
        os << separator() << record.simplified_term;
      }
      if (request.include_raw_terms)
      {
        os << separator() << record.raw_term;
      }
      os << std::endl;
    }
  }

  /** @brief Writes the summary report variant. */
  void write_summary_report(std::ostream&               os,
                            printer_t&                  printer,
                            const feature_isolation_t& fi)
  {
    os << "=== SPL SPECIFICATION ===" << endl;
    printer.print_specification(fi.m_spl);
    os << "\n=== THEORY SUMMARY ===" << endl;
    print_model_summary(os, fi.m_features);
    os << std::endl;
    fi.m_features.print_categories(os);
  }

  /** @brief Writes the full detailed report variant. */
  void write_full_report(std::ostream&               os,
                         printer_t&                  printer,
                         const feature_isolation_t& fi)
  {
    os << "=== SPL SPECIFICATION ===" << endl;
    printer.print_specification(fi.m_spl);
    os << "\n=== SYSTEMS ===" << endl;
    fi.m_features.print(os);
    os << "=== FEATURES BY CATEGORY ===" << endl;
    print_feature_rows(os, fi.m_features, fi.m_report_request);
    os << "=== FEATURES BY INDEX ===" << endl;
    print_feature_index_rows(os, fi.m_features, fi.m_report_request);
    if (fi.m_report_request.include_isolation)
    {
      os << "=== ISOLATION RESULTS ===" << endl;
      fi.print(os,
               fi.m_report_request.minimal_differences_only,
               fi.m_report_request.include_feature_names);
    }
  }

  /** @brief Writes the isolation-only report variant. */
  void write_isolation_only_report(std::ostream&               os,
                                   printer_t&                  printer,
                                   const feature_isolation_t& fi)
  {
    os << "=== SPL SPECIFICATION ===" << endl;
    printer.print_specification(fi.m_spl);
    os << "\n=== ISOLATION RESULTS ===" << endl;
    if (fi.m_report_request.include_isolation)
    {
      fi.print(os,
               fi.m_report_request.minimal_differences_only,
               fi.m_report_request.include_feature_names);
    }
    else
    {
      os << "Isolation output disabled by the approved report request." << endl;
    }
  }

  /** @brief Dispatches to the requested report writer. */
  [[nodiscard]] int write_report(std::ostream&              os,
                                 const feature_isolation_t& fi)
  {
    printer_t printer { os };
    switch (fi.m_report_request.mode)
    {
      case report_mode_t::full:
        write_full_report(os, printer, fi);
        return 0;
      case report_mode_t::summary:
        write_summary_report(os, printer, fi);
        return 0;
      case report_mode_t::isolation_only:
        write_isolation_only_report(os, printer, fi);
        return 0;
      case report_mode_t::selected_features:
        cerr << "The selected-features report mode is planned, but not yet executable in this CLI step.\n"
             << "Program terminated." << endl;
        return 7;
    }
    cerr << "Unknown report mode.\nProgram terminated." << endl;
    return 7;
  }

  /** @brief Represents one interactive choice presented when approving a planned execution. */
  struct interactive_plan_choice_t final
  {
      std::string          label {};
      std::string          rationale {};
      report_request_t     request {};
      report_feasibility_t feasibility {};
      bool                 recommended { false };
      bool                 requested { false };
  };

  /** @brief Builds the choice list shown in the interactive approval prompt. */
  [[nodiscard]] std::vector<interactive_plan_choice_t>
      build_interactive_plan_choices(const report_execution_plan_t& plan)
  {
    std::vector<interactive_plan_choice_t> choices {};
    auto add_choice = [&](const std::string&          label,
                          const std::string&          rationale,
                          const report_request_t&     request,
                          const report_feasibility_t& feasibility,
                          const bool                  recommended,
                          const bool                  requested) {
      const auto duplicate = std::ranges::find_if(choices, [&](const auto& existing) {
        return same_report_request(existing.request, request);
      });
      if (duplicate != choices.end())
      {
        if (recommended)
        {
          duplicate->recommended = true;
        }
        if (requested)
        {
          duplicate->requested = true;
        }
        return;
      }
      choices.push_back({ label, rationale, request, feasibility, recommended, requested });
    };

    add_choice("Keep the requested report",
               "Run exactly the report requested on the command line.",
               plan.requested_request,
               plan.requested_feasibility,
               false,
               true);
    add_choice(plan.adjustment_recommended ? "Use the recommended first step"
                                           : "Use the planner-confirmed report",
               plan.rationale.empty()
                   ? std::string { "The planner sees no better first step for this derived feature space." }
                   : plan.rationale,
               plan.recommended_request,
               plan.recommended_feasibility,
               true,
               false);

    for (const auto& option : plan.alternatives)
    {
      if (!option.executable_now || !option.request.has_value()
          || !option.feasibility.has_value())
      {
        continue;
      }
      add_choice(option.label,
                 option.rationale,
                 option.request.value(),
                 option.feasibility.value(),
                 same_report_request(option.request.value(), plan.recommended_request),
                 same_report_request(option.request.value(), plan.requested_request));
    }
    return choices;
  }

  /** @brief Prints a compact overview of the requested derived-feature-space size and risk. */
  void print_feature_theory_overview(std::ostream&                os,
                                     const report_feasibility_t& feasibility)
  {
    os << "Derived-feature-space overview\n"
       << "  Fundamental features: " << feasibility.base_feature_count << '\n'
       << "  Valid systems: " << feasibility.valid_system_count << '\n'
       << "  Derived features induced by the valid systems: " << feasibility.theory_feature_count << '\n'
       << "  Complex-feature lower bound: "
       << feasibility.complex_feature_lower_bound << '\n'
       << "  Derived features covered by the current report (upper bound): "
       << feasibility.included_feature_upper_bound << '\n';
  }

  /** @brief Captures the final execution choice after approval. */
  struct execution_approval_t
  {
    report_request_t request {};
    bool             guardrails_approved { false };
  };

  /** @brief Asks the user to keep the requested plan, switch to a lighter plan, or cancel. */
  [[nodiscard]] std::optional<execution_approval_t>
      approve_execution_plan(const report_execution_plan_t& plan,
                             const cli_options_t&           options,
                             const execution_guardrails_t&  guardrails)
  {
    if (!plan.requested_feasibility.approval_recommended)
    {
      return execution_approval_t { plan.requested_request, false };
    }

    if (options.accept_recommended)
    {
      cerr << "Applying planner recommendation without prompting." << endl;
      return execution_approval_t { plan.recommended_request, false };
    }
    if (options.assume_yes)
    {
      cerr << "Proceeding with the requested plan due to --yes." << endl;
      return execution_approval_t { plan.requested_request, false };
    }

    if (options.non_interactive || !stdin_is_terminal())
    {
      cerr << "Approval required for this request. Re-run interactively, or pass"
              " --yes to keep the requested plan, or --accept-recommended to"
              " adopt the planner's lighter plan.\n"
           << "Program terminated." << endl;
      return std::nullopt;
    }

    const auto choices = build_interactive_plan_choices(plan);
    std::size_t default_choice = 1;
    for (std::size_t index = 0; index < choices.size(); ++index)
    {
      if (choices.at(index).recommended)
      {
        default_choice = index + 1;
        break;
      }
    }

    cerr << '\n' << "Execution choice\n";
    print_feature_theory_overview(cerr, plan.requested_feasibility);
    cerr << "  Requested: " << request_display_name(plan.requested_request)
         << " (" << to_string(plan.requested_feasibility.risk) << ", "
         << plan.requested_feasibility.estimated_total_runtime_ms << " ms, "
         << plan.requested_feasibility.estimated_output_bytes << " bytes)\n"
         << "  Suggested: " << request_display_name(plan.recommended_request)
         << " (" << to_string(plan.recommended_feasibility.risk) << ", "
         << plan.recommended_feasibility.estimated_total_runtime_ms << " ms, "
         << plan.recommended_feasibility.estimated_output_bytes << " bytes)\n";
    if (!plan.rationale.empty())
    {
      cerr << "  Note: " << plan.rationale << '\n';
    }

    cerr << '\n' << "Choose how to continue\n";
    for (std::size_t index = 0; index < choices.size(); ++index)
    {
      const auto& choice = choices.at(index);
      cerr << "  " << (index + 1) << ") " << choice.label;
      if (choice.recommended)
      {
        cerr << " [recommended]";
      }
      if (choice.requested)
      {
        cerr << " [requested]";
      }
      cerr << " — " << choice.feasibility.summary();
      if (exceeds_guardrails(choice.feasibility, guardrails))
      {
        cerr << " [requires force]";
      }
      cerr << '\n';
    }
    cerr << "  0) Cancel\n";

    while (true)
    {
      cerr << "Select a plan [default " << default_choice << "]: " << flush;
      std::string line {};
      if (!std::getline(cin, line))
      {
        cerr << "\nInput closed before approval was given.\nProgram terminated."
             << endl;
        return std::nullopt;
      }

      const auto trimmed = trim_copy(line);
      if (trimmed.empty())
      {
        const auto& choice = choices.at(default_choice - 1);
        return execution_approval_t {
          choice.request,
          choice.requested && exceeds_guardrails(choice.feasibility, guardrails)
        };
      }
      if (trimmed == "0" || trimmed == "c" || trimmed == "cancel")
      {
        cerr << "Report generation cancelled by user." << endl;
        return std::nullopt;
      }

      try
      {
        const auto choice_index = static_cast<std::size_t>(std::stoul(trimmed));
        if (choice_index >= 1 && choice_index <= choices.size())
        {
          const auto& choice = choices.at(choice_index - 1);
          return execution_approval_t {
            choice.request,
            choice.requested && exceeds_guardrails(choice.feasibility, guardrails)
          };
        }
      }
      catch (const std::exception&)
      {
      }
      cerr << "Unknown choice. Enter one of the shown numbers, or 0 to cancel." << endl;
    }
  }

  /** @brief Enforces hard guardrails before actually generating a report. */
  [[nodiscard]] bool enforce_guardrails(const report_feasibility_t&  feasibility,
                                        const execution_guardrails_t& guardrails,
                                        const cli_options_t&          options,
                                        const bool                    already_approved)
  {
    if (!exceeds_guardrails(feasibility, guardrails))
    {
      return true;
    }

    if (already_approved)
    {
      cerr << "Running the requested report beyond hard guardrails by explicit approval.\n"
           << "  " << guardrail_summary(feasibility, guardrails) << endl;
      return true;
    }

    cerr << "Execution guardrail warning: the approved plan exceeds the configured hard limits.\n"
         << "  " << guardrail_summary(feasibility, guardrails) << "\n";

    if (options.force)
    {
      cerr << "Proceeding anyway due to --force." << endl;
      return true;
    }

    if (options.non_interactive || !stdin_is_terminal())
    {
      cerr << "Use --force to bypass the hard guardrails, or adjust"
              " --max-runtime-ms/--max-output-bytes.\n"
           << "Program terminated." << endl;
      return false;
    }

    cerr << "Choose how to continue\n"
         << "  1) Force execution anyway\n"
         << "  0) Cancel\n";

    while (true)
    {
      cerr << "Select action [0]: " << flush;
      std::string line {};
      if (!std::getline(cin, line))
      {
        cerr << "\nInput closed before guardrail approval was given.\n"
             << "Program terminated." << endl;
        return false;
      }

      const auto trimmed = trim_copy(line);
      if (trimmed.empty() || trimmed == "0" || trimmed == "cancel")
      {
        cerr << "Report generation cancelled by user." << endl;
        return false;
      }
      if (trimmed == "1" || trimmed == "force")
      {
        cerr << "Proceeding beyond hard guardrails by explicit user approval."
             << endl;
        return true;
      }
      cerr << "Unknown choice. Enter 1 to force execution, or 0 to cancel." << endl;
    }
  }

/** @brief Prints notes when legacy invocation or CLI overrides affect the current run. */
  void print_compatibility_notes(std::ostream&              os,
                                 const std::string&        program_name,
                                 const cli_options_t&      options,
                                 const spl_specification_t& specification)
  {
    if (options.legacy_invocation)
    {
      os << "Compatibility note: the legacy positional analyze invocation is still accepted,\n"
         << "but future runs should prefer '" << program_name << " analyze ...'.\n";
    }

    const bool cli_overrides_report_hint
        = options.mode_override.has_value() || options.category_override.has_value()
          || options.include_raw_terms_override.has_value()
          || options.include_simplified_terms_override.has_value()
          || options.include_isolation_override.has_value();
    if (cli_overrides_report_hint
        && specification.m_category_filter.has_value()
        && !specification.m_category_filter->empty())
    {
      os << "Compatibility note: this run overrides the SPL CATEGORY_FILTER hint with CLI-selected report settings.\n";
    }
  }

  /** @brief Executes the analyze command end-to-end, including planning and report writing. */
  [[nodiscard]] int run_analyze(const std::string& program_name,
                               const cli_options_t& options)
  {
    ifstream spl_file { options.specification_path };
    if (spl_file)
    {
      spl_file.close();
    }
    else
    {
      cerr << "Cannot open spl-specification-file named \""
           << options.specification_path << "\".\nProgram terminated."
           << endl;
      return 2;
    }

    ofstream result_file { options.result_path };
    if (!result_file)
    {
      cerr << "Cannot open result-file named \"" << options.result_path
           << "\".\nProgram terminated." << endl;
      return 3;
    }

    spl_processor_t processor { options.specification_path };
    report_request_t report_request = requested_report_request(processor, options);
    print_compatibility_notes(cerr,
                              program_name,
                              options,
                              processor.get_specification());

    const auto guardrails = guardrails_for(options);
    const auto plan = plan_report_execution(processor.get_specification().m_F,
                                            processor.get_specification()
                                                .system_space(),
                                            report_request);
    const auto approval = approve_execution_plan(plan, options, guardrails);
    if (!approval.has_value())
    {
      return 6;
    }
    report_request = approval->request;

    const auto approved_feasibility = estimate_report_feasibility(
        processor.get_specification().m_F,
        processor.get_specification().system_space(),
        report_request);
    if (!enforce_guardrails(
            approved_feasibility,
            guardrails,
            options,
            approval->guardrails_approved))
    {
      return 8;
    }

    feature_isolation_t fi { processor.get_specification(), report_request };
    const auto status = write_report(result_file, fi);
    return status;
  }

  /** @brief Executes the plan command end-to-end without creating a report file. */
  [[nodiscard]] int run_plan(const std::string& program_name,
                            const cli_options_t& options)
  {
    ifstream spl_file { options.specification_path };
    if (spl_file)
    {
      spl_file.close();
    }
    else
    {
      cerr << "Cannot open spl-specification-file named \""
           << options.specification_path << "\".\nProgram terminated."
           << endl;
      return 2;
    }

    spl_processor_t processor { options.specification_path };
    report_request_t report_request = requested_report_request(processor, options);
    print_compatibility_notes(cout,
                              program_name,
                              options,
                              processor.get_specification());

    const auto guardrails = guardrails_for(options);
    const auto plan = plan_report_execution(processor.get_specification().m_F,
                                            processor.get_specification()
                                                .system_space(),
                                            report_request);
    print_plan_summary(cout, plan, guardrails);
    print_drill_down_workflow(cout,
                              program_name.c_str(),
                              options,
                              processor.get_specification().m_F,
                              processor.get_specification().system_space(),
                              report_request,
                              guardrails,
                              true);
    return 0;
  }
} // namespace

/** @brief Executes the expression command to return only the minimized expression based on the found systems. */
  [[nodiscard]] int run_expression_from_systems(const std::string& program_name,
                               const cli_options_t& options)
  {
    
    ifstream spl_file { options.specification_path };
    if (spl_file)
    {
      spl_file.close();
    }
    else
    {
      cerr << "Cannot open spl-specification-file named \""
           << options.specification_path << "\".\nProgram terminated."
           << endl;
      return 2;
    }

    spl_processor_t processor { options.specification_path };

    report_request_t report_request = requested_report_request(processor, options);

    feature_isolation_t fi { processor.get_specification(), report_request, options.found_systems_in_alignment };

    return 0;

  }

/** @brief Prompts for the maximum grade to compute when it was not supplied via --max-grade. */
[[nodiscard]] std::optional<unsigned_integer> prompt_max_grade()
{
  while (true)
  {
    cerr << "Enter the maximum grade to compute (positive integer): " << flush;

    std::string line {};
    if (!std::getline(cin, line))
    {
      cerr << "\nInput closed before a grade was entered.\nProgram terminated."
           << endl;
      return std::nullopt;
    }

    const auto trimmed = trim_copy(line);
    try
    {
      std::size_t consumed { 0 };
      const auto  value = std::stoull(trimmed, &consumed);
      if (consumed == trimmed.size() && value >= 1)
      {
        return static_cast<unsigned_integer>(value);
      }
    }
    catch (...)
    {
    }
    cerr << "Please enter a positive integer." << endl;
  }
}

/** @brief Executes the traces command: precomputes a grade-bounded lookup file
 * mapping available-system-id combinations to their derived-feature expression. */
[[nodiscard]] int run_trace_export(const std::string& program_name,
                                   const cli_options_t& options)
{
  ifstream spl_file { options.specification_path };
  if (spl_file)
  {
    spl_file.close();
  }
  else
  {
    cerr << "Cannot open spl-specification-file named \""
         << options.specification_path << "\".\nProgram terminated."
         << endl;
    return 2;
  }

  auto max_grade = options.max_grade;
  if (!max_grade.has_value())
  {
    if (!stdin_is_terminal())
    {
      cerr << "Missing required option \"--max-grade=<N>\" for the traces"
              " command.\n"
           << "Program terminated." << endl;
      return 4;
    }
    max_grade = prompt_max_grade();
    if (!max_grade.has_value())
    {
      return 4;
    }
  }

  spl_processor_t processor { options.specification_path };
  const auto&     spec         = processor.get_specification();
  const auto&     system_space = spec.system_space();

  const std::vector<unsigned_integer> ordered_valid_system_ids(
      system_space.valid.begin(), system_space.valid.end());

  const auto traces = enumerate_bounded_traces(
      spec.m_F,
      ordered_valid_system_ids,
      std::vector<std::string>(spec.m_feature_names.begin(), spec.m_feature_names.end()),
      max_grade.value());

  ofstream output_file { options.result_path };
  if (!output_file)
  {
    cerr << "Cannot open output file named \"" << options.result_path
         << "\".\nProgram terminated." << endl;
    return 2;
  }

  std::set<std::string> written_keys {};
  unsigned_integer       written_count { 0 };
  for (const auto& trace : traces)
  {
    std::vector<unsigned_integer> available_ids {};
    for (auto position { zero }; position < ordered_valid_system_ids.size(); ++position)
    {
      if (!trace.systems_present.at(position))
      {
        continue;
      }
      const auto system_id = ordered_valid_system_ids.at(position);
      if (system_space.available.contains(system_id))
      {
        available_ids.push_back(system_id);
      }
    }
    if (available_ids.empty())
    {
      continue;
    }

    std::ostringstream key {};
    for (std::size_t index = 0; index < available_ids.size(); ++index)
    {
      if (index != 0)
      {
        key << ',';
      }
      // Paper-style 0-based system id, matching paper_system_label() minus its "s" prefix.
      key << (available_ids.at(index) - one);
    }
    const auto key_text = key.str();
    if (!written_keys.insert(key_text).second)
    {
      continue;
    }

    output_file << key_text << ": " << trace.label << '\n';
    ++written_count;
  }
  output_file.close();

  cerr << "Wrote " << written_count << " trace(s) up to grade "
       << max_grade.value() << " to \"" << options.result_path << "\"."
       << endl;
  (void)program_name;
  return 0;
}

/** @brief Entry point that routes between help, guided mode, analyze, and plan. */
int main(int argc, char* argv[])
{
  const auto program_name = program_display_name(argc > 0 ? argv[0] : nullptr);

  if (argc >= 2)
  {
    const std::string first_argument { argv[1] };
    if (is_help_token(first_argument))
    {
      if (argc >= 3)
      {
        const std::string second_argument { argv[2] };
        if (second_argument == "analyze")
        {
          print_analyze_help(cout, program_name);
          return 0;
        }
        if (second_argument == "plan")
        {
          print_plan_help(cout, program_name);
          return 0;
        }
        if (second_argument == "traces")
        {
          print_traces_help(cout, program_name);
          return 0;
        }
        if (is_guided_token(second_argument))
        {
          print_guided_help(cout, program_name);
          return 0;
        }
      }
      print_general_help(cout, program_name);
      return 0;
    }
    if (argc >= 3 && is_help_token(argv[2]))
    {
      if (first_argument == "analyze")
      {
        print_analyze_help(cout, program_name);
        return 0;
      }
      if (first_argument == "plan")
      {
        print_plan_help(cout, program_name);
        return 0;
      }
      if (first_argument == "traces")
      {
        print_traces_help(cout, program_name);
        return 0;
      }
      if (is_guided_token(first_argument))
      {
        print_guided_help(cout, program_name);
        return 0;
      }
    }
  }

  auto options = parse_cli_options(argc, argv);
  if (!options.has_value() && should_enter_guided_mode(argc, argv))
  {
    options = build_guided_options(argc, argv);
    if (!options.has_value())
    {
      if (!stdin_is_terminal())
      {
        print_general_help(cerr, program_name);
      }
      return 4;
    }
  }
  if (!options.has_value())
  {
    print_general_help(cerr, program_name);
    return 4;
  }

  switch (options->command)
  {
  case cli_command_t::analyze:
    return run_analyze(program_name, *options);
  case cli_command_t::plan:
    return run_plan(program_name, *options);
  case cli_command_t::expression_from_systems:
    return run_expression_from_systems(program_name, *options);
  case cli_command_t::trace_export:
    return run_trace_export(program_name, *options);
  }

  cerr << "Unknown command.\nProgram terminated." << endl;
  return 4;
}
