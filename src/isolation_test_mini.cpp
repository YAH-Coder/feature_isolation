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
   fi.m_features.print_features(os);
   cout << "Number of N features = " << fi.m_features.N_features.size() << endl;
   for (auto f : fi.m_features.N_features)
   {
      cout << to_string(f) << endl;
   }
}
