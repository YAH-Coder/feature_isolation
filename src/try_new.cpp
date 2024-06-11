#include "../include/spl_test.hpp"

using namespace feature_isolation;
using namespace std;

int main() {
   spl_processor sp{"../spl_specs/spl_isolation.csv"};
   cout << "Result of processing = " << sp.process() << endl;
   auto spl{sp.get_specification()};
   sp.print("result.txt");
   Printer p{cout};
   p.print_specification(spl);
}
