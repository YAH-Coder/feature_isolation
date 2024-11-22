/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "../include/isolation.hpp"
#include "../include/spl.hpp"

#include <chrono> // For timing
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main() {
  using namespace feature_isolation;

  // Start timing
  auto start = std::chrono::high_resolution_clock::now();

  feature_isolation_t fi( "spl_specs/factorial_test.csv" );

  std::stringstream ss;

  ss << "=== SPL SPECIFICATION ===" << std::endl;
  printer_t printer { ss };
  printer.print_specification( fi.m_spl );

  ss << "\n=== SYSTEMS ===" << std::endl;
  fi.m_features.print( ss );

  ss << "=== FEATURES BY CATEGORY ===" << std::endl;
  fi.m_features.print_features( ss );

  ss << "=== FEATURES BY INDEX ===" << std::endl;
  for ( auto index { one }; index <= fi.m_features.T; ++index ) {
    ss << index << separator() << fi.m_features.category_name( index )
       << separator() << to_string( fi.m_features.at( index ) ) << std::endl;
  }

  ss << "=== ISOLATION RESULTS ===" << std::endl;
  fi.print( ss, true, true ); // -min-diffs=true, -feature-names=true

  std::ofstream outfile( "output.csv" );
  outfile << ss.str();
  outfile.close();

  // Stop timing
  auto finish = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = finish - start;

  std::cout << "Elapsed time: " << elapsed.count() << " s" << std::endl;

  return 0;
}
