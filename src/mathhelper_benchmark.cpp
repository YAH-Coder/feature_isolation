#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "../include/catch.hpp"
#include "../include/fi_mathhelper.hpp"

using namespace feature_isolation;

// Benchmark cases for addition functions
TEST_CASE( "Benchmark addition functions", "[arithmetic][benchmark][add]" ) {
  unsignedInteger a = 123456789, b = 987654321;

  BENCHMARK( "Checked add" ) {
    return checked_arithmetic::add( a, b );
  };

  BENCHMARK( "Checked add_asign" ) {
    unsignedInteger result = a; // Assign 'a' to 'result' to mimic the in-place modification behavior
    return checked_arithmetic::add_asign( result, b );
  };

  BENCHMARK( "Unchecked add" ) {
    return unchecked_arithmetic::add( a, b );
  };

  BENCHMARK( "Unchecked add_asign" ) {
    unsignedInteger result = a; // Assign 'a' to 'result' to mimic the in-place modification behavior
    return unchecked_arithmetic::add_asign( result, b );
  };
}

// Benchmark cases for subtraction functions
TEST_CASE( "Benchmark subtraction functions", "[arithmetic][benchmark][subtract]" ) {
  unsignedInteger a = 987654321, b = 123456789;

  BENCHMARK( "Checked subtract" ) {
    return checked_arithmetic::subtract( a, b );
  };

  BENCHMARK( "Checked subtract_asign" ) {
    unsignedInteger result = a; // Assign 'a' to 'result' to mimic the in-place modification behavior
    return checked_arithmetic::subtract_asign( result, b );
  };

  BENCHMARK( "Unchecked subtract" ) {
    return unchecked_arithmetic::subtract( a, b );
  };

  BENCHMARK( "Unchecked subtract_asign" ) {
    unsignedInteger result = a; // Assign 'a' to 'result' to mimic the in-place modification behavior
    return unchecked_arithmetic::subtract_asign( result, b );
  };
}

TEST_CASE( "Benchmark multiplication functions", "[arithmetic][benchmark][multiply]" ) {
  unsignedInteger a = 123456, b = 654321;

  BENCHMARK( "Checked multiply" ) {
    return checked_arithmetic::multiply( a, b );
  };

  BENCHMARK( "Checked multiply_asign" ) {
    unsignedInteger result = a; // Assign 'a' to 'result' to mimic the in-place modification behavior
    return checked_arithmetic::multiply_asign( result, b );
  };

  BENCHMARK( "Unchecked multiply" ) {
    return unchecked_arithmetic::multiply( a, b );
  };

  BENCHMARK( "Unchecked multiply_asign" ) {
    unsignedInteger result = a; // Assign 'a' to 'result' to mimic the in-place modification behavior
    return unchecked_arithmetic::multiply_asign( result, b );
  };
}

TEST_CASE( "Power function performance", "[power]" ) {
  // Benchmarking power function
  BENCHMARK( "power small base, small exponent" ) {
    return power( 2, 10 ); // 2^10
  };

  BENCHMARK( "power large base, small exponent" ) {
    return power( 123456, 2 ); // 123456^2
  };

  BENCHMARK( "power small base, large exponent" ) {
    return power( 2, 20 ); // 2^20
  };
}

TEST_CASE( "Power2 function performance", "[power2]" ) {
  // Benchmarking power2 function
  BENCHMARK( "power2 small exponent" ) {
    return power2( 10 ); // 2^10
  };

  BENCHMARK( "power2 large exponent" ) {
    return power2( 20 ); // 2^20
  };
}
