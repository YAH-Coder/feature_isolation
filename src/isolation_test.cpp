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
}
