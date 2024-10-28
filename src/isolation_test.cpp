/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#define CATCH_CONFIG_MAIN
#include "../include/catch.hpp"
#include "../include/isolation.hpp"
#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <iostream>

using namespace feature_isolation;

TEST_CASE("Container conversion functions") {

    SECTION("to_vector conversion") {
        std::set<int> s = {1, 2, 3};
        auto v = to_vector(s);
        
        REQUIRE(v.size() == 3);
        REQUIRE(v[0] == 1);
        REQUIRE(v[1] == 2);
        REQUIRE(v[2] == 3);
    }

    SECTION("to_set conversion") {
        std::vector<int> v = {1, 2, 3};
        auto s = to_set(v);

        REQUIRE(s.size() == 3);
        REQUIRE(s.contains(1));
        REQUIRE(s.contains(2));
        REQUIRE(s.contains(3));
    }
}

TEST_CASE("Subset and proper_subset functions") {

    SECTION("Subset function") {
        std::set<int> s1 = {1, 2, 3};
        std::set<int> s2 = {1, 2};

        REQUIRE(subset(s1, s2) == true);  // s2 is a subset of s1
        REQUIRE(subset(s2, s1) == false); // s1 is not a subset of s2
    }

    SECTION("Proper subset function") {
        std::set<int> s1 = {1, 2, 3};
        std::set<int> s2 = {1, 2};

        REQUIRE(proper_subset(s1, s2) == true);  // s2 is a proper subset of s1
        REQUIRE(proper_subset(s1, s1) == false); // s1 is not a proper subset of itself
    }
}

TEST_CASE("Difference_t comparison and operators") {
    SECTION("Difference_t comparison operators") {
        difference_t d1;
        d1.I.insert(1); d1.I.insert(2);
        d1.U.insert(3); d1.U.insert(4);

        difference_t d2;
        d2.I.insert(1); d2.I.insert(2);
        d2.U.insert(3); d2.U.insert(4);

        difference_t d3;
        d3.I.insert(1); d3.I.insert(2);
        d3.U.insert(4); d3.U.insert(5);

        REQUIRE(d1 == d2);
        REQUIRE((d1 <=> d3) == std::strong_ordering::less);
    }

    SECTION("Proper difference subset functions") {
        differences_t diffs;
        system_ids_t i, u;
        i.insert(1); i.insert(2);
        u.insert(3); u.insert(4);

        feature_isolation_t::memorize_proper_difference_subset(diffs, i, u);

        REQUIRE(diffs.size() > 0);  // Ensure proper subsets are added

        difference_t diff;
        diff.I.insert(1); diff.I.insert(2);
        diff.U.insert(3); diff.U.insert(4);

        bool result = feature_isolation_t::proper_difference_subset(i, u, diff);
        REQUIRE(result == false);
    }
}

// Test partition function with size printing
TEST_CASE("Partition function with size output") {
    feature_isolation_t fi("spl_specs/factorial.csv");

    // Testing partition for feature_id = 1 (Logging)
    SECTION("Partition of feature 1 (Logging)") {
        auto result = fi.partition(1);
        REQUIRE(result.I.size() == 4);
        REQUIRE(result.U.size() == 4); 
    }
}

TEST_CASE("Flattening functions") {
    feature_isolation_t fi("spl_specs/factorial.csv");

    SECTION("Flatten by intersection") {
        system_ids_t systems;
        systems.insert(1);
        systems.insert(2);

        auto result = fi.flatten_by_intersection(systems);
        REQUIRE(result.size() == 12);
    }

    SECTION("Flatten by union") {
        system_ids_t systems;
        systems.insert(1);
        systems.insert(2);

        auto result = fi.flatten_by_union(systems);
        REQUIRE(result.size() == 22); 
    }

    SECTION("Flattened difference") {
        system_ids_t i, u;
        i.insert(1); i.insert(2);
        u.insert(3); u.insert(4);

        auto result = fi.flattened_difference(i, u);
        REQUIRE(result.size() == 2); 
    }
}

TEST_CASE("Difference calculation functions") {
    feature_isolation_t fi("spl_specs/factorial.csv");

    SECTION("Minimal differences") {
        unsigned_integer feature_id = 1; // Logging
        auto result = fi.minimal_differences(feature_id);
        
        REQUIRE(result.features.size() == 1);     // Features should be identified
        REQUIRE(result.differences.size() == 14);  // Differences should be recorded
    }

    SECTION("Some differences") {
        unsigned_integer feature_id = 1; // Logging
        auto result = fi.some_difference(feature_id);

        REQUIRE(result.features.size() == 1);     // Features should be identified
        REQUIRE(result.differences.size() == 1);  // Differences should be recorded
    }
}

// Test vector logical operators (&, |, ~)
TEST_CASE("Logical vector operators") {

    SECTION("Logical AND operator") {
        std::vector<bool> v1 = {true, false, true};
        std::vector<bool> v2 = {true, true, false};

        auto result = v1 & v2;
        REQUIRE(result == std::vector<bool>{true, false, false});
    }

    SECTION("Logical OR operator") {
        std::vector<bool> v1 = {true, false, true};
        std::vector<bool> v2 = {true, true, false};

        auto result = v1 | v2;
        REQUIRE(result == std::vector<bool>{true, true, true});
    }

    SECTION("Logical NOT operator") {
        std::vector<bool> v = {true, false, true};

        auto result = ~v;
        REQUIRE(result == std::vector<bool>{false, true, false});
    }
}

TEST_CASE("Name generation and print functions") {
    feature_isolation_t fi("spl_specs/factorial.csv");

    SECTION("Name generation") {
        unsigned_integer feature_id = 1;
        auto name = fi.name(feature_id);

        REQUIRE(name == "Logging"); 
    }

    SECTION("Print functions") {
        std::stringstream ss;
        fi.print(ss, true, true);  // Capture output to a stringstream

        REQUIRE(ss.str().size() > 0);  // Ensure output is generated
    }
}

TEST_CASE("Output comparison with sample_output.csv") {
    feature_isolation_t fi("spl_specs/factorial.csv");

    SECTION("Compare output with sample_output.csv") {
        std::stringstream ss;

        ss << "=== SPL SPECIFICATION ===" << std::endl;
        printer_t printer { ss };
        printer.print_specification(fi.m_spl);
        ss << "\n=== SYSTEMS ===" << std::endl;
        fi.m_features.print(ss);
        ss << "=== FEATURES BY CATEGORY ===" << std::endl;
        fi.m_features.print_features(ss);
        ss << "=== FEATURES BY INDEX ===" << std::endl;
        for (auto index { one }; index <= fi.m_features.T; ++index)
        {
            ss << index << separator() << fi.m_features.category_name(index)
               << separator() << to_string(fi.m_features.at(index)) << std::endl;
        }
        ss << "=== ISOLATION RESULTS ===" << std::endl;
        fi.print(ss, true, true);  // -min-diffs=true, -feature-names=true

        std::ifstream sample_file("testing_csv/sample_output.csv");
        REQUIRE(sample_file.is_open());

        std::stringstream sample_ss;
        sample_ss << sample_file.rdbuf();

        REQUIRE(ss.str() == sample_ss.str());

        sample_file.close();
    }
}