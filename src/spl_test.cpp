/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#define CATCH_CONFIG_MAIN
#include "../include/catch.hpp"
#include "../include/spl.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using namespace feature_isolation;

bool compare_files( const std::string& file1, const std::string& file2 ) {
  std::ifstream f1( file1, std::ifstream::binary | std::ifstream::ate );
  std::ifstream f2( file2, std::ifstream::binary | std::ifstream::ate );

  if ( f1.fail() || f2.fail() ) {
    return false;
  }

  if ( f1.tellg() != f2.tellg() ) {
    return false;
  }

  f1.seekg( 0, std::ifstream::beg );
  f2.seekg( 0, std::ifstream::beg );

  return std::equal( std::istreambuf_iterator<char>( f1.rdbuf() ),
                     std::istreambuf_iterator<char>(),
                     std::istreambuf_iterator<char>( f2.rdbuf() ) );
}

TEST_CASE( "Process valid SPL specification", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/functional_spec.csv"; // Input file for valid SPL specification
  std::string output1_filename = "testing_csv/output1.csv";
  std::string output2_filename = "testing_csv/output2.csv";

  // First run: Process input file to create output1
  spl_processor_t processor1( input_filename );
  REQUIRE_NOTHROW( processor1.get_specification() ); // Ensure processing succeeds
  processor1.print( output1_filename );

  // Second run: Process output1 to create output2
  spl_processor_t processor2( output1_filename );
  REQUIRE_NOTHROW( processor2.get_specification() ); // Ensure processing succeeds
  processor2.print( output2_filename );

  // Compare output1 and output2
  REQUIRE( compare_files( output1_filename, output2_filename ) == true );

  // Delete the output files if they are the same
  std::remove( output1_filename.c_str() );
  std::remove( output2_filename.c_str() );
}

TEST_CASE( "Process SPL with REQUIRES_ALL constraint violation", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/requires_all_test.csv";

  // Expect an exception due to the violation of the REQUIRES_ALL constraint
  REQUIRE_THROWS_WITH(
      spl_processor_t( input_filename ).get_specification(),
      Catch::Matchers::Contains( "violates REQUIRES_ALL constraint for feature" ) );
}

TEST_CASE( "Process SPL specification with categories", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/categories_test.csv";
  spl_processor_t processor( input_filename );
  const spl_specification_t& spec = processor.get_specification();

  // Verify that categories are correctly parsed
  REQUIRE( ( spec.m_categories & feature_isolation::F ) != 0 );
  REQUIRE( ( spec.m_categories & feature_isolation::A ) != 0 );
  REQUIRE( ( spec.m_categories & feature_isolation::O ) != 0 );
  REQUIRE( ( spec.m_categories & feature_isolation::N ) == 0 );
}

TEST_CASE( "Process SPL specification with available systems", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/available_systems_test.csv";
  spl_processor_t processor( input_filename );
  const spl_specification_t& spec = processor.get_specification();

  // Verify that only specified systems are available
  REQUIRE( spec.m_available_systems == spl_specification_t::systems_t{ 1, 2, 3 } );
}

TEST_CASE( "Process SPL specification with missing systems", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/missing_systems_test.csv";
  spl_processor_t processor( input_filename );
  const spl_specification_t& spec = processor.get_specification();

  // Verify that the missing systems are excluded from the full list
  spl_specification_t::systems_t expected_systems;
  for ( unsigned_integer i = 1; i <= spec.m_max_available_systems; ++i ) {
    if ( i != 4 && i != 5 && i != 6 ) {
      expected_systems.push_back( i );
    }
  }
  REQUIRE( spec.m_available_systems == expected_systems );
}

TEST_CASE( "Process SPL specification with MAX_RANK", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/max_rank_test.csv";
  spl_processor_t processor( input_filename );
  const spl_specification_t& spec = processor.get_specification();

  // Verify that MAX_RANK is set correctly
  REQUIRE( spec.m_max_rank == 2 );
}

TEST_CASE( "Process SPL specification with syntax error", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/syntax_error_test.csv";

  // Expect an exception due to the syntax error in the FEATURES section
  REQUIRE_THROWS_WITH(
      spl_processor_t( input_filename ).process(),
      Catch::Matchers::Contains( "Failed to parse SPL specification: expected a numeric feature count, but got" ) );
}

