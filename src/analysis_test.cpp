/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#define CATCH_CONFIG_MAIN
#include "../include/catch.hpp"
#include "../include/feature_analysis.hpp"
#include <map>
#include <sstream>

using namespace feature_isolation;

namespace
{
  [[nodiscard]] system_ids_t make_system_ids(std::initializer_list<unsigned_integer> values)
  {
    system_ids_t ids {};
    ids.insert(values.begin(), values.end());
    return ids;
  }

  [[nodiscard]] system_space_t unconstrained_system_space(unsigned_integer feature_count)
  {
    system_space_t space {};
    const auto system_count = power2(feature_count);
    for (auto system_id { one }; system_id <= system_count; ++system_id)
    {
      space.valid.insert(system_id);
    }
    space.available = space.valid;
    return space;
  }

  [[nodiscard]] system_space_t four_system_space()
  {
    return unconstrained_system_space(2);
  }

  [[nodiscard]] feature_analysis_t two_feature_theory_analysis()
  {
    const auto space = four_system_space();
    return feature_analysis_t(2, space, { "fx", "fy" });
  }

  [[nodiscard]] std::set<std::string>
      simplified_terms(const feature_analysis_t& analysis)
  {
    std::set<std::string> terms {};
    for (const auto& record : analysis.feature_registry())
    {
      terms.insert(record.simplified_term);
    }
    return terms;
  }
} // namespace

TEST_CASE("system_t prefers the generic feature-id set when present", "[system_t][theory]")
{
  system_t system;
  system.feature_ids.insert(1);
  system.feature_ids.insert(2);
  system.feature_ids.insert(5);

  REQUIRE(system.T() == 3);
  REQUIRE(system.all_features().size() == 3);
  REQUIRE(system.all_features().count(1) == 1);
  REQUIRE(system.all_features().count(5) == 1);
}

TEST_CASE("theory registry initialization exposes system space and registry",
          "[feature_analysis_t][theory]")
{
  system_ids_t valid_systems;
  valid_systems.insert(1);
  valid_systems.insert(3);
  valid_systems.insert(4);

  system_ids_t available_systems;
  available_systems.insert(3);
  available_systems.insert(4);

  system_ids_t missing_systems;
  missing_systems.insert(1);

  feature_analysis_t analysis(
      2, system_space_t { valid_systems, available_systems, missing_systems });

  REQUIRE(analysis.uses_theory_registry());
  REQUIRE(analysis.valid_system_count() == 3);
  REQUIRE(analysis.feature_count() == 8);
  REQUIRE(analysis.system_count() == 4);
  REQUIRE(analysis.system_space().valid == valid_systems);
  REQUIRE(analysis.system_space().available == available_systems);
  REQUIRE(analysis.system_space().missing == missing_systems);
  REQUIRE(analysis.feature_registry().size() == 8);
  REQUIRE(analysis.m_systems.size() == 3);
  REQUIRE(analysis.system_by_id(3).all_features().size() == 4);
  REQUIRE(analysis.system_by_id(4).all_features().size() == 4);
}

TEST_CASE("theory registry classifies the unconstrained two-feature example",
          "[feature_analysis_t][theory][classification]")
{
  auto analysis = two_feature_theory_analysis();

  std::map<theory_feature_category_t, size_t> counts {};
  for (const auto& record : analysis.feature_registry())
  {
    counts[record.category]++;
  }

  REQUIRE(analysis.feature_count() == 16);
  REQUIRE(counts[theory_feature_category_t::impossible] == 1);
  REQUIRE(counts[theory_feature_category_t::core] == 1);
  REQUIRE(counts[theory_feature_category_t::fundamental] == 2);
  REQUIRE(counts[theory_feature_category_t::not_feature] == 2);
  REQUIRE(counts[theory_feature_category_t::and_feature] == 4);
  REQUIRE(counts[theory_feature_category_t::or_feature] == 4);
  REQUIRE(counts[theory_feature_category_t::complex] == 2);
}

