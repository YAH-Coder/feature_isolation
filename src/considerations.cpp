#include "../include/fi_mathhelper.hpp"
#include "../include/feature_isolation2.hpp"
using namespace feature_isolation;
#include <iostream>
#include <climits>
#include <set>
#include <vector>
#include <string>
using namespace std;

unsigned_integer S(unsigned_integer F)
{
   return power2(F);
}

unsigned_integer T(unsigned_integer F)
{
   return 2 * F +
          4 * sum_of_combinations(F,2) + 
          2 * (sum_of_combinations(F,1) - 1);
}

unsigned_integer feature_option1(unsigned_integer T,unsigned_integer S)
{
   return T * S * sizeof(unsigned_integer);
}

unsigned_integer feature_option2(unsigned_integer T,unsigned_integer S)
{
   return feature_option1(T,S) / 2;
}

unsigned_integer feature_option3(unsigned_integer T,unsigned_integer S)
{
   return T * S / CHAR_BIT;
}

unsigned_integer kibibyte(unsigned_integer byte)
{
   return byte / 1024;
}

unsigned_integer mebibyte(unsigned_integer byte)
{
   return kibibyte(byte) / 1024;
}

unsigned_integer system_option1(unsigned_integer S,unsigned_integer T)
{
   return S * (T / 2) * (S / CHAR_BIT);
}

unsigned_integer system_option2(unsigned_integer S,unsigned_integer T)
{
  return S * (T / 2) * sizeof(unsigned_integer); 
}

void memory_representation(unsigned_integer F)
{
   unsigned_integer S { power2(F) };
   unsigned_integer T { ::T(F) };
   cout << F << '\t' << S << '\t' << T << '\t'
        << mebibyte(feature_option1(T,S)) << '\t'
        << mebibyte(feature_option2(T,S)) << '\t'
        << mebibyte(feature_option3(T,S)) << '\t'
        << mebibyte(system_option1(S,T)) << '\t'
        << mebibyte(system_option2(S,T))
        << endl;
}

int main()
{
   cout << "F\tS\tT\tF1\tF2\tF3\tS1\tS2" << endl;
   for (unsigned_integer F = 2; F <= 20; ++F)
   {
      memory_representation(F);
   }
   feature_isolation_t fi { 4,4,ALL };
   cout << to_string(fi) << endl;
   cout << "F" << endl << to_string(fi.F_features) << endl
        << "N" << endl << to_string(fi.N_features) << endl
        << "A" << endl << to_string(fi.A_features) << endl
        << "O" << endl << to_string(fi.O_features) << endl
        << "A" << endl << to_string(fi.AN_features) << endl
        << "O" << endl << to_string(fi.ON_features) << endl;
}
