/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#define CATCH_CONFIG_MAIN
#include "../include/catch.hpp"
#include "../include/isolation.hpp"
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

using namespace feature_isolation;

static system_ids_t make_system_ids(std::initializer_list<unsigned_integer> values)
{
  system_ids_t ids {};
  ids.insert(values.begin(), values.end());
  return ids;
}

TEST_CASE("Container conversion functions")
{
  SECTION("to_vector conversion")
  {
    std::set<int> s = { 1, 2, 3 };
    auto          v = to_vector(s);

    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 2);
    REQUIRE(v[2] == 3);
  }

  SECTION("to_set conversion")
  {
    std::vector<int> v = { 1, 2, 3 };
    auto             s = to_set(v);

    REQUIRE(s.size() == 3);
    REQUIRE(s.contains(1));
    REQUIRE(s.contains(2));
    REQUIRE(s.contains(3));
  }
}

TEST_CASE("Subset and proper_subset functions")
{
  SECTION("Subset function")
  {
    std::set<int> s1 = { 1, 2, 3 };
    std::set<int> s2 = { 1, 2 };

    REQUIRE(subset(s1, s2) == true);  // s2 is a subset of s1
    REQUIRE(subset(s2, s1) == false); // s1 is not a subset of s2
  }

  SECTION("Proper subset function")
  {
    std::set<int> s1 = { 1, 2, 3 };
    std::set<int> s2 = { 1, 2 };

    REQUIRE(proper_subset(s1, s2) == true); // s2 is a proper subset of s1
    REQUIRE(proper_subset(s1, s1)
            == false); // s1 is not a proper subset of itself
  }
}

TEST_CASE("Difference_t comparison and operators")
{
  SECTION("Difference_t comparison operators")
  {
    difference_t d1;
    d1.I.insert(1);
    d1.I.insert(2);
    d1.U.insert(3);
    d1.U.insert(4);

    difference_t d2;
    d2.I.insert(1);
    d2.I.insert(2);
    d2.U.insert(3);
    d2.U.insert(4);

    difference_t d3;
    d3.I.insert(1);
    d3.I.insert(2);
    d3.U.insert(4);
    d3.U.insert(5);

    REQUIRE(d1 == d2);
    REQUIRE((d1 <=> d3) == std::strong_ordering::less);
  }

  SECTION("Proper difference subset functions")
  {
    differences_t diffs;
    system_ids_t  i, u;
    i.insert(1);
    i.insert(2);
    u.insert(3);
    u.insert(4);

    feature_isolation_t::memorize_proper_difference_subset(diffs, i, u);

    REQUIRE(diffs.size() > 0); // Ensure proper subsets are added

    difference_t diff;
    diff.I.insert(1);
    diff.I.insert(2);
    diff.U.insert(3);
    diff.U.insert(4);

    bool result = feature_isolation_t::proper_difference_subset(i, u, diff);
    REQUIRE(result == false);
  }
}

// Test partition function with size printing
TEST_CASE("Partition function with size output")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/factorial_three_features.spl");

  // Testing partition for feature_id = 1 (Logging)
  SECTION("Partition of feature 1 (Logging)")
  {
    auto result = fi.partition(1);
    REQUIRE(result.I.size() >= 0);
    REQUIRE(result.U.size() > 0);
  }
}

TEST_CASE("Flattening functions")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/factorial_three_features.spl");

  SECTION("Flatten by intersection")
  {
    system_ids_t systems;
    systems.insert(1);
    systems.insert(2);

    auto result = fi.flatten_by_intersection(systems);
    REQUIRE(result.size() > 0);
  }

  SECTION("Flatten by union")
  {
    system_ids_t systems;
    systems.insert(1);
    systems.insert(2);

    auto result = fi.flatten_by_union(systems);
    REQUIRE(result.size() > 0);
  }

  SECTION("Flattened difference")
  {
    system_ids_t i, u;
    i.insert(1);
    i.insert(2);
    u.insert(3);
    u.insert(4);

    auto result = fi.flattened_difference(i, u);
    REQUIRE(result.size() > 0);
  }
}

