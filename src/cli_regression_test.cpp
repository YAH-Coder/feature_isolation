#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#if __has_include(<pty.h>)
#include <pty.h>
#elif __has_include(<util.h>)
#include <util.h>
#else
#error "A PTY header is required for CLI regression tests."
#endif

namespace
{
  namespace fs = std::filesystem;

  struct process_result_t
  {
    int         exit_code {};
    std::string output {};
  };

  [[nodiscard]] auto root_path() -> const fs::path&
  {
    static const fs::path root = fs::current_path();
    return root;
  }

  [[nodiscard]] auto generated_dir() -> fs::path
  {
    return root_path() / "testing" / "generated";
  }

  [[nodiscard]] auto cli_path() -> fs::path
  {
    return generated_dir() / "cli";
  }

  [[nodiscard]] auto read_all_from_fd(const int fd) -> std::string
  {
    std::string result {};
    char        buffer[4096];
    while (true)
    {
      const auto count = ::read(fd, buffer, sizeof(buffer));
      if (count > 0)
      {
        result.append(buffer, static_cast<std::size_t>(count));
        continue;
      }
      if (count == 0)
      {
        break;
      }
      if (errno == EINTR)
      {
        continue;
      }
      throw std::runtime_error(std::string { "read failed: " } + std::strerror(errno));
    }
    return result;
  }

  [[nodiscard]] auto run_process(const std::vector<std::string>& args,
                                 const std::string&              input = {})
      -> process_result_t
  {
    if (args.empty())
    {
      throw std::runtime_error("run_process requires at least one argument");
    }

    int stdout_pipe[2] { -1, -1 };
    int stdin_pipe[2] { -1, -1 };
    if (::pipe(stdout_pipe) != 0 || ::pipe(stdin_pipe) != 0)
    {
      throw std::runtime_error(std::string { "pipe failed: " } + std::strerror(errno));
    }

    const pid_t pid = ::fork();
    if (pid < 0)
    {
      throw std::runtime_error(std::string { "fork failed: " } + std::strerror(errno));
    }

    if (pid == 0)
    {
      ::chdir(root_path().c_str());
      ::dup2(stdin_pipe[0], STDIN_FILENO);
      ::dup2(stdout_pipe[1], STDOUT_FILENO);
      ::dup2(stdout_pipe[1], STDERR_FILENO);
      ::close(stdin_pipe[0]);
      ::close(stdin_pipe[1]);
      ::close(stdout_pipe[0]);
      ::close(stdout_pipe[1]);

      std::vector<char*> argv {};
      argv.reserve(args.size() + 1);
      for (const auto& arg : args)
      {
        argv.push_back(const_cast<char*>(arg.c_str()));
      }
      argv.push_back(nullptr);
      ::execvp(argv.front(), argv.data());
      std::perror("execvp");
      _exit(127);
    }

    ::close(stdin_pipe[0]);
    ::close(stdout_pipe[1]);

    if (!input.empty())
    {
      const char* data = input.data();
      std::size_t total_written {};
      while (total_written < input.size())
      {
        const auto written = ::write(stdin_pipe[1],
                                     data + total_written,
                                     input.size() - total_written);
        if (written < 0)
        {
          if (errno == EINTR)
          {
            continue;
          }
          throw std::runtime_error(std::string { "write failed: " } + std::strerror(errno));
        }
        total_written += static_cast<std::size_t>(written);
      }
    }
    ::close(stdin_pipe[1]);

    std::string output = read_all_from_fd(stdout_pipe[0]);
    ::close(stdout_pipe[0]);

    int status {};
    if (::waitpid(pid, &status, 0) < 0)
    {
      throw std::runtime_error(std::string { "waitpid failed: " } + std::strerror(errno));
    }

    int exit_code = 1;
    if (WIFEXITED(status))
    {
      exit_code = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
      exit_code = 128 + WTERMSIG(status);
    }

    return { exit_code, output };
  }

  struct interaction_t
  {
    std::string prompt {};
    std::string reply {};
  };

