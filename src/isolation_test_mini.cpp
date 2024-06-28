#include "../include/isolation.hpp"
#include "../include/feature_analysis.hpp"
#include "../include/spl.hpp"
#include <iostream>
#include <fstream>
using namespace std;

using namespace feature_isolation;

int main ()
{
   feature_isolation_t fi { "../spl_specs/spl_isolation.csv" };
   ofstream os { "../src/spl_isolation_result.csv" };
   os << "=== SYSTEMS ===" << endl;
   fi.m_features.print(os);
   os << "=== FEATURES BY CATEGORY ===" << endl;
   fi.m_features.print_features(os);
   os << "=== FEATURES BY INDEX ===" << endl;
   for (auto index { one }; index <= fi.m_features.T; ++index)
   {
      os << index << separator() << separator() << to_string( fi.m_features.at(index)) << endl;
   };
   fi.print(os);
}