TEST_CASE("Difference calculation functions")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/factorial_three_features.spl");

  SECTION("Minimal differences")
  {
    unsigned_integer feature_id = 1; // Logging
    auto             result     = fi.minimal_differences(feature_id);

    REQUIRE(result.features.size() >= 0);
    REQUIRE(result.differences.size() >= 0);
  }

  SECTION("Some differences")
  {
    unsigned_integer feature_id = 1; // Logging
    auto             result     = fi.some_difference(feature_id);

    REQUIRE(result.features.size() >= 0);
    REQUIRE(result.differences.size() >= 0);
  }
}

// Test vector logical operators (&, |, ~)
TEST_CASE("Logical vector operators")
{
  SECTION("Logical AND operator")
  {
    std::vector<bool> v1 = { true, false, true };
    std::vector<bool> v2 = { true, true, false };

    auto result = v1 & v2;
    REQUIRE(result == std::vector<bool> { true, false, false });
  }

  SECTION("Logical OR operator")
  {
    std::vector<bool> v1 = { true, false, true };
    std::vector<bool> v2 = { true, true, false };

    auto result = v1 | v2;
    REQUIRE(result == std::vector<bool> { true, true, true });
  }

  SECTION("Logical NOT operator")
  {
    std::vector<bool> v = { true, false, true };

    auto result = ~v;
    REQUIRE(result == std::vector<bool> { false, true, false });
  }
}

TEST_CASE("Theory-native naming and print functions")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/unconstrained_2f.spl");

  SECTION("Name generation")
  {
    REQUIRE(fi.name(1) == "⊥");
    REQUIRE(fi.name(fi.m_features.feature_count()) == "⊤");
  }

  SECTION("Print functions")
  {
    std::stringstream ss;
    fi.print(ss, true, true);

    REQUIRE(ss.str().find("CANDIDATE FEATURES") != std::string::npos);
    REQUIRE(ss.str().find("ℓ_T(t_i)") != std::string::npos);
    REQUIRE(ss.str().find("RESULT TYPE") != std::string::npos);
  }
}

TEST_CASE("Theory-native report exposes new categories, DSL coverage, and ambiguity columns")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/unconstrained_2f.spl");

  std::stringstream ss;

  ss << "=== SPL SPECIFICATION ===" << std::endl;
  printer_t printer { ss };
  printer.print_specification(fi.m_spl);
  ss << "\n=== THEORY DSL COVERAGE ===" << std::endl;
  printer.print_theory_dsl_coverage(fi.m_spl);
  ss << "\n=== SYSTEMS ===" << std::endl;
  fi.m_features.print(ss);
  ss << "=== FEATURES BY CATEGORY ===" << std::endl;
  fi.m_features.print_features(ss);
  ss << "=== FEATURES BY INDEX ===" << std::endl;
  ss << "i" << separator() << "FEATURE" << separator() << "CATEGORY"
     << separator() << "t_i" << separator() << "β(t_i)" << separator()
     << "ℓ_T(t_i)(simplified)" << separator() << "ℓ_T(t_i)(raw term)"
     << std::endl;
  for (const auto& record : fi.m_features.feature_registry())
  {
    ss << paper_feature_index(record.id) << separator()
       << paper_feature_label(record.id) << separator()
       << feature_isolation::to_string(record.category) << separator()
       << to_string(record.systems_present) << separator()
       << to_string(record.systems_present) << separator()
       << record.simplified_term << separator() << record.raw_term << std::endl;
  }
  ss << "=== ISOLATION RESULTS ===" << std::endl;
  fi.print(ss, true, true);

  const auto report = ss.str();
  REQUIRE(report.find("Impossible") != std::string::npos);
  REQUIRE(report.find("Core") != std::string::npos);
  REQUIRE(report.find("Fundamental") != std::string::npos);
  REQUIRE(report.find("Complex") != std::string::npos);
  REQUIRE(report.find("RESULT TYPE") != std::string::npos);
  REQUIRE(report.find("β(t_i)") != std::string::npos);
  REQUIRE(report.find("THEORY DSL COVERAGE") != std::string::npos);
  REQUIRE(report.find("SUPPORTED") != std::string::npos);
  REQUIRE(report.find("NOT DIRECTLY COVERED") != std::string::npos);
}