TEST_CASE("paper example category membership matches the published two-feature example",
          "[feature_analysis_t][theory][paper]")
{
  auto analysis = two_feature_theory_analysis();

  std::map<theory_feature_category_t, std::set<std::set<unsigned_integer>>> actual {};

  for (const auto& record : analysis.feature_registry())
  {
    std::set<unsigned_integer> systems {};
    for (std::size_t index = 0; index < record.systems_present.size(); ++index)
    {
      if (record.systems_present[index])
      {
        systems.insert(static_cast<unsigned_integer>(index + 1));
      }
    }
    actual[record.category].insert(systems);
  }

  REQUIRE(actual[theory_feature_category_t::impossible]
          == std::set<std::set<unsigned_integer>> { {} });
  REQUIRE(actual[theory_feature_category_t::core]
          == std::set<std::set<unsigned_integer>> { {1, 2, 3, 4} });
  REQUIRE(actual[theory_feature_category_t::fundamental]
          == std::set<std::set<unsigned_integer>> { {2, 4}, {3, 4} });
  REQUIRE(actual[theory_feature_category_t::not_feature]
          == std::set<std::set<unsigned_integer>> { {1, 2}, {1, 3} });
  REQUIRE(actual[theory_feature_category_t::and_feature]
          == std::set<std::set<unsigned_integer>> { {1}, {2}, {3}, {4} });
  REQUIRE(actual[theory_feature_category_t::or_feature]
          == std::set<std::set<unsigned_integer>> { {1, 2, 3}, {1, 2, 4}, {1, 3, 4}, {2, 3, 4} });
  REQUIRE(actual[theory_feature_category_t::complex]
          == std::set<std::set<unsigned_integer>> { {1, 4}, {2, 3} });
}

TEST_CASE("theory registry accessors resolve category and bitvector by feature id",
          "[feature_analysis_t][theory][lookup]")
{
  auto analysis = two_feature_theory_analysis();

  const auto first = analysis.at(1);
  const auto last  = analysis.at(analysis.feature_count());

  REQUIRE(first.size() == 4);
  REQUIRE(last.size() == 4);
  REQUIRE(analysis.category_name(1) == "Impossible");
  REQUIRE(analysis.category_name(analysis.feature_count()) == "Core");
}

TEST_CASE("theory-native reporting uses new category headers",
          "[feature_analysis_t][theory][print]")
{
  auto analysis = two_feature_theory_analysis();

  std::ostringstream systems_os;
  analysis.print_systems_statistics(systems_os);
  REQUIRE(systems_os.str().find("Impossible") != std::string::npos);
  REQUIRE(systems_os.str().find("Fundamental") != std::string::npos);
  REQUIRE(systems_os.str().find("DF") == std::string::npos);

  std::ostringstream features_os;
  analysis.print_features(features_os);
  REQUIRE(features_os.str().find("β(t_i)") != std::string::npos);
  REQUIRE(features_os.str().find("ℓ_T(t_i)(simplified)") != std::string::npos);
  REQUIRE(features_os.str().find("DESCRIPTION") == std::string::npos);

  std::ostringstream grouped_systems_os;
  analysis.print_systems_features(grouped_systems_os);
  REQUIRE(grouped_systems_os.str().find("Impossible") != std::string::npos);
  REQUIRE(grouped_systems_os.str().find("Complex") != std::string::npos);

  std::ostringstream summary_os;
  analysis.print(summary_os);
  REQUIRE(summary_os.str().find("|S_valid|") != std::string::npos);
  REQUIRE(summary_os.str().find("|T|") != std::string::npos);
  REQUIRE(summary_os.str().find("IN PRINCIPLE") == std::string::npos);
}


TEST_CASE("system reports only materialize valid systems",
          "[feature_analysis_t][theory][valid-systems-only]")
{
  system_space_t constrained_space {};
  constrained_space.valid = make_system_ids({ 1, 3, 4 });
  constrained_space.available = make_system_ids({ 3, 4 });
  constrained_space.missing = make_system_ids({ 1 });

  feature_analysis_t analysis(2, constrained_space, { "fx", "fy" });

  std::ostringstream systems_os;
  analysis.print_systems_statistics(systems_os);
  const auto systems_text = systems_os.str();
  REQUIRE(systems_text.find("\n2\ts1\t") == std::string::npos);
  REQUIRE(systems_text.find("\n1\ts0\tmissing") != std::string::npos);
  REQUIRE(systems_text.find("\n3\ts2\tavailable") != std::string::npos);
  REQUIRE(systems_text.find("\n4\ts3\tavailable") != std::string::npos);
}

