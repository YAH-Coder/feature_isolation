#include "../include/isolation.hpp"
#include "../include/feature_analysis.hpp"
#include "../include/spl.hpp"
#include <iostream>
using namespace std;

using namespace feature_isolation;

int main ()
{
   feature_isolation_t fi{ "../spl_specs/spl_isolation.csv" };
   cout << fi.m_features.m_systems.at(0).T() << endl;
   auto [i,u] = fi.partition(1);
   print("I",i);
   print("U",u);
   for (auto s : i)
   {
      print("FI",fi.sid_to_fids(s));
   }
   auto i_flat { fi.flatten_by_intersection(i) };
   print("I flattened",i_flat);
   for (auto s : i)
   {
      print("FU",fi.sid_to_fids(s));
   }
   auto u_flat { fi.flatten_by_union(u) };
   print("U flattened",u_flat);
   print("D flattened",fi.flattened_difference(1));
}
