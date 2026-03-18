/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#define CATCH_CONFIG_MAIN
#include "../include/catch.hpp"
#include "../include/spl.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace feature_isolation;

static system_ids_t make_system_ids(std::initializer_list<unsigned_integer> values)
{
  system_ids_t ids {};
  ids.insert(values.begin(), values.end());
  return ids;
}

bool compare_files(const std::string& file1, const std::string& file2)
{
  std::ifstream f1(file1, std::ifstream::binary | std::ifstream::ate);
  std::ifstream f2(file2, std::ifstream::binary | std::ifstream::ate);
  if (f1.fail() || f2.fail())
  {
    return false;
  }
  if (f1.tellg() != f2.tellg())
  {
    return false;
  }
  f1.seekg(0, std::ifstream::beg);
  f2.seekg(0, std::ifstream::beg);
  return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
                    std::istreambuf_iterator<char>(),
                    std::istreambuf_iterator<char>(f2.rdbuf()));
}

TEST_CASE("Process valid new DSL specification and preserve roundtrip", "[spl_processor_t][new_dsl]")
{
  const std::string input_filename = "spl_specs/new_dsl/valid/constraints_boolean_and_count.spl";
  const std::string output1_filename = "testing/generated/output1.spl";
  const std::string output2_filename = "testing/generated/output2.spl";

  spl_processor_t processor1(input_filename);
  REQUIRE_NOTHROW(processor1.get_specification());
  processor1.print(output1_filename);

  spl_processor_t processor2(output1_filename);
  REQUIRE_NOTHROW(processor2.get_specification());
  processor2.print(output2_filename);

  REQUIRE(compare_files(output1_filename, output2_filename));

  std::remove(output1_filename.c_str());
  std::remove(output2_filename.c_str());
}

TEST_CASE("Process unconstrained new DSL specification", "[spl_processor_t][new_dsl]")
{
  spl_processor_t processor("spl_specs/new_dsl/valid/unconstrained_2f.spl");
  const spl_specification_t& spec = processor.get_specification();

  REQUIRE(spec.m_spl_name == "TwoFeatureUnconstrained");
  REQUIRE(spec.m_F == 2);
  REQUIRE(spec.m_feature_names == feature_names_t{ "fx", "fy" });
  REQUIRE(spec.m_constraints.empty());
  REQUIRE(spec.m_constraints_specified == false);
  REQUIRE(valid_system_ids(spec) == make_system_ids({1, 2, 3, 4}));
  REQUIRE(available_valid_system_ids(spec) == make_system_ids({1, 2, 3, 4}));
  REQUIRE(missing_valid_system_ids(spec).empty());
}

TEST_CASE("Process Boolean and cardinality constraints in the new DSL", "[spl_processor_t][new_dsl]")
{
  spl_processor_t processor("spl_specs/new_dsl/valid/constraints_boolean_and_count.spl");
  const spl_specification_t& spec = processor.get_specification();

  REQUIRE(spec.m_constraints_specified);
  REQUIRE(spec.m_constraints.size() == 5);
  REQUIRE(to_string(spec.m_constraints.at(0)) == "(fx -> fy)");
  REQUIRE(to_string(spec.m_constraints.at(1)) == "(!(fz && fw))");
  REQUIRE(to_string(spec.m_constraints.at(2)) == "((fx || fz) -> fw)");
  REQUIRE(to_string(spec.m_constraints.at(3)) == "COUNT(fx, fy, fz) <= 2");
  REQUIRE(to_string(spec.m_constraints.at(4)) == "COUNT(fy, fz, fw) IN [1,2]");
  REQUIRE(valid_system_ids(spec) == make_system_ids({3, 9, 11, 12}));
}

