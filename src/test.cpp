// Test for push from iPad - to be undone lazter.
#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "../include/catch.hpp"
#include "../include/fi_mathhelper.hpp"

using namespace feature_isolation;

// Template function to test addition operations
template <typename Func>
void testAdditionFunc(Func func, const std::string& testName, bool expectThrow) {
   SECTION(testName + " - Normal operation without overflow") {
      REQUIRE(func(1, 2) == 3);
      REQUIRE(func(std::numeric_limits<unsignedInteger>::max() - 1, 1) == std::numeric_limits<unsignedInteger>::max());
   }
   SECTION(testName + " - Edge cases for overflow") {
      if (expectThrow) {
         REQUIRE_THROWS_AS(func(std::numeric_limits<unsignedInteger>::max(), 1), std::overflow_error);
      } else {
         REQUIRE_NOTHROW(func(std::numeric_limits<unsignedInteger>::max(), 1));
      }
   }
   SECTION(testName + " - Test with zero values") {
      REQUIRE(func(0, 0) == 0);
      REQUIRE(func(0, 1) == 1);
      REQUIRE(func(std::numeric_limits<unsignedInteger>::max(), 0) == std::numeric_limits<unsignedInteger>::max());
   }
}

// Template function to test subtraction operations
template <typename Func>
void testSubtractionFunc(Func func, const std::string& testName, bool expectUnderflow) {
   SECTION(testName + " - Normal operation without underflow") {
      REQUIRE(func(3, 1) == 2);
      REQUIRE(func(std::numeric_limits<unsignedInteger>::max(), 1) == std::numeric_limits<unsignedInteger>::max() - 1);
   }
   SECTION(testName + " - Edge cases for underflow") {
      if (expectUnderflow) {
         REQUIRE_THROWS_AS(func(1, std::numeric_limits<unsignedInteger>::max()), std::underflow_error);
      } else {
         REQUIRE_NOTHROW(func(1, std::numeric_limits<unsignedInteger>::max()));
      }
   }
   SECTION(testName + " - Test with zero values") {
      REQUIRE(func(0, 0) == 0);
      if (expectUnderflow) {
         REQUIRE_THROWS_AS(func(0, 1), std::underflow_error);
      } else {
         REQUIRE_NOTHROW(func(0, 1));
      }
   }
}

// Template function to test multiplication operations
template <typename Func>
void testMultiplicationFunc(Func func, const std::string& testName, bool expectThrow) {
   SECTION(testName + " - Normal operation without overflow") {
      REQUIRE(func(2, 3) == 6);
      REQUIRE(func(1, std::numeric_limits<unsignedInteger>::max()) == std::numeric_limits<unsignedInteger>::max());
   }
   SECTION(testName + " - Edge cases for overflow") {
      if (expectThrow) {
         // For checked functions, expect an overflow_error when multiplying values that would exceed max
         REQUIRE_THROWS_AS(func(std::numeric_limits<unsignedInteger>::max() / 2 + 1, 3), std::overflow_error);
      } else {
         // For unchecked functions, no overflow detection so no exception expected
         REQUIRE_NOTHROW(func(std::numeric_limits<unsignedInteger>::max() / 2 + 1, 3));
      }
   }
   SECTION(testName + " - Test with zero and one") {
      REQUIRE(func(0, 10) == 0);
      REQUIRE(func(10, 0) == 0);
      REQUIRE(func(1, std::numeric_limits<unsignedInteger>::max()) == std::numeric_limits<unsignedInteger>::max());
      REQUIRE(func(std::numeric_limits<unsignedInteger>::max(), 1) == std::numeric_limits<unsignedInteger>::max());
   }
}

// Test cases for addition functions
TEST_CASE("Arithmetic function tests for addition", "[arithmetic][add]") {
   // Checked functions expect throws on overflow
   testAdditionFunc(checked_arithmetic::add, "Checked add", true);
   testAdditionFunc([](unsignedInteger a, unsignedInteger b) { return checked_arithmetic::add_asign(a, b); }, "Checked add_asign", true);

   // Unchecked functions do not expect throws on overflow
   testAdditionFunc(unchecked_arithmetic::add, "Unchecked add", false);
   testAdditionFunc([](unsignedInteger a, unsignedInteger b) { return unchecked_arithmetic::add_asign(a, b); }, "Unchecked add_asign", false);
}

