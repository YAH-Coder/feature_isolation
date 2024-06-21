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
   ofstream os { "../src//spl_islation_result.csv" };
   fi.print(os);
}
