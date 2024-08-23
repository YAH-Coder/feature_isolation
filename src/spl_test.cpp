#define CATCH_CONFIG_MAIN
#include "../include/catch.hpp"
#include "../include/spl_z3.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using namespace feature_isolation;

bool compare_files( const std::string& file1, const std::string& file2 ) {
  std::ifstream f1( file1, std::ifstream::binary | std::ifstream::ate );
  std::ifstream f2( file2, std::ifstream::binary | std::ifstream::ate );

  if ( f1.fail() || f2.fail() ) {
    std::cerr << "Error opening files for comparison." << std::endl;
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
  std::string input_filename = "../testing_csv/functional_spec.csv"; // Input file for valid SPL specification
  std::string output1_filename = "../testing_csv/output1.csv";
  std::string output2_filename = "../testing_csv/output2.csv";

  // First run: Process input file to create output1
  spl_processor_t processor1( input_filename );
  REQUIRE( processor1.process() == true ); // Ensure processing succeeds
  processor1.print( output1_filename );

  // Second run: Process output1 to create output2
  spl_processor_t processor2( output1_filename );
  REQUIRE( processor2.process() == true ); // Ensure processing succeeds
  processor2.print( output2_filename );

  // Compare output1 and output2
  REQUIRE( compare_files( output1_filename, output2_filename ) == true );

  // Delete the output files if they are the same
  std::remove( output1_filename.c_str() );
  std::remove( output2_filename.c_str() );
}

TEST_CASE( "Process invalid SPL specification and check error message", "[spl_processor_t]" ) {
  std::string wrong_filename = "../testing_csv/wrong_spl_specification.csv"; // Input file for invalid SPL specification

  spl_processor_t processor( wrong_filename );

  // Capture the standard error output
  std::ostringstream error_output;
  std::streambuf* old_cerr = std::cerr.rdbuf( error_output.rdbuf() );

  // Run the processor
  REQUIRE( processor.process() == false ); // Expecting the process to fail

  // Restore the original cerr buffer
  std::cerr.rdbuf( old_cerr );

  // Check that the correct error message was output to stderr
  std::string expected_error = "Unexpected : Software at line 1, column 14\nFailed to parse SPL specification in file: wrong_spl_specification.csv\n";
  REQUIRE( error_output.str() == expected_error );
}

TEST_CASE( "Process SPL specification with missing feature amount and check error message", "[spl_processor_t]" ) {
  std::string missing_feature_amount_filename = "../testing_csv/no_feature_amount_specified.csv"; // Input file with missing feature amount

  spl_processor_t processor( missing_feature_amount_filename );

  // Capture the standard error output
  std::ostringstream error_output;
  std::streambuf* old_cerr = std::cerr.rdbuf( error_output.rdbuf() );

  // Run the processor
  REQUIRE( processor.process() == false ); // Expecting the process to fail

  // Restore the original cerr buffer
  std::cerr.rdbuf( old_cerr );

  // Check that the correct error message was output to stderr
  std::string expected_error = "Error occurred when reading number of independent features in file \nUnexpected : Logging at line 8, column 0\nFailed to parse SPL specification in file: no_feature_amount_specified.csv\n";
  REQUIRE( error_output.str() == expected_error );
}

TEST_CASE( "Process invalid system configuration and check error message", "[spl_processor_t]" ) {
  std::string invalid_system_config_filename = "../testing_csv/invalid_system_config.csv"; // Input file with invalid system configuration

  spl_processor_t processor( invalid_system_config_filename );

  // Capture the standard error output
  std::ostringstream error_output;
  std::streambuf* old_cerr = std::cerr.rdbuf( error_output.rdbuf() );

  // Run the processor
  REQUIRE( processor.process() == true );

  // Restore the original cerr buffer
  std::cerr.rdbuf( old_cerr );

  // Check that the correct error message was output to stderr
  std::string expected_error = "System 4 violates REQUIRES_ONE constraint for feature ROOT and dependent feature Iterative\nFailed to validate systems , because invalid system configuration in file: invalid_system_config.csv\n";
  REQUIRE( error_output.str() == expected_error );
}
