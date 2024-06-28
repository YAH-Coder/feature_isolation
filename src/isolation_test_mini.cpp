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
   fi.m_features.print(os);
   fi.m_features.print_features(os);
   os << "feature_analysis_t::at()" << endl;
   for (auto index { one }; index <= fi.m_features.T; ++index)
   {
      os << index << separator() << separator() << to_string( fi.m_features.at(index)) << endl;
   };
   fi.print_header(os);
   fi.print(os);
   auto result { fi.partition(34) };
   fi.print(cout,result);
   for (const auto& sid : fi.m_spl.m_available_systems)
   {
      cout << "SID " << sid << endl;
      for (const auto& f : fi.m_features.m_systems.at(sid - 1).all_features())
      {
         cout << to_string(f) << " ";
      }
      cout << endl;
      if (fi.m_features.m_systems.at(sid - 1).all_features().contains(34))
      {
         cout << " I " << sid;
      }
      else
      {
         cout << " U " << sid;
      }
      cout << endl;
   } 
   cout << endl;
//   fi.print(cout,fi.m_spl.m_available_systems);
//   cout << endl;
}
