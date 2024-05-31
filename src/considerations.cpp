#include "../include/feature_analysis2.hpp"
#include "../include/fi_mathhelper.hpp"
#include "../include/spl.hpp"
#include "../include/isolation.hpp"
using namespace feature_isolation;
#include <climits>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
using namespace std;

unsigned_integer S( unsigned_integer F ) {
  return power2( F );
}

unsigned_integer T( unsigned_integer F ) {
  return 2 * F +
         4 * sum_of_combinations( F, 2 ) +
         2 * ( sum_of_combinations( F, 1 ) - 1 );
}

unsigned_integer feature_option1( unsigned_integer T, unsigned_integer S ) {
  return T * S * sizeof( unsigned_integer );
}

unsigned_integer feature_option2( unsigned_integer T, unsigned_integer S ) {
  return feature_option1( T, S ) / 2;
}

unsigned_integer feature_option3( unsigned_integer T, unsigned_integer S ) {
  return T * S / CHAR_BIT;
}

unsigned_integer kibibyte( unsigned_integer byte ) {
  return byte / 1024;
}

unsigned_integer mebibyte( unsigned_integer byte ) {
  return kibibyte( byte ) / 1024;
}

unsigned_integer system_option1( unsigned_integer S, unsigned_integer T ) {
  return S * ( T / 2 ) * ( S / CHAR_BIT );
}

unsigned_integer system_option2( unsigned_integer S, unsigned_integer T ) {
  return S * ( T / 2 ) * sizeof( unsigned_integer );
}

void memory_representation( unsigned_integer F ) {
  unsigned_integer S { power2( F ) };
  unsigned_integer T { ::T( F ) };
  cout << F << '\t' << S << '\t' << T << '\t'
       << mebibyte( feature_option1( T, S ) ) << '\t'
       << mebibyte( feature_option2( T, S ) ) << '\t'
       << mebibyte( feature_option3( T, S ) ) << '\t'
       << mebibyte( system_option1( S, T ) ) << '\t'
       << mebibyte( system_option2( S, T ) )
       << endl;
}

int main() {
  cout << "F\tS\tT\tF1\tF2\tF3\tS1\tS2" << endl;
  for ( unsigned_integer F = 2; F <= 20; ++F ) {
    memory_representation( F );
  }

  feature_isolation::feature_analysis_t fi { 4, 4, ALL };
  ofstream out { "fi4.csv" };
  fi.print( out );
  out << endl;
  fi.print_features( out );
  out << endl;
  fi.print_systems_statistics( out );
  out << endl;
  fi.print_systems_features( out );

  feature_isolation::spl_specification_t sample_spl_one { "sample_spl_spec.csv" };
  sample_spl_one.output_to_csv( "output_spl.csv" );
  feature_isolation::spl_specification_t sample_spl_two { "output_spl.csv" };
  sample_spl_two.output_to_csv( "output_spl_final.csv" );

{
   // Playing with feature_isolation_t
   feature_isolation_t isolation {"spl_isolation.csv" };
   auto filename { isolation.m_spl.m_filename };
   filename = filename.erase(filename.find(".csv"),4) + "_result.csv";
   ofstream o { filename };
   for (unsigned_integer id { 1 }; id <= 3; ++id )
   {
      auto decomposed_ids { isolation.decompose(id) };
      o << "Feature" << separator() << id << '\n'
        << "Present in system(s)";
        for (auto s : decomposed_ids.i)
        {
           o << separator() << s;
        }
        o << '\n';
        o << "Absent in system(s)";
        for (auto s : decomposed_ids.u)
        {
           o << separator() << s;
        }
        o << '\n' << endl;
   }
}
}
