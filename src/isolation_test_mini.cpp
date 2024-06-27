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
   ofstream os { "../src//spl_isolation_result.csv" };
   fi.m_features.print(os);
   os << "THE 3 NOT FEATURES (IDS 4, 5 and 6) ARE MISSING" << endl;
   fi.m_features.print_features(cout);
   for (auto index { one }; index <= fi.m_features.T; ++index)
   {
      cout << "FID " << index << " " << to_string( fi.m_features.at(index)) << endl;
   }
}
