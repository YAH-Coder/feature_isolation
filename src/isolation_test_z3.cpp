#include "../include/spl_z3.hpp"
#include "../include/isolation.hpp"

using namespace std;

using namespace feature_isolation;

int main()
{
  feature_isolation_t fi { "../spl_specs/factorial.csv" };
  ofstream            os { "./temp.csv" };
  os << "=== SPL SPECIFICATION ===" << endl;
  printer_t printer { os };
  printer.print_specification(fi.m_spl);
  os << "\n=== SYSTEMS ===" << endl;
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
