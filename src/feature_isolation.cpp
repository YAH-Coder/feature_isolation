#include "../include/spl.hpp"
#include "../include/isolation.hpp"
#include <string>
#include <fstream>

using namespace std;

using namespace feature_isolation;

int main(int argc,char * argv[])
{
  if (argc != 3)
  {
     cerr << "Invalid number of arguments, expected 3, got " << argc
          << "\nMust be called with program-name spl-specification-file result-file."
          << "\nProgram terminated." << endl;
     return 1;
  }
  ifstream spl_file { argv[1] };
  if (spl_file)
  {
     spl_file.close();
  }
  else
  {
     cerr << "Cannot open spl-specification-file named \"" << argv[1]
          << "\".\nProgram terminated." << endl;
     return 2;
  }
  ofstream result_file { argv[2] };
  if (!result_file)
  {
     cerr << "Cannot open result-file named \"" << argv[2]
          << "\".\nProgram terminated." << endl;
     return 3;
  }
  feature_isolation_t fi { argv[1] };
  result_file << "=== SPL SPECIFICATION ===" << endl;
  printer_t printer { result_file };
  printer.print_specification(fi.m_spl);
  result_file << "\n=== SYSTEMS ===" << endl;
  fi.m_features.print(result_file);
  result_file << "=== FEATURES BY CATEGORY ===" << endl;
  fi.m_features.print_features(result_file);
  result_file << "=== FEATURES BY INDEX ===" << endl;
  for (auto index { one }; index <= fi.m_features.T; ++index)
  {
    result_file << index << separator() << fi.m_features.category_name(index)
       << separator() << to_string(fi.m_features.at(index)) << endl;
  };
  result_file << "=== ISOLATION RESULTS ===" << endl;
  fi.print(result_file,true,false); 
}
