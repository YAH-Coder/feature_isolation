#include "../include/isolation.hpp"
#include "../include/feature_analysis.hpp"
#include "../include/spl.hpp"
#include <iostream>
#include <fstream>
using namespace std;

using namespace feature_isolation;

int main ()
{
   feature_isolation_t fi{ "../spl_specs/spl_isolation.csv" };
   ofstream os { "../src/spl_isolation_result.csv" };
   fi.m_features.print(os);
   fi.m_features.print_features(os);
   os << "feature_analysis_t::at()" << endl;
   for (auto index { one }; index <= fi.m_features.T; ++index)
   {
      os << index << separator() << separator() << to_string( fi.m_features.at(index)) << endl;
   }
}