TEST_CASE("theory registry prefers CNF over equally concise DNF for complex terms",
          "[feature_analysis_t][theory][simplification]")
{
  auto analysis = two_feature_theory_analysis();
  const auto terms = simplified_terms(analysis);

  REQUIRE(terms.contains("(fx ∨ fy) ∧ (¬fx ∨ ¬fy)"));
  REQUIRE(terms.contains("(fy ∨ ¬fx) ∧ (fx ∨ ¬fy)"));
  REQUIRE_FALSE(terms.contains("fx ∧ ¬fy ∨ fy ∧ ¬fx"));
  REQUIRE_FALSE(terms.contains("fy ∧ ¬fx ∨ fx ∧ fy"));
}

TEST_CASE("constrained theory registry reproduces the paper term set for fx implies fy",
          "[feature_analysis_t][theory][constraints]")
{
  system_space_t constrained_space {};
  constrained_space.valid = make_system_ids({ 1, 3, 4 });
  constrained_space.available = constrained_space.valid;

  feature_analysis_t analysis(2, constrained_space, { "fx", "fy" });

  const std::set<std::string> expected_terms {
    "⊥",
    "fx",
    "fy ∧ ¬fx",
    "fy",
    "¬fy",
    "fx ∨ ¬fy",
    "¬fx",
    "⊤"
  };

  REQUIRE(analysis.feature_count() == 8);
  REQUIRE(simplified_terms(analysis) == expected_terms);
}

TEST_CASE("theory-native feature names prefer simplified terms",
          "[feature_analysis_t][theory][names]")
{
  auto analysis = two_feature_theory_analysis();

  REQUIRE(analysis.category_name(1) == "Impossible");
  REQUIRE(analysis.category_name(analysis.feature_count()) == "Core");
  REQUIRE(!analysis.feature_registry().at(0).simplified_term.empty());
}


TEST_CASE("category-filtered theory analysis preserves ids while pruning complex details",
          "[feature_analysis_t][theory][category_filter]")
{
  const auto space = four_system_space();
  feature_analysis_t analysis(2,
                              space,
                              { "fx", "fy" },
                              std::vector<theory_feature_category_t> {
                                  theory_feature_category_t::core,
                                  theory_feature_category_t::fundamental,
                                  theory_feature_category_t::not_feature,
                                  theory_feature_category_t::and_feature,
                                  theory_feature_category_t::or_feature });

  const auto& registry = analysis.feature_registry();
  REQUIRE(registry.size() == 16);
  REQUIRE_FALSE(analysis.includes_category(theory_feature_category_t::complex));

  bool saw_complex = false;
  for (const auto& record : registry)
  {
    REQUIRE(record.id >= 1);
    if (record.category == theory_feature_category_t::complex)
    {
      saw_complex = true;
      REQUIRE(record.simplified_term.empty());
      REQUIRE(record.raw_term.empty());
      REQUIRE_FALSE(analysis.includes_feature(record.id));
    }
    else if (analysis.includes_category(record.category))
    {
      REQUIRE_FALSE(record.simplified_term.empty());
    }
  }
  REQUIRE(saw_complex);

  std::ostringstream features_os;
  analysis.print_features(features_os);
  REQUIRE(features_os.str().find("Complex") == std::string::npos);
}

TEST_CASE("report requests separate CLI output policy from DSL-level hints",
          "[feature_analysis_t][theory][report_request]")
{
  const auto space = four_system_space();
  auto request = report_request_t::from_category_hint(
      std::vector<theory_feature_category_t> {
          theory_feature_category_t::core,
          theory_feature_category_t::fundamental,
          theory_feature_category_t::not_feature,
          theory_feature_category_t::and_feature,
          theory_feature_category_t::or_feature });

  REQUIRE(request.includes_category(theory_feature_category_t::core));
  REQUIRE_FALSE(request.includes_category(theory_feature_category_t::complex));
  REQUIRE_FALSE(request.include_complex_details);

  feature_analysis_t analysis(2, space, { "fx", "fy" }, request);
  REQUIRE_FALSE(analysis.report_request().include_complex_details);
  REQUIRE_FALSE(analysis.includes_category(theory_feature_category_t::complex));
}

