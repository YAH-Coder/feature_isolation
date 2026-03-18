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

  [[nodiscard]] system_space_t four_system_space()
  {
    system_space_t space {};
    space.valid.insert(1);
    space.valid.insert(2);
    space.valid.insert(3);
    space.valid.insert(4);
    space.available = space.valid;
    return space;
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
  REQUIRE(analysis.m_systems.size() == 4);
  REQUIRE(analysis.m_systems.at(2).all_features().size() == 4);
  REQUIRE(analysis.m_systems.at(3).all_features().size() == 4);
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

TEST_CASE("theory-native constructor derives the candidate system space from the base feature count",
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

