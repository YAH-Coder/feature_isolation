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
   cout << "Tbefore = " << fi.m_features.T << endl;
   ofstream os { "../src//spl_islation_result.csv" };
   fi.m_features.print_features(os);
   os << endl;
   for (auto fid { one };fid <= fi.m_features.T; ++fid)
   {
      os << fid << separator();
      fi.m_features.print_feature(os,fi.m_features.at(fid));
      os << endl;
   }
   fi.print(os);
}