  [[nodiscard]] auto run_interactive(const std::vector<std::string>& args,
                                     const std::vector<interaction_t>& interactions,
                                     const int timeout_seconds = 20)
      -> process_result_t
  {
    int   master_fd = -1;
    pid_t pid = ::forkpty(&master_fd, nullptr, nullptr, nullptr);
    if (pid < 0)
    {
      throw std::runtime_error(std::string { "forkpty failed: " } + std::strerror(errno));
    }

    if (pid == 0)
    {
      ::chdir(root_path().c_str());
      std::vector<char*> argv {};
      argv.reserve(args.size() + 1);
      for (const auto& arg : args)
      {
        argv.push_back(const_cast<char*>(arg.c_str()));
      }
      argv.push_back(nullptr);
      ::execvp(argv.front(), argv.data());
      std::perror("execvp");
      _exit(127);
    }

    std::string transcript {};
    std::size_t interaction_index {};
    bool        child_exited = false;
    int         status {};

    while (true)
    {
      if (!child_exited)
      {
        const pid_t waited = ::waitpid(pid, &status, WNOHANG);
        if (waited == pid)
        {
          child_exited = true;
        }
      }

      while (interaction_index < interactions.size() &&
             transcript.find(interactions.at(interaction_index).prompt) != std::string::npos)
      {
        const std::string reply_line = interactions.at(interaction_index).reply + "\n";
        if (::write(master_fd, reply_line.data(), reply_line.size()) < 0)
        {
          throw std::runtime_error(std::string { "interactive write failed: " }
                                   + std::strerror(errno));
        }
        ++interaction_index;
      }

      if (child_exited)
      {
        char buffer[4096];
        const auto count = ::read(master_fd, buffer, sizeof(buffer));
        if (count > 0)
        {
          transcript.append(buffer, static_cast<std::size_t>(count));
          continue;
        }
        if (count == 0 || (count < 0 && errno == EIO))
        {
          break;
        }
        if (count < 0 && errno == EINTR)
        {
          continue;
        }
        throw std::runtime_error(std::string { "interactive read failed: " }
                                 + std::strerror(errno));
      }

      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(master_fd, &read_fds);
      timeval timeout { timeout_seconds, 0 };
      const int select_result = ::select(master_fd + 1,
                                         &read_fds,
                                         nullptr,
                                         nullptr,
                                         &timeout);
      if (select_result < 0)
      {
        if (errno == EINTR)
        {
          continue;
        }
        throw std::runtime_error(std::string { "select failed: " } + std::strerror(errno));
      }
      if (select_result == 0)
      {
        ::kill(pid, SIGKILL);
        ::waitpid(pid, &status, 0);
        throw std::runtime_error("interactive test timed out");
      }

      char buffer[4096];
      const auto count = ::read(master_fd, buffer, sizeof(buffer));
      if (count > 0)
      {
        transcript.append(buffer, static_cast<std::size_t>(count));
        continue;
      }
      if (count == 0 || (count < 0 && errno == EIO))
      {
        break;
      }
      if (count < 0 && errno == EINTR)
      {
        continue;
      }
      throw std::runtime_error(std::string { "interactive read failed: " }
                               + std::strerror(errno));
    }

    ::close(master_fd);

    int exit_code = 1;
    if (WIFEXITED(status))
    {
      exit_code = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
      exit_code = 128 + WTERMSIG(status);
    }

    if (interaction_index != interactions.size())
    {
      std::ostringstream message {};
      message << "Interactive test did not observe prompt "
              << interactions.at(interaction_index).prompt << "\n--- transcript ---\n"
              << transcript << "\n--- end ---";
      throw std::runtime_error(message.str());
    }

    return { exit_code, transcript };
  }

  void assert_contains(const std::string& text,
                       const std::string& needle,
                       const std::string& context)
  {
    if (!text.contains(needle))
    {
      std::ostringstream message {};
      message << "Expected to find '" << needle << "' in " << context
              << ".\n--- output ---\n" << text << "\n--- end ---";
      throw std::runtime_error(message.str());
    }
  }

  void assert_not_contains(const std::string& text,
                           const std::string& needle,
                           const std::string& context)
  {
    if (text.contains(needle))
    {
      std::ostringstream message {};
      message << "Did not expect to find '" << needle << "' in " << context
              << ".\n--- output ---\n" << text << "\n--- end ---";
      throw std::runtime_error(message.str());
    }
  }

  void write_unconstrained_spec(const fs::path& path, const int feature_count)
  {
    std::ofstream stream { path };
    if (!stream)
    {
      throw std::runtime_error("Could not write generated SPL file");
    }
    stream << "SPL Benchmark" << feature_count << "F\nFEATURES " << feature_count;
    for (int index = 0; index < feature_count; ++index)
    {
      stream << ' ' << 'f' << (index + 1);
    }
    stream << '\n';
  }