TEST_CASE("Parsed constraints carry compiled feature indices for immutable evaluation",
          "[spl_processor_t][new_dsl][compiled]")
{
  spl_processor_t processor("spl_specs/new_dsl/valid/constraints_boolean_and_count.spl");
  const spl_specification_t& spec = processor.get_specification();

  const auto* implication = std::get_if<theory_formula_t::implication_t>(
      &spec.m_constraints.at(0).payload());
  REQUIRE(implication != nullptr);
  REQUIRE(std::get<theory_formula_t::feature_ref_t>(implication->lhs->payload())
              .compiled_feature_index
              .value()
          == 0);
  REQUIRE(std::get<theory_formula_t::feature_ref_t>(implication->rhs->payload())
              .compiled_feature_index
              .value()
          == 1);

  const auto* count_compare = std::get_if<theory_count_compare_t>(
      &spec.m_constraints.at(3).payload());
  REQUIRE(count_compare != nullptr);
  REQUIRE(count_compare->compiled_feature_indices
          == std::vector<unsigned_integer> { 0, 1, 2 });

  const auto* count_interval = std::get_if<theory_count_interval_t>(
      &spec.m_constraints.at(4).payload());
  REQUIRE(count_interval != nullptr);
  REQUIRE(count_interval->compiled_feature_indices
          == std::vector<unsigned_integer> { 1, 2, 3 });
}

TEST_CASE("Optional sections remain optional in the new DSL", "[spl_processor_t][new_dsl]")
{
  std::istringstream is("SPL Minimal\nFEATURES 2 a b\nCONSTRAINTS\nEND\n");
  parser_t parser(is, "<memory>");
  const auto& spec = parser.m_spec;

  REQUIRE(spec.m_spl_name == "Minimal");
  REQUIRE(spec.m_constraints_specified);
  REQUIRE(spec.m_constraints.empty());
  REQUIRE(spec.m_available_systems_specified == false);
  REQUIRE(spec.m_missing_systems_specified == false);
  REQUIRE(spec.m_available_systems.empty());
  REQUIRE(spec.m_missing_systems.empty());
  REQUIRE(spec.system_space().valid == make_system_ids({1, 2, 3, 4}));
  REQUIRE(spec.system_space().available == make_system_ids({1, 2, 3, 4}));
  REQUIRE(spec.system_space().missing.empty());
}

TEST_CASE("System space is computed once and exposed immutably on the specification",
          "[spl_processor_t][new_dsl][systems]")
{
  std::istringstream is(
      "SPL Partitioned\n"
      "FEATURES 2 fx fy\n"
      "CONSTRAINTS\n"
      "  ASSERT fx -> fy\n"
      "END\n"
      "AVAILABLE_SYSTEMS 3 4 END\n"
      "MISSING_SYSTEMS 1 END\n");
  parser_t parser(is, "<memory>");
  const auto& spec = parser.m_spec;

  REQUIRE(spec.system_space().valid == make_system_ids({1, 3, 4}));
  REQUIRE(spec.system_space().available == make_system_ids({3, 4}));
  REQUIRE(spec.system_space().missing == make_system_ids({1}));
  REQUIRE(valid_system_ids(spec) == spec.system_space().valid);
  REQUIRE(available_valid_system_ids(spec) == spec.system_space().available);
  REQUIRE(missing_valid_system_ids(spec) == spec.system_space().missing);
  REQUIRE(theory_system_space(spec).valid == spec.system_space().valid);
  REQUIRE(theory_system_space(spec).available == spec.system_space().available);
  REQUIRE(theory_system_space(spec).missing == spec.system_space().missing);
}

TEST_CASE("Explicit system sections remain parsed-only while derived spaces are computed separately",
          "[spl_processor_t][new_dsl][systems]")
{
  spl_processor_t processor("spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl");
  const auto& spec = processor.get_specification();

  REQUIRE(spec.m_available_systems_specified == false);
  REQUIRE(spec.m_available_systems.empty());
  REQUIRE(spec.m_missing_systems_specified);
  REQUIRE(spec.m_missing_systems == spl_specification_t::systems_t{1});
  REQUIRE(valid_system_ids(spec) == make_system_ids({1, 3, 4}));
  REQUIRE(available_valid_system_ids(spec) == make_system_ids({3, 4}));
  REQUIRE(missing_valid_system_ids(spec) == make_system_ids({1}));
}

