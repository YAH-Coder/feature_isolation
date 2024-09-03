#include "../include/spl_z3.hpp"
#include "../include/isolation.hpp"

using namespace std;

using namespace feature_isolation;

int main()
{
/*   spl_processor_t spl_proc { "../spl_specs/spl_isolation3.csv" };
   spl_proc.print("./temp.csv");
   spl_proc.process();
   spl_proc.print("temp.csv");
   auto spl_spec { spl_proc.get_specification() };
   printer_t spl_print { cout };
   spl_print.print_specification(spl_spec); // liefert das Gleiche wie spl_proc.print("./temp.csv"; */
  try
  {
  feature_isolation_t fi { "../spl_specs/factorial.csv" };
  } catch (const overflow_error& e)
  {
     cerr << e.what() << endl;
  }
  ofstream            os { "./temp.csv" };

}
