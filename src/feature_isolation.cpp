/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "../include/isolation.hpp"
#include "../include/spl.hpp"
#include <fstream>
#include <string>

using namespace std;

using namespace feature_isolation;

int main(int argc, char* argv[])
{
  if (argc != 5)
  {
    cerr << "Invalid number of arguments, expected 5, got " << argc
         << "\nMust be called with program-name spl-specification-file "
            "result-file -min-diffs=[false|true] -feature-names=[false|true]."
         << "\nProgram terminated." << endl;
    return 1;
  }
  ifstream spl_file { argv[1] };
  if (spl_file)
  {
    spl_file.close();
  }
  else
  {
    cerr << "Cannot open spl-specification-file named \"" << argv[1]
         << "\".\nProgram terminated." << endl;
    return 2;
  }
  ofstream result_file { argv[2] };
  if (!result_file)
  {
    cerr << "Cannot open result-file named \"" << argv[2]
         << "\".\nProgram terminated." << endl;
    return 3;
  }
  string diff_option { argv[3] }, names_option { argv[4] };
  bool   min_diffs {}, names {};
  if (diff_option == "-min-diffs=false")
  {
    min_diffs = false;
  }
  else if (diff_option == "-min-diffs=true")
  {
    min_diffs = true;
  }
  else
  {
    cerr << "Unknown option \"" << diff_option << "\".\n"
         << "Expected \"-min-diffs=false\" or  \"-min-diffs=true\".\n"
         << "Program terminated." << endl;
    return 4;
  }
  if (names_option == "-feature-names=false")
  {
    names = false;
  }
  else if (names_option == "-feature-names=true")
  {
    names = true;
  }
  else
  {
    cerr << "Unknown option \"" << names_option << "\".\n"
         << "Expected \"-feature-names=false\" or  \"-feature-names=true\".\n"
         << "Program terminated." << endl;
    return 5;
  }
  feature_isolation_t fi { argv[1] };
  result_file << "=== SPL SPECIFICATION ===" << endl;
  printer_t printer { result_file };
  printer.print_specification(fi.m_spl);
  result_file << "\n=== SYSTEMS ===" << endl;
  fi.m_features.print(result_file);
  result_file << "=== FEATURES BY CATEGORY ===" << endl;
  fi.m_features.print_features(result_file);
  result_file << "=== FEATURES BY INDEX ===" << endl;
  for (auto index { one }; index <= fi.m_features.T; ++index)
  {
    result_file << index << separator() << fi.m_features.category_name(index)
                << separator() << to_string(fi.m_features.at(index)) << endl;
  };
  result_file << "=== ISOLATION RESULTS ===" << endl;
  fi.print(result_file, min_diffs, names);
}
 