TEST_CASE("CATEGORY_FILTER stores theory-native categories only", "[spl_processor_t][new_dsl]")
{
  spl_processor_t processor("spl_specs/new_dsl/valid/constraints_boolean_and_count.spl");
  const auto& spec = processor.get_specification();

  REQUIRE(spec.m_category_filter_specified);
  REQUIRE(spec.m_category_filter.has_value());
  REQUIRE(spec.m_category_filter->size() == 6);
  REQUIRE(spec.m_category_filter->front() == theory_feature_category_t::core);
  REQUIRE(spec.m_category_filter->back() == theory_feature_category_t::complex);
}

TEST_CASE("New DSL tokenizer recognizes theory operators and keywords", "[tokenizer_t][new_dsl]")
{
  std::istringstream input(
      "CONSTRAINTS ASSERT !(fx && fy) ASSERT COUNT(fx, fy) <= 1 ASSERT fx <-> TRUE END");
  tokenizer_t tokenizer(input);
  std::vector<std::string> tokens;
  while (true)
  {
    auto token = tokenizer.next();
    if (token.type() == token_type_t::EMPTY)
    {
      break;
    }
    tokens.push_back(token.value());
  }

  REQUIRE(std::find(tokens.begin(), tokens.end(), "CONSTRAINTS") != tokens.end());
  REQUIRE(std::find(tokens.begin(), tokens.end(), "ASSERT") != tokens.end());
  REQUIRE(std::find(tokens.begin(), tokens.end(), "COUNT") != tokens.end());
  REQUIRE(std::find(tokens.begin(), tokens.end(), "&&") != tokens.end());
  REQUIRE(std::find(tokens.begin(), tokens.end(), "<=") != tokens.end());
  REQUIRE(std::find(tokens.begin(), tokens.end(), "<->") != tokens.end());
}

TEST_CASE("Legacy DSL keywords are rejected", "[spl_processor_t][new_dsl][legacy_removed]")
{
  SECTION("MAX_RANK")
  {
    std::istringstream is("SPL Bad\nFEATURES 2 a b\nMAX_RANK 1\n");
    REQUIRE_THROWS(parser_t(is, "<memory>"));
  }
  SECTION("legacy CATEGORIES")
  {
    std::istringstream is("SPL Bad\nFEATURES 2 a b\nCATEGORIES\nF\nEND\n");
    REQUIRE_THROWS(parser_t(is, "<memory>"));
  }
  SECTION("EXPLICIT_DEPENDENCIES")
  {
    std::istringstream is("SPL Bad\nFEATURES 2 a b\nEXPLICIT_DEPENDENCIES\na REQUIRES_ALL b END\nEND\n");
    REQUIRE_THROWS(parser_t(is, "<memory>"));
  }
}

TEST_CASE("COUNT relations produce the expected valid systems", "[spl_processor_t][new_dsl][count]")
{
  const std::vector<std::pair<std::string, system_ids_t>> cases = {
      { "spl_specs/new_dsl/valid/count_eq_1.spl", make_system_ids({2, 3, 5}) },
      { "spl_specs/new_dsl/valid/count_ne_1.spl", make_system_ids({1, 4, 6, 7, 8}) },
      { "spl_specs/new_dsl/valid/count_lt_2.spl", make_system_ids({1, 2, 3, 5}) },
      { "spl_specs/new_dsl/valid/count_le_2.spl", make_system_ids({1, 2, 3, 4, 5, 6, 7}) },
      { "spl_specs/new_dsl/valid/count_gt_1.spl", make_system_ids({4, 6, 7, 8}) },
      { "spl_specs/new_dsl/valid/count_ge_2.spl", make_system_ids({4, 6, 7, 8}) },
      { "spl_specs/new_dsl/valid/count_in_1_2.spl", make_system_ids({2, 3, 4, 5, 6, 7}) },
  };

  for (const auto& [filename, expected] : cases)
  {
    CAPTURE(filename);
    spl_processor_t processor(filename);
    REQUIRE(valid_system_ids(processor.get_specification()) == expected);
  }
}