TEST_CASE("summary report requests skip raw and simplified term materialization",
          "[feature_analysis_t][theory][report_request]")
{
  const auto space = unconstrained_system_space(4);
  auto request = report_request_t::unrestricted();
  request.mode = report_mode_t::summary;
  request.include_raw_terms = false;
  request.include_simplified_terms = false;
  request.include_isolation = false;
  request.include_complex_details = false;

  feature_analysis_t analysis(4, space, {}, request);

  for (const auto& record : analysis.feature_registry())
  {
    REQUIRE(record.simplified_term.empty());
    REQUIRE(record.raw_term.empty());
    REQUIRE(record.display_name.empty());
  }
}

TEST_CASE("theory-native constructor derives the candidate system space from the fundamental feature count",
          "[feature_analysis_t][theory][constructor]")
{
  const auto space = four_system_space();
  feature_analysis_t analysis(2, space, { "fx", "fy" });

  REQUIRE(analysis.uses_theory_registry());
  REQUIRE(analysis.system_count() == 4);
  REQUIRE(analysis.valid_system_count() == 4);
  REQUIRE(analysis.system_space().valid == space.valid);
  REQUIRE(analysis.system_space().available == space.available);
  REQUIRE(analysis.system_space().missing == space.missing);
}

TEST_CASE("theory-native constructor rejects incomplete system-space partitions",
          "[feature_analysis_t][theory][validation]")
{
  system_space_t invalid_space {};
  invalid_space.valid = make_system_ids({ 1, 2, 3, 4 });
  invalid_space.available = make_system_ids({ 1, 2, 3 });
  invalid_space.missing = make_system_ids({ 1 });

  REQUIRE_THROWS_AS(feature_analysis_t(2, invalid_space, { "fx", "fy" }),
                    std::invalid_argument);
}



TEST_CASE("feasibility estimator classifies small theory-native reports as safe",
          "[report_feasibility_t][estimate]")
{
  const auto estimate = estimate_report_feasibility(
      2, four_system_space(), report_request_t::unrestricted());

  REQUIRE(estimate.base_feature_count == 2);
  REQUIRE(estimate.valid_system_count == 4);
  REQUIRE(estimate.theory_feature_count == 16);
  REQUIRE(estimate.risk == feasibility_risk_t::safe);
  REQUIRE_FALSE(estimate.approval_recommended);
  REQUIRE(estimate.summary().find("risk=safe") != std::string::npos);
}

TEST_CASE("feasibility estimator detects the pruning benefit of skipping complex details",
          "[report_feasibility_t][estimate]")
{
  const auto space = unconstrained_system_space(4);
  const auto full_estimate
      = estimate_report_feasibility(4, space, report_request_t::unrestricted());

  auto filtered_request = report_request_t::from_category_hint(
      std::vector<theory_feature_category_t> {
          theory_feature_category_t::core,
          theory_feature_category_t::fundamental,
          theory_feature_category_t::not_feature,
          theory_feature_category_t::and_feature,
          theory_feature_category_t::or_feature });
  filtered_request.include_isolation = false;
  const auto filtered_estimate
      = estimate_report_feasibility(4, space, filtered_request);

  REQUIRE(filtered_estimate.complex_feature_lower_bound > 0);
  REQUIRE(filtered_estimate.included_feature_upper_bound
          < full_estimate.included_feature_upper_bound);
  REQUIRE(filtered_estimate.estimated_total_runtime_ms
          < full_estimate.estimated_total_runtime_ms);
  REQUIRE(filtered_estimate.estimated_output_bytes
          < full_estimate.estimated_output_bytes);
}

TEST_CASE("feasibility estimator marks large exhaustive reports as impractical",
          "[report_feasibility_t][estimate]")
{
  const auto space = unconstrained_system_space(5);
  const auto estimate = estimate_report_feasibility(
      5, space, report_request_t::unrestricted());

  REQUIRE(estimate.valid_system_count == 32);
  REQUIRE(estimate.theory_feature_count == power2(32));
  REQUIRE(estimate.risk == feasibility_risk_t::impractical);
  REQUIRE(estimate.approval_recommended);
}




