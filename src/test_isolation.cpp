#include "../include/isolation.hpp"
#include "../include/feature_analysis.hpp"
#include "../include/spl.hpp"

using namespace feature_isolation;

int
main ()
{
   feature_isolation_t fi{ "spl_isolation.csv" };
   auto x { fi.m_features.m_systems.at(0).T() };
}
