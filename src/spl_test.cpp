#include "../include/spl_z3.hpp"

using namespace feature_isolation;

int main( int argc, char* argv[] ) {
  if ( argc < 2 ) {
    std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
    return 1;
  }

  std::string input_filename = argv[1];
  spl_processor_t processor( input_filename );

  if ( !processor.process() ) {
    return 1;
  }

  processor.print( "output.csv" );

  return 0;
}