TEST_CASE("feasibility estimator keeps constrained non-complex isolation reports within the safe range",
          "[report_feasibility_t][estimate][constrained]")
{
  system_space_t space {};
  space.valid = make_system_ids({ 1, 2, 3, 4, 5, 6, 7, 8,
                                  9, 10, 11, 12, 13, 14, 15, 16 });

  auto request = report_request_t::from_category_hint(
      std::vector<theory_feature_category_t> {
          theory_feature_category_t::impossible,
          theory_feature_category_t::core,
          theory_feature_category_t::fundamental,
          theory_feature_category_t::not_feature,
          theory_feature_category_t::and_feature,
          theory_feature_category_t::or_feature });
  request.include_complex_details = false;
  request.include_raw_terms = false;
  request.include_simplified_terms = true;
  request.include_isolation = true;

  const auto estimate = estimate_report_feasibility(22, space, request);

  REQUIRE(estimate.valid_system_count == 16);
  REQUIRE(estimate.theory_feature_count == power2(16));
  REQUIRE(estimate.risk == feasibility_risk_t::safe);
  REQUIRE(estimate.estimated_total_runtime_ms < 300000.0L);
  REQUIRE_FALSE(estimate.approval_recommended);
}

TEST_CASE("feasibility estimator reports derived-feature-space signals beyond raw size",
          "[report_feasibility_t][estimate]")
{
  const auto space = unconstrained_system_space(4);
  const auto estimate = estimate_report_feasibility(
      4, space, report_request_t::unrestricted());

  REQUIRE(std::ranges::any_of(
      estimate.observations,
      [](const std::string& observation) {
        return observation.find("valid systems") != std::string::npos;
      }));
  REQUIRE(std::ranges::any_of(
      estimate.observations,
      [](const std::string& observation) {
        return observation.find("Complex derived features") != std::string::npos;
      }));
  REQUIRE(std::ranges::any_of(
      estimate.observations,
      [](const std::string& observation) {
        return observation.find("Isolation output") != std::string::npos;
      }));
}

TEST_CASE("feasibility estimator explains why category-focused drill-downs are attractive",
          "[report_feasibility_t][estimate]")
{
  const auto space = unconstrained_system_space(4);
  auto request = report_request_t::from_category_hint(
      std::vector<theory_feature_category_t> {
          theory_feature_category_t::fundamental,
          theory_feature_category_t::not_feature });
  request.include_isolation = false;
  const auto estimate = estimate_report_feasibility(4, space, request);

  REQUIRE(std::ranges::any_of(
      estimate.observations,
      [](const std::string& observation) {
        return observation.find("small set of feature categories")
               != std::string::npos;
      }));
}

TEST_CASE("report planner keeps safe theory-native requests unchanged",
          "[report_execution_plan_t][plan]")
{
  const auto request = report_request_t::unrestricted();
  const auto plan = plan_report_execution(2, four_system_space(), request);

  REQUIRE_FALSE(plan.adjustment_recommended);
  REQUIRE(plan.requested_feasibility.risk == feasibility_risk_t::safe);
  REQUIRE(plan.recommended_feasibility.risk == feasibility_risk_t::safe);
  REQUIRE(plan.summary().find("requested mode=full") != std::string::npos);
}

TEST_CASE("report planner recommends a lighter executable plan for risky requests",
          "[report_execution_plan_t][plan]")
{
  const auto space = unconstrained_system_space(4);
  const auto request = report_request_t::unrestricted();
  const auto plan = plan_report_execution(4, space, request);

  REQUIRE(plan.adjustment_recommended);
  REQUIRE(plan.requested_feasibility.risk != feasibility_risk_t::safe);
  REQUIRE(plan.recommended_feasibility.estimated_total_runtime_ms
          < plan.requested_feasibility.estimated_total_runtime_ms);
  REQUIRE((!plan.recommended_request.include_complex_details
           || !plan.recommended_request.include_isolation
           || plan.recommended_request.mode == report_mode_t::summary));
  REQUIRE_FALSE(plan.rationale.empty());
  REQUIRE(std::ranges::any_of(
      plan.requested_feasibility.observations,
      [](const std::string& observation) {
        return observation.find("derived feature space") != std::string::npos
               || observation.find("valid systems") != std::string::npos
               || observation.find("Complex derived features")
                      != std::string::npos;
      }));
}

TEST_CASE("report planner surfaces selected-feature suggestions for impractical requests",
          "[report_execution_plan_t][plan]")
{
  const auto space = unconstrained_system_space(5);
  const auto request = report_request_t::unrestricted();
  const auto plan = plan_report_execution(5, space, request);

  REQUIRE(plan.requested_feasibility.risk == feasibility_risk_t::impractical);
  REQUIRE(std::ranges::any_of(
      plan.alternatives,
      [](const report_plan_option_t& option) {
        return option.label == "selected feature IDs only"
               && !option.executable_now;
      }));
}