// Test cases for subtraction functions
TEST_CASE("Arithmetic function tests for subtraction", "[arithmetic][subtract]") {
   // Checked functions expect underflow detection
   testSubtractionFunc(checked_arithmetic::subtract, "Checked subtract", true);
   testSubtractionFunc([](unsignedInteger a, unsignedInteger b) { return checked_arithmetic::subtract_asign(a, b); }, "Checked subtract_asign", true);

   // Unchecked functions do not perform underflow detection
   testSubtractionFunc(unchecked_arithmetic::subtract, "Unchecked subtract", false);
   testSubtractionFunc([](unsignedInteger a, unsignedInteger b) { return unchecked_arithmetic::subtract_asign(a, b); }, "Unchecked subtract_asign", false);
}

TEST_CASE("Power function tests", "[arithmetic][power]") {
   SECTION("Normal operation") {
      REQUIRE(power(2, 3) == 8); // 2^3 = 8
      REQUIRE(power(3, 2) == 9); // 3^2 = 9
   }

   SECTION("Power of 0") {
      REQUIRE(power(2, 0) == 1); // Any number to the power of 0 is 1
      REQUIRE(power(0, 5) == 0); // 0 to any positive power is 0
   }

   SECTION("Power of 1") {
      REQUIRE(power(1, 0) == 1); // 1^0 = 1
   }

   SECTION("Large exponent") {
      REQUIRE(power(2, 10) == 1024); // 2^10 = 1024
   }
}

// Test cases for the power2 function
TEST_CASE("Power2 function tests", "[arithmetic][power2]") {
   SECTION("Normal operation") {
      REQUIRE(power2(3) == 8);     // 2^3 = 8
      REQUIRE(power2(10) == 1024); // 2^10 = 1024
   }

   SECTION("Power2 of 0") {
      REQUIRE(power2(0) == 1); // 2^0 = 1
   }
}

// Test cases for the product function
TEST_CASE("Product function tests", "[arithmetic][product]") {
   SECTION("Normal operation") {
      REQUIRE(product(1, 4) == 24);  // 1*2*3*4 = 24
      REQUIRE(product(2, 5) == 120); // 2*3*4*5 = 120
   }

   SECTION("Product with from > to") {
      REQUIRE(product(5, 2) == 0); // Should return 0, as the range is invalid
   }

   SECTION("Product including zero") {
      REQUIRE(product(0, 2) == 0); // Includes zero, should return 0
   }

   SECTION("Product of single number") {
      REQUIRE(product(3, 3) == 3); // The product of a single number is itself
   }
}

// Test cases for the factorial function
TEST_CASE("Factorial function tests", "[arithmetic][factorial]") {
   SECTION("Normal operation") {
      REQUIRE(factorial(3) == 6);   // 3! = 3*2*1 = 6
      REQUIRE(factorial(5) == 120); // 5! = 5*4*3*2*1 = 120
   }

   SECTION("Factorial of 0") {
      REQUIRE(factorial(0) == 1); // 0! = 1 by definition
   }

   SECTION("Factorial of 1") {
      REQUIRE(factorial(1) == 1); // 1! = 1 by definition
   }
}

// Test cases for the combinations function
TEST_CASE("Combinations function tests", "[arithmetic][combinations]") {
   SECTION("Normal operation") {
      REQUIRE(combinations(5, 2) == 10); // 5C2 = 10
      REQUIRE(combinations(5, 3) == 10); // 5C3 = 10, symmetry check
   }

   SECTION("Combinations with k = 0") {
      REQUIRE(combinations(5, 0) == 1); // nC0 = 1
   }

   SECTION("Combinations with k = n") {
      REQUIRE(combinations(5, 5) == 1); // nCn = 1
   }

   SECTION("Combinations with n = 0") {
      REQUIRE(combinations(0, 0) == 1); // 0C0 = 1, edge case
   }

   SECTION("Combinations with k > n") {
      REQUIRE(combinations(5, 6) == 0); // nCk = 0 if k > n
   }
}

// Test cases for the sum_of_combinations function
TEST_CASE("Sum of combinations function tests", "[arithmetic][sum_of_combinations]") {
   SECTION("Normal operation") {
      REQUIRE(sum_of_combinations(3, 1) == 4);  // 3C0 + 3C1 = 1 + 3 = 4
      REQUIRE(sum_of_combinations(4, 2) == 11); // 4C2 + 4C3 + 4C4 = 6 + 4 + 1 = 11
   }

   SECTION("Sum of combinations with k = 0") {
      REQUIRE(sum_of_combinations(3, 0) == 1); // 3C0 = 1
   }

   SECTION("Sum of combinations with n = 0") {
      REQUIRE(sum_of_combinations(0, 0) == 1); // Edge case, only 0C0 = 1
   }

   SECTION("Sum of combinations with k > n") {
      REQUIRE(sum_of_combinations(3, 4) == 8);
   }
}
