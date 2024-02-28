#define CATCH_CONFIG_MAIN

#include <cstdint>

#include "catch.hpp"
#include "feature_isolation.hpp"

namespace checked = feature_isolation::checked_arithmetic;
namespace unchecked = feature_isolation::unchecked_arithmetic;

using cInteger = std::uintmax_t;

TEST_CASE("Checked arithmetic addition tests", "[add]") {
  SECTION("Normal operation") {
    REQUIRE(checked::add(1UL, 2UL) == 3UL);
    REQUIRE(checked::add(0UL, 0UL) == 0UL);
  }

  SECTION("Addition right below overflow") {
    REQUIRE(checked::add(std::numeric_limits<cInteger>::max() - 1UL, 1UL) ==
            std::numeric_limits<cInteger>::max());
  }

  SECTION("Overflow detection") {
    REQUIRE_THROWS_AS(checked::add(std::numeric_limits<cInteger>::max(), 1UL),
                      std::overflow_error);
  }
}

TEST_CASE("Checked arithmetic subtraction tests", "[subtract]") {
  SECTION("Normal operation") {
    REQUIRE(checked::subtract(5UL, 3UL) == 2UL);
    REQUIRE(checked::subtract(0UL, 0UL) == 0UL);
  }

  SECTION("Subtraction resulting in zero") {
    REQUIRE(checked::subtract(10UL, 10UL) == 0UL);
  }

  SECTION("Subtraction without underflow") {
    REQUIRE(checked::subtract(std::numeric_limits<cInteger>::max(), 1UL) ==
            std::numeric_limits<cInteger>::max() - 1UL);
  }

  SECTION("Subtraction leading to underflow detection") {
    REQUIRE_THROWS_AS(checked::subtract(0UL, 1UL), std::underflow_error);
  }
}

TEST_CASE("Checked arithmetic multiplication tests", "[multiply]") {
  SECTION("Normal operation") {
    REQUIRE(checked::multiply(2UL, 3UL) == 6UL);
    REQUIRE(checked::multiply(0UL, 123UL) == 0UL);
    REQUIRE(checked::multiply(std::numeric_limits<cInteger>::max(), 0UL) ==
            0UL);
  }

  SECTION("Overflow detection") {
    REQUIRE_THROWS_AS(
        checked::multiply(std::numeric_limits<cInteger>::max() / 2UL + 1UL,
                          2UL),
        std::overflow_error);
    REQUIRE_THROWS_AS(
        checked::multiply(std::numeric_limits<cInteger>::max(), 2UL),
        std::overflow_error);
  }

  SECTION("Multiplication by one") {
    REQUIRE(checked::multiply(1UL, std::numeric_limits<cInteger>::max()) ==
            std::numeric_limits<cInteger>::max());
    REQUIRE(checked::multiply(std::numeric_limits<cInteger>::max(), 1UL) ==
            std::numeric_limits<cInteger>::max());
  }

  SECTION("Multiplication resulting in maximum value") {
    cInteger half_max = std::numeric_limits<cInteger>::max() / 2UL;
    REQUIRE(checked::multiply(half_max, 2UL) ==
            std::numeric_limits<cInteger>::max() - 1UL);
  }
}

TEST_CASE("Unchecked arithmetic addition tests", "[add_unchecked]") {
  SECTION("Normal operation") {
    REQUIRE(unchecked::add(1UL, 2UL) == 3UL);
    REQUIRE(unchecked::add(0UL, 0UL) == 0UL);
  }
}

TEST_CASE("Unchecked arithmetic subtraction tests", "[subtract_unchecked]") {
  SECTION("Normal operation") {
    REQUIRE(unchecked::subtract(5UL, 3UL) == 2UL);
    REQUIRE(unchecked::subtract(0UL, 0UL) == 0UL);
  }
}

TEST_CASE("Unchecked arithmetic multiplication tests", "[multiply_unchecked]") {
  SECTION("Normal operation") {
    REQUIRE(unchecked::multiply(2UL, 3UL) == 6UL);
    REQUIRE(unchecked::multiply(0UL, 123UL) == 0UL);
  }
}

TEST_CASE("power tests", "[utility_functions]") {
  SECTION("Base cases") {
    REQUIRE(feature_isolation::power(2UL, 3UL) == 8UL);
    REQUIRE(feature_isolation::power(2UL, 0UL) == 1UL);
    REQUIRE(feature_isolation::power(0UL, 5UL) == 0UL);
  }
}

TEST_CASE("power2 tests", "[utility_functions]") {
  SECTION("Normal operations") {
    REQUIRE(feature_isolation::power2(10UL) == 1024UL);
    REQUIRE(feature_isolation::power2(0UL) == 1UL);
  }

  SECTION("Large exponent") {
    REQUIRE(feature_isolation::power2(30UL) ==
            1ULL << 30UL); // Assuming cInteger can handle the result without
                           // overflow
  }
}

TEST_CASE("product tests", "[utility_functions]") {
  SECTION("Normal operation") {
    REQUIRE(feature_isolation::product(1UL, 4UL) == 24UL);
    REQUIRE(feature_isolation::product(3UL, 5UL) == 60UL);
  }

  SECTION("Edge cases") {
    REQUIRE(feature_isolation::product(0UL, 0UL) == 0UL);
    REQUIRE(feature_isolation::product(1UL, 1UL) == 1UL);
  }

  SECTION("Range including zero") {
    REQUIRE(feature_isolation::product(0UL, 3UL) == 0UL);
  }
}

TEST_CASE("factorial tests", "[utility_functions]") {
  SECTION("Normal operations") {
    REQUIRE(feature_isolation::factorial(5UL) == 120UL);
    REQUIRE(feature_isolation::factorial(0UL) == 1UL);
  }

  SECTION("Large values") {
    REQUIRE(
        feature_isolation::factorial(10UL) ==
        3628800UL); // Assuming cInteger can handle the result without overflow
  }
}

TEST_CASE("combinations tests", "[utility_functions]") {
  SECTION("Valid combinations") {
    REQUIRE(feature_isolation::combinations(5UL, 3UL) == 10UL);
    REQUIRE(feature_isolation::combinations(5UL, 5UL) == 1UL);
    REQUIRE(feature_isolation::combinations(5UL, 0UL) == 1UL);
  }

  SECTION("Symmetry property") {
    REQUIRE(feature_isolation::combinations(10UL, 3UL) ==
            feature_isolation::combinations(10UL, 7UL));
  }

  SECTION("Additive principle") {
    REQUIRE(feature_isolation::combinations(10UL, 5UL) ==
            feature_isolation::combinations(9UL, 4UL) +
                feature_isolation::combinations(9UL, 5UL));
  }
}

TEST_CASE("sum of combinations tests", "[utility_functions]") {
  SECTION("Normal operations") {
    REQUIRE(feature_isolation::sum_of_combinations(3UL, 2UL) ==
            feature_isolation::combinations(3UL, 2UL) +
                feature_isolation::combinations(3UL, 3UL));
    REQUIRE(feature_isolation::sum_of_combinations(4UL, 2UL) ==
            feature_isolation::combinations(4UL, 2UL) +
                feature_isolation::combinations(4UL, 3UL) +
                feature_isolation::combinations(4UL, 4UL));
  }

  SECTION("k = 0") {
    REQUIRE(feature_isolation::sum_of_combinations(3UL, 0UL) == 8UL);
  }

  SECTION("Complete sum property") {
    REQUIRE(feature_isolation::sum_of_combinations(3UL, 0UL) == 8UL);
    REQUIRE(feature_isolation::sum_of_combinations(4UL, 0UL) == 16UL);
  }
}
