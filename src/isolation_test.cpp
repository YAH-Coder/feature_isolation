#include "../include/isolation.hpp"
#include "../include/feature_analysis.hpp"
#include "../include/spl.hpp"
#include <iostream>
using namespace std;

using namespace feature_isolation;

int main ()
{
   feature_isolation_t fi{ "../spl_specs/spl_isolation.csv" };
/*   cout << fi.m_features.m_systems.at(0).T() << endl;
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
   print("D flattened",fi.flattened_difference(1)); // DON'T CALL THIS METHOD!
   print("D flattened again",fi.flattened_difference(i,u)); */
   for (auto fid { one };fid <= fi.m_features.T;++fid)
   {
      auto result { fi.minimal_differences(fid) };
      print("D flat",result.features);
      cout << "Minimal subsets for isolating feature with id " << fid << "\n";
      for (auto i { zero };auto s : result.differences)
      {
         cout << ++i << endl;
         print("I",s.I);
         print("U",s.U);
      }
   }
/*   system_ids_t s1 { 1, 2, 3 },
                s2 { 2, 3 },
                s3 { 5, 6, 7 },
                s4 { 5, 6 };
   difference_t d1 { s1, s3 },
                d2 { s2, s4 };
   cout << feature_isolation_t::proper_difference_subset(s2,s4,d1) << endl
        << feature_isolation_t::proper_difference_subset(s2,s4,d2) << endl;
   differences_t m;
   m.insert(d1);
   cout << m.size() << endl;
   m.insert(d1);
   cout << m.size() << endl;*/
}
