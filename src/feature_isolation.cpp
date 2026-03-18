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

namespace
{
  std::string system_membership_set(const feature_t& systems_present,
                                    const system_space_t& system_space)
  {
    std::vector<unsigned_integer> ordered_valid_system_ids(
        system_space.valid.begin(), system_space.valid.end());
    std::ostringstream os;
    bool first = true;
    os << "{";
    for (auto system_index { zero }; system_index < ordered_valid_system_ids.size();
         ++system_index)
    {
      if (!systems_present.at(system_index))
      {
        continue;
      }
      if (!first)
      {
        os << ", ";
      }
      os << paper_system_label(ordered_valid_system_ids.at(system_index));
      first = false;
    }
    os << "}";
    return first ? std::string{"∅"} : os.str();
  }
}

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
  if (fi.m_features.uses_theory_registry())
  {
    result_file << "\n=== THEORY DSL COVERAGE ===" << endl;
    printer.print_theory_dsl_coverage(fi.m_spl);
  }
  result_file << "\n=== SYSTEMS ===" << endl;
  fi.m_features.print(result_file);
  result_file << "=== FEATURES BY CATEGORY ===" << endl;
  fi.m_features.print_features(result_file);
  result_file << "=== FEATURES BY INDEX ===" << endl;
  result_file << "i" << separator() << "FEATURE" << separator() << "CATEGORY"
              << separator() << "t_i" << separator() << "β(t_i)" << separator()
              << "ℓ_T(t_i)(simplified)" << separator() << "ℓ_T(t_i)(raw term)"
              << endl;
  for (const auto& record : fi.m_features.feature_registry())
  {
    if (!fi.m_features.includes_category(record.category))
    {
      continue;
    }
    result_file << paper_feature_index(record.id) << separator()
                << paper_feature_label(record.id) << separator()
                << feature_isolation::to_string(record.category) << separator()
                << system_membership_set(record.systems_present, fi.m_features.system_space())
                << separator() << to_string(record.systems_present) << separator()
                << record.simplified_term << separator() << record.raw_term << endl;
  }
  result_file << "=== ISOLATION RESULTS ===" << endl;
  fi.print(result_file, min_diffs, names);
}
