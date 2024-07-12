#include "../include/feature_analysis.hpp"
#include "../include/isolation.hpp"
#include "../include/spl.hpp"
#include <fstream>
#include <iostream>
using namespace std;

using namespace feature_isolation;

int main()
{
  feature_isolation_t fi { "../spl_specs/spl_isolation4.csv" };
  ofstream            os { "../src/spl_isolation_result.csv" };
  os << "=== SYSTEMS ===" << endl;
  fi.m_features.print(os);
  os << "=== FEATURES BY CATEGORY ===" << endl;
  fi.m_features.print_features(os);
  os << "=== FEATURES BY INDEX ===" << endl;
  for (auto index { one }; index <= fi.m_features.T; ++index)
  {
    os << index << separator() << fi.m_features.category_name(index)
       << separator() << to_string(fi.m_features.at(index)) << endl;
  };
  os << "=== ISOLATION RESULTS ===" << endl;
  fi.print(os,false,true);
  for (auto i { one };auto F : fi.m_features.F_features)
  {
     for (auto j { zero };auto N : fi.m_features.N_features)
     {
        cout << setw(3) << i << " " << to_string(F) << endl
             << setw(3) << ++j + fi.m_features.N_features.size() << " " << to_string(N) << endl;
        auto r { F };
        for (auto k { zero }; k < F.size(); ++k)
        {
           r.at(k) = r.at(k) * N.at(k);
        }
        cout << "  * " << to_string(r) << endl << endl;
     }
     ++ i;
  }
}
