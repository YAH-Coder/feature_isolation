/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "../include/isolation.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace feature_isolation;

namespace
{
  using clock_t = std::chrono::steady_clock;
  using duration_ms_t = std::chrono::duration<double, std::milli>;

  struct benchmark_result_t
  {
      std::string scenario {};
      std::string benchmark {};
      std::size_t iterations_per_sample { 0 };
      double      median_ms_per_iteration { 0.0 };
  };

  void write_unconstrained_spec(
      const std::filesystem::path&    path,
      unsigned_integer                feature_count,
      const std::vector<std::string>& category_filter = {})
  {
    std::ofstream output(path);
    output << "SPL Benchmark" << feature_count << "F\n";
    output << "FEATURES " << feature_count;
    for (unsigned_integer index { 0 }; index < feature_count; ++index)
    {
      output << ' ' << 'f' << (index + 1);
    }
    output << "\n";
    if (!category_filter.empty())
    {
      output << "CATEGORY_FILTER";
      for (const auto& category : category_filter)
      {
        output << ' ' << category;
      }
      output << "\n";
    }
  }

  unsigned_integer benchmark_feature_id(const feature_isolation_t& isolation)
  {
    for (const auto& record : isolation.m_features.feature_registry())
    {
      if (record.category == theory_feature_category_t::complex)
      {
        return record.id;
      }
    }
    for (const auto& record : isolation.m_features.feature_registry())
    {
      if (record.category == theory_feature_category_t::and_feature
          || record.category == theory_feature_category_t::or_feature)
      {
        return record.id;
      }
    }
    return isolation.m_features.feature_registry().front().id;
  }

  template<typename Fn>
  benchmark_result_t benchmark(const std::string& scenario,
                               const std::string& name,
                               Fn&&               fn)
  {
    fn();

    const auto single_start = clock_t::now();
    fn();
    const auto single_elapsed
        = std::max(duration_ms_t(clock_t::now() - single_start).count(), 0.001);

    const auto target_sample_ms = 75.0;
    const std::size_t iterations_per_sample
        = std::max<std::size_t>(1,
                                static_cast<std::size_t>(target_sample_ms / single_elapsed));

    std::vector<double> samples {};
    samples.reserve(5);
    for (std::size_t sample { 0 }; sample < 5; ++sample)
    {
      const auto sample_start = clock_t::now();
      for (std::size_t iteration { 0 }; iteration < iterations_per_sample; ++iteration)
      {
        fn();
      }
      const auto elapsed = duration_ms_t(clock_t::now() - sample_start).count();
      samples.push_back(elapsed / static_cast<double>(iterations_per_sample));
    }

    std::ranges::sort(samples);
    return { scenario, name, iterations_per_sample, samples.at(samples.size() / 2) };
  }
} // namespace

int main()
{
  const auto generated_dir = std::filesystem::path("testing/generated");
  std::filesystem::create_directories(generated_dir);

  struct scenario_config_t
  {
      unsigned_integer        feature_count { 0 };
      std::string             scenario {};
      std::vector<std::string> category_filter {};
      bool                    benchmark_isolation { true };
  };

  const std::vector<scenario_config_t> scenarios {
    { 3U, "3f_unconstrained", {}, true },
    { 4U,
      "4f_unconstrained_no_complex",
      { "Core", "Fundamental", "Not", "And", "Or" },
      false }
  };

  std::vector<benchmark_result_t> results {};
  for (const auto& config : scenarios)
  {
    const auto spec_path = generated_dir / (config.scenario + ".spl");
    write_unconstrained_spec(spec_path, config.feature_count, config.category_filter);

    const spl_processor_t processor(spec_path.string());
    const auto& spec = processor.get_specification();
    const std::vector<std::string> feature_names(spec.m_feature_names.begin(),
                                                 spec.m_feature_names.end());
    const feature_isolation_t isolation(spec_path.string());
    const auto feature_id = benchmark_feature_id(isolation);

    results.push_back(benchmark(config.scenario,
                                "theory_system_space",
                                [&spec]() { return theory_system_space(spec); }));

    results.push_back(benchmark(config.scenario,
                                "feature_analysis_ctor",
                                [&spec, &feature_names]() {
                                  return feature_analysis_t(spec.m_F,
                                                            theory_system_space(spec),
                                                            feature_names,
                                                            spec.m_category_filter);
                                }));

    if (config.benchmark_isolation)
    {
      results.push_back(benchmark(config.scenario,
                                  "minimal_differences",
                                  [&isolation, feature_id]() {
                                    return isolation.minimal_differences(feature_id);
                                  }));
    }
  }

  std::cout << "scenario\tbenchmark\titerations/sample\tmedian_ms/iteration\n";
  std::cout << std::fixed << std::setprecision(3);
  for (const auto& result : results)
  {
    std::cout << result.scenario << '\t' << result.benchmark << '\t'
              << result.iterations_per_sample << '\t'
              << result.median_ms_per_iteration << '\n';
  }
  return 0;
}
