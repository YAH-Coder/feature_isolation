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
   os << "IN THE FOLLOWING LIST TWO FEATURES ARE MISSING, NAMELY 00000000 and 11111111" << endl;
   fi.m_features.print_features(os);
}
