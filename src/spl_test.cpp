#define CATCH_CONFIG_MAIN
#include "../include/catch.hpp"
#include "../include/spl.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using namespace feature_isolation;

bool compare_files(const std::string& file1, const std::string& file2) {
    std::ifstream f1(file1, std::ifstream::binary | std::ifstream::ate);
    std::ifstream f2(file2, std::ifstream::binary | std::ifstream::ate);

    if (f1.fail() || f2.fail()) {
        std::cerr << "Error opening files for comparison." << std::endl;
        return false;
    }

    if (f1.tellg() != f2.tellg()) {
        return false;
    }

    f1.seekg(0, std::ifstream::beg);
    f2.seekg(0, std::ifstream::beg);

    return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
                      std::istreambuf_iterator<char>(),
                      std::istreambuf_iterator<char>(f2.rdbuf()));
}

TEST_CASE("Process valid SPL specification", "[spl_processor_t]") {
    std::string input_filename = "testing_csv/functional_spec.csv"; // Input file for valid SPL specification
    std::string output1_filename = "testing_csv/output1.csv";
    std::string output2_filename = "testing_csv/output2.csv";

    // First run: Process input file to create output1
    spl_processor_t processor1(input_filename);
    REQUIRE_NOTHROW(processor1.get_specification()); // Ensure processing succeeds
    processor1.print(output1_filename);

    // Second run: Process output1 to create output2
    spl_processor_t processor2(output1_filename);
    REQUIRE_NOTHROW(processor2.get_specification()); // Ensure processing succeeds
    processor2.print(output2_filename);

    // Compare output1 and output2
    REQUIRE(compare_files(output1_filename, output2_filename) == true);

    // Delete the output files if they are the same
    std::remove(output1_filename.c_str());
    std::remove(output2_filename.c_str());
}

TEST_CASE("Process invalid SPL specification and check error message", "[spl_processor_t]") {
    std::string wrong_filename = "testing_csv/wrong_spl_specification.csv"; // Input file for invalid SPL specification

    // Capture the standard error output
    std::ostringstream error_output;
    std::streambuf* old_cerr = std::cerr.rdbuf(error_output.rdbuf());

    // Try to process the invalid file
    REQUIRE_THROWS_AS(spl_processor_t(wrong_filename), std::logic_error);

    // Restore the original cerr buffer
    std::cerr.rdbuf(old_cerr);

    // Check that the correct error message was output to stderr
    std::string expected_error = "Failed to parse SPL specification in file: " + wrong_filename + "\n";
    REQUIRE(error_output.str().find(expected_error) != std::string::npos);
}

TEST_CASE("Process SPL specification with missing feature amount and check error message", "[spl_processor_t]") {
    std::string missing_feature_amount_filename = "testing_csv/no_feature_amount_specified.csv"; // Input file with missing feature amount

    // Capture the standard error output
    std::ostringstream error_output;
    std::streambuf* old_cerr = std::cerr.rdbuf(error_output.rdbuf());

    try {
        // Try to process the invalid file and expect it to fail
        spl_processor_t processor(missing_feature_amount_filename);
        processor.process(); // This should throw
        FAIL("Expected std::logic_error to be thrown");
    } catch (const std::logic_error& e) {
        // Restore the original cerr buffer
        std::cerr.rdbuf(old_cerr);

        // Check the exception message
        std::string actual_exception_message = e.what();
        std::string expected_exception_message = "Failed to parse SPL specification";
        REQUIRE(actual_exception_message.find(expected_exception_message) != std::string::npos);

        // Compare the captured error output
        std::string actual_error_output = error_output.str();
        std::string expected_error_output =
            "Error occurred when reading number of independent features in file \n"
            "Unexpected : Logging at line 8, column 0\n"
            "Failed to parse SPL specification in file: " +
            missing_feature_amount_filename + "\n";

        // Check that the correct error message was output to stderr
        REQUIRE(actual_error_output.find(expected_error_output) != std::string::npos);
    }
}

TEST_CASE("Process invalid system configuration and check error message", "[spl_processor_t]") {
    std::string invalid_system_config_filename = "testing_csv/invalid_system_config.csv"; // Input file with invalid system configuration

    // Capture the standard error output
    std::ostringstream error_output;
    std::streambuf* old_cerr = std::cerr.rdbuf(error_output.rdbuf());

    // Try to process the invalid file
    REQUIRE_NOTHROW(spl_processor_t(invalid_system_config_filename).get_specification());

    // Restore the original cerr buffer
    std::cerr.rdbuf(old_cerr);

    // Check that the correct error message was output to stderr
    std::string expected_error = "Failed to validate systems , because invalid system configuration in file: " + invalid_system_config_filename + "\n";
    REQUIRE(error_output.str().find(expected_error) != std::string::npos);
}
