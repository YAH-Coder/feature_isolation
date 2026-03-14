/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "../include/catch.hpp"
#include "../include/fi_mathhelper.hpp"

using namespace feature_isolation;

// Benchmark cases for addition functions
TEST_CASE("Benchmark addition functions", "[arithmetic][benchmark][add]")
{
  unsigned_integer a = 1'2345'6789, b = 9'8765'4321;

  BENCHMARK("Checked add")
  {
    return checked_arithmetic::add(a, b);
  };

  BENCHMARK("Checked add_assign")
  {
    unsigned_integer result = a; // Assign 'a' to 'result' to mimic the in-place
                                 // modification behavior
    return checked_arithmetic::add_assign(result, b);
  };

  BENCHMARK("Unchecked add")
  {
    return unchecked_arithmetic::add(a, b);
  };

  BENCHMARK("Unchecked add_assign")
  {
    unsigned_integer result = a; // Assign 'a' to 'result' to mimic the in-place
                                 // modification behavior
    return unchecked_arithmetic::add_assign(result, b);
  };
}

// Benchmark cases for subtraction functions
TEST_CASE("Benchmark subtraction functions",
          "[arithmetic][benchmark][subtract]")
{
  unsigned_integer a = 9'8765'4321, b = 1'2345'6789;

  BENCHMARK("Checked subtract")
  {
    return checked_arithmetic::subtract(a, b);
  };

  BENCHMARK("Checked subtract_assign")
  {
    unsigned_integer result = a; // Assign 'a' to 'result' to mimic the in-place
                                 // modification behavior
    return checked_arithmetic::subtract_assign(result, b);
  };

  BENCHMARK("Unchecked subtract")
  {
    return unchecked_arithmetic::subtract(a, b);
  };

  BENCHMARK("Unchecked subtract_assign")
  {
    unsigned_integer result = a; // Assign 'a' to 'result' to mimic the in-place
                                 // modification behavior
    return unchecked_arithmetic::subtract_assign(result, b);
  };
}

TEST_CASE("Benchmark multiplication functions",
          "[arithmetic][benchmark][multiply]")
{
  unsigned_integer a = 12'3456, b = 65'4321;

  BENCHMARK("Checked multiply")
  {
    return checked_arithmetic::multiply(a, b);
  };

  BENCHMARK("Checked multiply_assign")
  {
    unsigned_integer result = a; // Assign 'a' to 'result' to mimic the in-place
                                 // modification behavior
    return checked_arithmetic::multiply_assign(result, b);
  };

  BENCHMARK("Unchecked multiply")
  {
    return unchecked_arithmetic::multiply(a, b);
  };

  BENCHMARK("Unchecked multiply_assign")
  {
    unsigned_integer result = a; // Assign 'a' to 'result' to mimic the in-place
                                 // modification behavior
    return unchecked_arithmetic::multiply_assign(result, b);
  };
}

TEST_CASE("Power function performance", "[power]")
{
  // Benchmarking power function
  BENCHMARK("power small base, small exponent")
  {
    return power(2, 10); // 2^10
  };

  BENCHMARK("power large base, small exponent")
  {
    return power(12'3456, 2); // 123456^2
  };

  BENCHMARK("power small base, large exponent")
  {
    return power(2, 20); // 2^20
  };
}

TEST_CASE("Power2 function performance", "[power2]")
{
  // Benchmarking power2 function
  BENCHMARK("power2 small exponent")
  {
    return power2(10); // 2^10
  };

  BENCHMARK("power2 large exponent")
  {
    return power2(20); // 2^20
  };
}