TEST_CASE( "Process valid SPL specification with detailed checks", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/functional_spec.csv"; // Input file for valid SPL specification
  spl_processor_t processor( input_filename );
  const spl_specification_t& spec = processor.get_specification();

  // Verify SPL name
  REQUIRE( spec.m_spl_name == "Factorial" ); // SPL name is "Factorial"

  // Verify the number of features
  REQUIRE( spec.m_feature_names.size() == 4 ); // There are 4 features

  // Verify feature names
  REQUIRE( spec.m_feature_names[0] == "Checked" );
  REQUIRE( spec.m_feature_names[1] == "Logging" );
  REQUIRE( spec.m_feature_names[2] == "Iterative" );
  REQUIRE( spec.m_feature_names[3] == "Recursive" );

  // Verify dependencies
  // m_requires_one should have size 1, containing ROOT (-1) with dependencies on Iterative and Recursive
  REQUIRE( spec.m_requires_one.size() == 1 );
  REQUIRE( spec.m_requires_one.count( -1 ) == 1 ); // ROOT is represented by -1

  // Check the dependencies for ROOT
  auto root_deps = spec.m_requires_one.at( -1 );
  REQUIRE( root_deps.size() == 2 );

  // Find the indices for Iterative and Recursive
  // From feature names, "Iterative" is at index 2, "Recursive" is at index 3
  REQUIRE( root_deps.count( 2 ) == 1 ); // Iterative
  REQUIRE( root_deps.count( 3 ) == 1 ); // Recursive

  // Verify that other dependencies are empty
  REQUIRE( spec.m_requires_all.empty() );
  REQUIRE( spec.m_requires_any.empty() );
  REQUIRE( spec.m_excludes_all.empty() );

  // Verify available systems
  spl_specification_t::systems_t expected_available_systems = { 5, 6, 7, 8, 9, 10, 11, 12 };
  REQUIRE( spec.m_available_systems == expected_available_systems );

  // Verify categories
  // Since categories are not specified in the functional_spec.csv, check default value
  REQUIRE( spec.m_categories == feature_isolation::F ); // Default category is F

  // Verify other attributes
  REQUIRE( spec.m_spl_specified == true );
  REQUIRE( spec.m_feature_names_specified == true );
  REQUIRE( spec.m_categories_specified == false ); // Categories not specified
  REQUIRE( spec.m_explicit_dependencies_specified == true );
  REQUIRE( spec.m_available_systems_specified == true );
  REQUIRE( spec.m_missing_systems_specified == false );
  REQUIRE( spec.m_max_rank_specified == false );

  // Verify MAX_RANK
  // MAX_RANK is not specified, so it should default to the number of features (F)
  REQUIRE( spec.m_max_rank == 4 ); // Number of features is 4
}


TEST_CASE( "Process SPL with REQUIRES_ANY constraint", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/requires_any_test.csv";

  spl_processor_t processor( input_filename );
  const spl_specification_t& spec = processor.get_specification();

  REQUIRE( spec.m_requires_any.size() == 1 );
}

TEST_CASE( "Process SPL with EXCLUDES_ALL constraint violation", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/excludes_all_test.csv";

  REQUIRE_THROWS_WITH(
      spl_processor_t( input_filename ).get_specification(),
      Catch::Matchers::Contains( "violates EXCLUDES_ALL constraint for feature" ) );
}

TEST_CASE( "Process SPL with REQUIRES_ONE constraint violation", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/requires_one_test.csv";

  REQUIRE_THROWS_WITH(
      spl_processor_t( input_filename ).get_specification(),
      Catch::Matchers::Contains( "violates REQUIRES_ONE constraint for feature" ) );
}

TEST_CASE( "Process SPL with missing EXPLICIT_DEPENDENCIES section", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/no_explicit_dependencies.csv";
  spl_processor_t processor( input_filename );
  const spl_specification_t& spec = processor.get_specification();

  REQUIRE( spec.m_requires_all.empty() );
  REQUIRE( spec.m_requires_any.empty() );
  REQUIRE( spec.m_requires_one.empty() );
  REQUIRE( spec.m_excludes_all.empty() );
}

TEST_CASE( "Process SPL with duplicate categories", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/duplicate_categories_test.csv";

  REQUIRE_THROWS_WITH(
      spl_processor_t( input_filename ).get_specification(),
      Catch::Matchers::Contains( "Duplicate category found: " ) );
}

TEST_CASE( "Process SPL with invalid feature name", "[spl_processor_t]" ) {
  std::string input_filename = "testing_csv/invalid_feature_name.csv";

  REQUIRE_THROWS_WITH(
      spl_processor_t( input_filename ).get_specification(),
      Catch::Matchers::Contains( "Feature NotFoundFeature is not a specified feature name." ) );
}