TEST_CASE("Availability and missing-system validation errors are surfaced", "[spl_processor_t][new_dsl][validation]")
{
  REQUIRE_THROWS_WITH(
      spl_processor_t("spl_specs/new_dsl/invalid/invalid_available_systems.spl").get_specification(),
      Catch::Matchers::Contains("System 2 listed in AVAILABLE_SYSTEMS is not valid under the declared constraints."));
  REQUIRE_THROWS_WITH(
      spl_processor_t("spl_specs/new_dsl/invalid/invalid_missing_systems.spl").get_specification(),
      Catch::Matchers::Contains("System 2 listed in MISSING_SYSTEMS is not valid under the declared constraints."));
  REQUIRE_THROWS_WITH(
      spl_processor_t("spl_specs/new_dsl/invalid/overlapping_available_missing.spl").get_specification(),
      Catch::Matchers::Contains("System 3 cannot be listed in both AVAILABLE_SYSTEMS and MISSING_SYSTEMS."));
  REQUIRE_THROWS_WITH(
      spl_processor_t("spl_specs/new_dsl/invalid/incomplete_partition_available_missing.spl").get_specification(),
      Catch::Matchers::Contains("AVAILABLE_SYSTEMS and MISSING_SYSTEMS must form a disjoint partition of the valid system space."));
}

TEST_CASE("Malformed COUNT syntax is rejected", "[spl_processor_t][new_dsl][validation]")
{
  REQUIRE_THROWS_WITH(
      spl_processor_t("spl_specs/new_dsl/invalid/invalid_count_interval.spl").get_specification(),
      Catch::Matchers::Contains("Invalid COUNT interval"));
  REQUIRE_THROWS_WITH(
      spl_processor_t("spl_specs/new_dsl/invalid/duplicate_count_feature.spl").get_specification(),
      Catch::Matchers::Contains("Duplicate COUNT feature found"));
}

TEST_CASE("documentation-facing paper examples stay aligned with the canonical fixtures",
          "[spl_processor_t][theory][examples]")
{
  REQUIRE(compare_files("external_documentation/examples_new_dsl/paper_unconstrained_2f.spl",
                        "spl_specs/new_dsl/valid/unconstrained_2f.spl"));
  REQUIRE(compare_files("external_documentation/examples_new_dsl/paper_fx_implies_fy.spl",
                        "spl_specs/new_dsl/valid/fx_implies_fy.spl"));
  REQUIRE(compare_files("external_documentation/examples_new_dsl/paper_fx_implies_fy_missing_s0.spl",
                        "spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl"));
  REQUIRE(compare_files("external_documentation/examples_new_dsl/paper_factorial.spl",
                        "spl_specs/new_dsl/valid/factorial_paper.spl"));
}

TEST_CASE("Theory paper fixtures are expressed with the new DSL", "[spl_processor_t][theory][fixtures]")
{
  SECTION("unconstrained two-feature example")
  {
    spl_processor_t processor("spl_specs/new_dsl/valid/unconstrained_2f.spl");
    const auto& spec = processor.get_specification();
    REQUIRE(spec.m_F == 2);
    REQUIRE(valid_system_ids(spec) == make_system_ids({1, 2, 3, 4}));
  }

  SECTION("constrained two-feature example")
  {
    spl_processor_t processor("spl_specs/new_dsl/valid/fx_implies_fy.spl");
    REQUIRE(valid_system_ids(processor.get_specification()) == make_system_ids({1, 3, 4}));
  }

  SECTION("missing-system paper example")
  {
    spl_processor_t processor("spl_specs/new_dsl/valid/fx_implies_fy_missing_s0.spl");
    const auto& spec = processor.get_specification();
    REQUIRE(valid_system_ids(spec) == make_system_ids({1, 3, 4}));
    REQUIRE(available_valid_system_ids(spec) == make_system_ids({3, 4}));
    REQUIRE(missing_valid_system_ids(spec) == make_system_ids({1}));
  }

  SECTION("factorial paper example")
  {
    spl_processor_t processor("spl_specs/new_dsl/valid/factorial_paper.spl");
    REQUIRE(valid_system_ids(processor.get_specification())
            == make_system_ids({2, 3, 6, 7, 10, 11, 14, 15}));
  }
}
