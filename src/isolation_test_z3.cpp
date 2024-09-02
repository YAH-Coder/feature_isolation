#include "../include/spl_z3.hpp"

using namespace std;

using namespace feature_isolation;

int main()
{
   spl_processor_t spl_proc { "../spl_specs/spl_isolation3.csv" };
   spl_proc.process();
   spl_proc.print("temp.csv");
   auto spl_spec { spl_proc.get_specification() };
}