  void test_help_output()
  {
    const auto result = run_process({ cli_path().string(), "--help" });
    if (result.exit_code != 0)
    {
      throw std::runtime_error(result.output);
    }
    assert_contains(result.output,
                    "Derived-feature-space report generator",
                    "--help output");
    assert_contains(result.output,
                    "guided [analyze|plan] [seed-inputs]",
                    "--help output");
    assert_contains(result.output,
                    "derived features induced by those valid systems",
                    "--help output");
  }

  void test_plan_output()
  {
    const auto result = run_process(
        { cli_path().string(), "plan", "spl_specs/new_dsl/valid/unconstrained_2f.spl" });
    if (result.exit_code != 0)
    {
      throw std::runtime_error(result.output);
    }
    assert_contains(result.output, "Requested plan", "plan output");
    assert_contains(result.output, "Recommended plan", "plan output");
    assert_contains(result.output, "fundamental features=2", "plan output");
    assert_contains(result.output, "valid systems=4", "plan output");
    assert_contains(result.output, "derived features≈16", "plan output");
    assert_contains(result.output, "Suggested drill-down workflow", "plan output");
  }

  void test_summary_report_generation()
  {
    const fs::path report_path = generated_dir() / "cli_summary_report.txt";
    const auto result = run_process({ cli_path().string(),
                                      "analyze",
                                      "spl_specs/new_dsl/valid/unconstrained_2f.spl",
                                      report_path.string(),
                                      "-min-diffs=false",
                                      "-feature-names=false",
                                      "--mode=summary",
                                      "--yes",
                                      "--non-interactive" });
    if (result.exit_code != 0)
    {
      throw std::runtime_error(result.output);
    }
    std::ifstream report_stream { report_path };
    std::stringstream buffer {};
    buffer << report_stream.rdbuf();
    const auto report = buffer.str();
    assert_contains(report, "=== SPL SPECIFICATION ===", "summary report");
    assert_contains(report, "=== THEORY SUMMARY ===", "summary report");
    assert_contains(report, "CATEGORY\tCOUNT", "summary report");
    assert_not_contains(report, "THEORY DSL COVERAGE", "summary report");
    assert_not_contains(report, "NOT DIRECTLY COVERED", "summary report");
    assert_not_contains(report, "=== ISOLATION RESULTS ===", "summary report");
  }


  void test_full_report_omits_dsl_coverage_section()
  {
    const fs::path report_path = generated_dir() / "cli_full_report.txt";
    const auto result = run_process({ cli_path().string(),
                                      "analyze",
                                      "spl_specs/new_dsl/valid/unconstrained_2f.spl",
                                      report_path.string(),
                                      "-min-diffs=false",
                                      "-feature-names=false",
                                      "--mode=full",
                                      "--categories=impossible,core,fundamental,not,and,or,complex",
                                      "--raw-terms=false",
                                      "--simplified-terms=true",
                                      "--isolation=false",
                                      "--yes",
                                      "--non-interactive" });
    if (result.exit_code != 0)
    {
      throw std::runtime_error(result.output);
    }
    std::ifstream report_stream { report_path };
    std::stringstream buffer {};
    buffer << report_stream.rdbuf();
    const auto report = buffer.str();
    assert_contains(report, "=== SPL SPECIFICATION ===", "full report");
    assert_contains(report, "=== SYSTEMS ===", "full report");
    assert_contains(report, "=== FEATURES BY CATEGORY ===", "full report");
    assert_not_contains(report, "THEORY DSL COVERAGE", "full report");
    assert_not_contains(report, "NOT DIRECTLY COVERED", "full report");
  }