TEST_CASE("Theory-mode summary is fully theory-native",
          "[feature_analysis_t][theory][summary]")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/unconstrained_2f.spl");

  std::stringstream ss;
  fi.m_features.print(ss);

  const auto summary = ss.str();
  REQUIRE(summary.find("|S_valid|") != std::string::npos);
  REQUIRE(summary.find("|S_available|") != std::string::npos);
  REQUIRE(summary.find("LEGACY_MAX_RANK") == std::string::npos);
  REQUIRE(summary.find("LEGACY_CATEGORIES") == std::string::npos);
}



TEST_CASE("Theory target: unconstrained two-feature paper example",
          "[.][theory][fixtures]")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/unconstrained_2f.spl");

  // Paper target: |S| = 4 and |T| = 16 for the unconstrained two-feature case.
  REQUIRE(fi.m_features.system_space().available.size() == 4);
  REQUIRE(fi.m_features.valid_system_count() == 4);
  REQUIRE(fi.m_features.feature_count() == 16);
}

TEST_CASE("Theory target: constrained two-feature paper example",
          "[.][theory][fixtures]")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/fx_implies_fy.spl");

  // Paper target: the valid system space is reduced to 3 systems and the
  // constrained feature space has size 8.
  REQUIRE(fi.m_features.system_space().available.size() == 3);
  REQUIRE(fi.m_features.valid_system_count() == 3);
  REQUIRE(fi.m_features.feature_count() == 8);
}

TEST_CASE("Theory target: factorial paper example",
          "[.][theory][fixtures]")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/factorial_paper.spl");

  // Paper target: 8 valid systems and therefore 256 derived features.
  REQUIRE(fi.m_features.system_space().available.size() == 8);
  REQUIRE(fi.m_features.valid_system_count() == 8);
  REQUIRE(fi.m_features.feature_count() == 256);
}

TEST_CASE("Theory target: one missing valid system destroys uniqueness",
          "[.][theory][fixtures]")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl");

  // Fixture sanity for the paper's missing-system scenario where s0 is absent.
  const system_ids_t expected_available_systems = make_system_ids({ 3, 4 });

  REQUIRE(fi.m_features.system_space().available == expected_available_systems);
}


TEST_CASE("Theory target: missing valid systems yield ambiguous candidate sets",
          "[.][theory][fixtures][isolation]")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl");

  const auto result = fi.minimal_differences(1);

  REQUIRE(result.features.size() == 2);
  REQUIRE(result.differences.empty() == false);
}


TEST_CASE("Theory target: missing-system paper fixture retains the four paper ambiguity classes",
          "[.][theory][fixtures][isolation][paper]")
{
  feature_isolation_t fi("spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl");

  std::set<std::set<unsigned_integer>> ambiguous_feature_sets {};

  for (unsigned_integer feature_id = 1; feature_id <= fi.m_features.feature_count(); ++feature_id)
  {
    const auto result = fi.minimal_differences(feature_id);
    if (result.features.size() == 2)
    {
      ambiguous_feature_sets.insert(std::set<unsigned_integer>(result.features.begin(),
                                                               result.features.end()));
    }
  }

  REQUIRE(ambiguous_feature_sets.size() == 4);
}

TEST_CASE("Dynamic witness search supports feature ids above 1024",
          "[isolation][dynamic]")
{
  system_feature_map_t system_features {};

  feature_ids_t system_1_features {};
  system_1_features.insert(2048);
  system_1_features.insert(4096);
  system_features.emplace(1, system_1_features);

  feature_ids_t system_2_features {};
  system_2_features.insert(4096);
  system_features.emplace(2, system_2_features);

  system_ids_t I {};
  I.insert(1);

  system_ids_t U {};
  U.insert(2);

  feature_ids_t target_features {};
  target_features.insert(2048);

  const auto differences = feature_isolation_t::search_minimal_differences(
      target_features, I, U, system_features);

  REQUIRE(differences.size() == 1);
  REQUIRE(differences.begin()->I == I);
  REQUIRE(differences.begin()->U == U);
}
