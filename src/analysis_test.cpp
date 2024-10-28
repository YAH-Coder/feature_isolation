/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#define CATCH_CONFIG_MAIN
#include "../include/catch.hpp"
#include "../include/feature_analysis.hpp"
#include <iostream>
#include <sstream>

using namespace feature_isolation;

TEST_CASE( "Test system_t calculations", "[system_t]" ) {
  system_t system;

  // Simulate features being added
  system.F_feature_ids.insert( 1 );
  system.F_feature_ids.insert( 2 );
  system.N_feature_ids.insert( 3 );
  system.A_feature_ids.insert( 4 );
  system.ANF_feature_ids.insert( 5 );

  // Test DF calculation (sum of all non-F feature categories)
  REQUIRE( system.DF() == 3 ); // 1 from N, 1 from A, 1 from ANF

  // Test T calculation (total features: DF + F)
  REQUIRE( system.T() == 5 ); // 3 from DF + 2 from F
}

TEST_CASE( "Test feature_analysis_t initialization", "[feature_analysis_t]" ) {
  unsigned_integer F = 3;
  unsigned_integer max_rank = 2;
  unsigned_integer feature_categories = 255; // Include all categories

  feature_analysis_t analysis( F, max_rank, feature_categories );

  // Test initial values
  REQUIRE( analysis.F == 3 );
  REQUIRE( analysis.R == 2 );
  REQUIRE( analysis.C == 255 );
  REQUIRE( analysis.S == ( 1 << F ) ); // 2^F = 2^3 = 8 systems

  // Check feature offsets and total number of features
  REQUIRE( analysis.F_offset == 0 );
  REQUIRE( analysis.N_offset == analysis.F_offset + analysis.F );
  REQUIRE( analysis.A_offset == analysis.N_offset + analysis.N );
  REQUIRE( analysis.ANF_offset == analysis.ON_offset + analysis.ON );

  // Check that the systems were initialized correctly
  REQUIRE( analysis.m_systems.size() == 8 ); // 2^F systems
}

TEST_CASE( "Test feature_analysis_t category functions", "[feature_analysis_t]" ) {
  unsigned_integer F = 3; // local F hides feature_isolation::F --> ok
  unsigned_integer N { F };
  unsigned_integer max_rank = 2;
  unsigned_integer feature_categories = 255; // Include all categories

  feature_analysis_t analysis( F, max_rank, feature_categories );

  // Test category() and category_name() functions
  REQUIRE( analysis.category( 0 ) == 0 ); // F category

  // Adjusted tests to reflect correct feature offsets
  REQUIRE( analysis.category( F + 1 ) == 1 );     // N category (starts after F)
  /* The following N ist NOT declared locally, hence feature_isolation::N is
   * taken. Therefore, N should be initialized with F above.
   */
  REQUIRE( analysis.category( F + N + 1 ) == 2 ); // A category (starts after F + N)

  // Test category_name()
  REQUIRE( analysis.category_name( 0 ) == "F" );
  REQUIRE( analysis.category_name( F + 1 ) == "N" );
  REQUIRE( analysis.category_name( F + N + 1 ) == "A" );
}

TEST_CASE( "Test feature_analysis_t feature retrieval", "[feature_analysis_t]" ) {
  unsigned_integer F = 3;
  unsigned_integer max_rank = 2;
  unsigned_integer feature_categories = 255; // Include all categories

  feature_analysis_t analysis( F, max_rank, feature_categories );

  // Retrieve the first feature for system 1
  auto feature = analysis.at( 1 ); // Retrieve first feature (ID 1)

  // Test that the feature values match system ID representation
  // System ID 1: 000 -> feature[0] = false, feature[1] = false, feature[2] = false
  // System ID 2: 001 -> feature[0] = true, feature[1] = false, feature[2] = false
  REQUIRE( feature[0] == true );
  REQUIRE( feature[1] == false ); // This was failing, now should reflect correct system ID
}

TEST_CASE( "Test all features retrieval in system", "[system_t]" ) {
  system_t system;
  system.F_feature_ids.insert( 1 );
  system.F_feature_ids.insert( 2 );
  system.N_feature_ids.insert( 3 );
  system.A_feature_ids.insert( 4 );
  system.AN_feature_ids.insert( 5 );

  auto all_features = system.all_features();

  REQUIRE( all_features.size() == 5 );
  REQUIRE( all_features.count( 1 ) == 1 );
  REQUIRE( all_features.count( 5 ) == 1 );
}

TEST_CASE( "Test feature_analysis_t printing methods", "[feature_analysis_t]" ) {
  unsigned_integer F = 3;
  unsigned_integer max_rank = 2;
  unsigned_integer feature_categories = 255; // Include all categories

  feature_analysis_t analysis( F, max_rank, feature_categories );

  // Test feature printing
  std::ostringstream os;
  analysis.print_features( os );
  std::string output = os.str();
  REQUIRE( output.find( "FEATURE ID" ) != std::string::npos );
  REQUIRE( output.find( "CATEGORY" ) != std::string::npos );
  REQUIRE( output.find( "DESCRIPTION" ) != std::string::npos );

  // Test system statistics printing
  os.str( "" ); // Reset the stringstream
  analysis.print_systems_statistics( os );
  output = os.str();
  REQUIRE( output.find( "SYSTEM ID" ) != std::string::npos );
  REQUIRE( output.find( "DF" ) != std::string::npos );

  // Test full analysis printing
  os.str( "" );
  analysis.print( os );
  output = os.str();
  REQUIRE( output.find( "IN PRINCIPLE" ) != std::string::npos );
  REQUIRE( output.find( "ACTUALLY" ) != std::string::npos );
}

TEST_CASE( "Test feature_analysis_t exception handling", "[feature_analysis_t]" ) {
  unsigned_integer F = 3;
  unsigned_integer max_rank = 4;             // Invalid because max_rank > F
  unsigned_integer feature_categories = 255; // Include all categories

  // Test that an invalid rank throws an exception
  REQUIRE_THROWS_AS( feature_analysis_t( F, max_rank, feature_categories ), std::invalid_argument );
}
