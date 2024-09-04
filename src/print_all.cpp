#include "../include/feature_analysis.hpp"
#include <fstream>

using namespace std;

using namespace feature_isolation;

int main() {
  ofstream out { "fi2.csv" };
  feature_isolation::feature_analysis_t fi2 { 2, 2, ALL };
  out << "This is the output for the print function for 2features: " << endl;
  fi2.print( out );
  out << "This is the output for the print_features function for 2features: " << endl;
  fi2.print_features( out );
  out << "This is the output for the print_statistics function for 2features: " << endl;
  fi2.print_systems_statistics( out );
  out << "This is the output for the print_systems_features function for 2features: " << endl;
  fi2.print_systems_features( out );
  out << endl;
  feature_isolation::feature_analysis_t fi3 { 3, 3, ALL };
  out << "This is the output for the print function for 3features: " << endl;
  fi3.print( out );
  out << "This is the output for the print_features function for 3features: " << endl;
  fi3.print_features( out );
  out << "This is the output for the print_statistics function for 3features: " << endl;
  fi3.print_systems_statistics( out );
  out << "This is the output for the print_systems_features function for 3features: " << endl;
  fi3.print_systems_features( out );
  out << endl;
  feature_isolation::feature_analysis_t fi4 { 4, 4, ALL };
  out << "This is the output for the print function for 4features: " << endl;
  fi4.print( out );
  out << "This is the output for the print_features function for 4features: " << endl;
  fi4.print_features( out );
  out << "This is the output for the print_statistics function for 4features: " << endl;
  fi4.print_systems_statistics( out );
  out << "This is the output for the print_systems_features function for 4features: " << endl;
  fi4.print_systems_features( out );
  out << endl;
  feature_isolation::feature_analysis_t fi5 { 5, 5, ALL };
  out << "This is the output for the print function for 5features: " << endl;
  fi5.print( out );
  out << "This is the output for the print_features function for 5features: " << endl;
  fi5.print_features( out );
  out << "This is the output for the print_statistics function for 5features: " << endl;
  fi5.print_systems_statistics( out );
  out << "This is the output for the print_systems_features function for 5features: " << endl;
  fi5.print_systems_features( out );
  out << endl;
  return 0;
}