  void test_guardrails_and_recommended_execution()
  {
    const fs::path spec_path = generated_dir() / "cli_unconstrained_4f.spl";
    write_unconstrained_spec(spec_path, 4);

    const fs::path refused_report = generated_dir() / "cli_risky_refused.txt";
    const auto refused = run_process({ cli_path().string(),
                                       "analyze",
                                       spec_path.string(),
                                       refused_report.string(),
                                       "-min-diffs=false",
                                       "-feature-names=false",
                                       "--yes",
                                       "--non-interactive" });
    if (refused.exit_code != 8)
    {
      std::ostringstream message {};
      message << "Expected exit code 8, got " << refused.exit_code << ".\n"
              << refused.output;
      throw std::runtime_error(message.str());
    }
    assert_contains(refused.output, "Execution guardrail warning", "guardrail refusal output");
    assert_contains(refused.output, "recommended mode=summary", "guardrail refusal output");

    const fs::path accepted_report = generated_dir() / "cli_risky_accepted.txt";
    const auto accepted = run_process({ cli_path().string(),
                                        "analyze",
                                        spec_path.string(),
                                        accepted_report.string(),
                                        "-min-diffs=false",
                                        "-feature-names=false",
                                        "--accept-recommended",
                                        "--non-interactive" });
    if (accepted.exit_code != 0)
    {
      throw std::runtime_error(accepted.output);
    }

    std::ifstream report_stream { accepted_report };
    std::stringstream buffer {};
    buffer << report_stream.rdbuf();
    const auto report = buffer.str();
    assert_contains(report, "=== THEORY SUMMARY ===", "accepted recommended report");
    assert_not_contains(report, "=== SYSTEMS ===", "accepted recommended report");
  }

  void test_compatibility_notes()
  {
    const fs::path legacy_report = generated_dir() / "cli_legacy_report.txt";
    const auto legacy = run_process({ cli_path().string(),
                                      "spl_specs/new_dsl/valid/unconstrained_2f.spl",
                                      legacy_report.string(),
                                      "-min-diffs=false",
                                      "-feature-names=false",
                                      "--mode=summary" });
    if (legacy.exit_code != 0)
    {
      throw std::runtime_error(legacy.output);
    }
    assert_contains(legacy.output,
                    "Compatibility note: the legacy positional analyze invocation is still accepted",
                    "legacy invocation output");

    const fs::path override_report = generated_dir() / "cli_override_report.txt";
    const auto override = run_process({ cli_path().string(),
                                        "analyze",
                                        "spl_specs/new_dsl/valid/constraints_boolean_and_count.spl",
                                        override_report.string(),
                                        "-min-diffs=false",
                                        "-feature-names=false",
                                        "--mode=summary" });
    if (override.exit_code != 0)
    {
      throw std::runtime_error(override.output);
    }
    assert_contains(override.output,
                    "Compatibility note: this run overrides the SPL CATEGORY_FILTER hint",
                    "CATEGORY_FILTER override output");
  }

  void test_guided_plan_flow()
  {
    const auto result = run_interactive(
        { cli_path().string(), "guided", "plan", "spl_specs/new_dsl/valid/unconstrained_2f.spl" },
        {
          { "Select CLI command [plan]:", "" },
          { "Enter SPL specification path [spl_specs/new_dsl/valid/unconstrained_2f.spl]:", "" },
          { "Choose a report mode.", "1" },
          { "Adjust advanced options [y/N]:", "" },
        });
    if (result.exit_code != 0)
    {
      throw std::runtime_error(result.output);
    }
    assert_contains(result.output, "Guided mode", "guided plan transcript");
    assert_contains(result.output, "Model overview", "guided plan transcript");
    assert_contains(result.output, "Suggested default:", "guided plan transcript");
    assert_contains(result.output, "Run summary", "guided plan transcript");
    assert_not_contains(result.output, "Planner recommendation", "guided plan transcript");
    assert_not_contains(result.output, "Selected report", "guided plan transcript");
  }
}

int main()
{
  try
  {
    fs::create_directories(generated_dir());
    std::cout << "CLI regression tests\n";

    const std::vector<std::pair<std::string, void (*)()>> tests {
      { "help output", test_help_output },
      { "plan output", test_plan_output },
      { "summary report generation", test_summary_report_generation },
      { "full report omits DSL coverage section", test_full_report_omits_dsl_coverage_section },
      { "guardrails and recommended execution", test_guardrails_and_recommended_execution },
      { "compatibility notes", test_compatibility_notes },
      { "guided plan flow", test_guided_plan_flow },
    };

    for (const auto& [name, test] : tests)
    {
      test();
      std::cout << "  [ok] " << name << '\n';
    }
    std::cout << "All CLI regression tests passed.\n";
    return 0;
  }
  catch (const std::exception& exception)
  {
    std::cerr << exception.what() << std::endl;
    return 1;
  }
}
