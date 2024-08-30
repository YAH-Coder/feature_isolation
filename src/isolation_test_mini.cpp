#include "../include/feature_analysis.hpp"
#include "../include/isolation.hpp"
#include "../include/spl.hpp"
#include <fstream>
#include <iostream>
using namespace std;

using namespace feature_isolation;

auto or_combine(feature_t f1,feature_t f2)
{
   for (auto i { zero };i < f1.size();++i)
      f1.at(i) = f1.at(i) || f2.at(i);
   return f1;
}

auto and_combine(feature_t f1,feature_t f2)
{
   for (auto i { zero };i < f1.size();++i)
      f1.at(i) = f1.at(i) && f2.at(i);
   return f1;
}

auto or_not_combine(feature_t f1,feature_t f2)
{
   for (auto i { zero };i < f1.size();++i)
      f1.at(i) = f1.at(i) || !f2.at(i);
   return f1;
}

auto and_not_combine(feature_t f1,feature_t f2)
{
   for (auto i { zero };i < f1.size();++i)
      f1.at(i) = f1.at(i) && !f2.at(i);
   return f1;
}
int main()
{
//  feature_isolation_t fi { "../spl_specs/spl_isolation3.csv" };
//  ofstream            os { "../src/spl_isolation_result3.csv" };
  feature_isolation_t fi { "../spl_specs/factorial.csv" };
  cout << "3" << endl;
  ofstream            os { "./temp.csv" };
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
  fi.print(os,true,false); 
}
