#include "../include/fi_mathhelper.hpp"
using namespace feature_isolation;
#include <iostream>
#include <climits>
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

unsigned_integer option1(unsigned_integer T,unsigned_integer S)
{
   return T * S * sizeof(unsigned_integer);
}

unsigned_integer option2(unsigned_integer T,unsigned_integer S)
{
   return option1(T,S) / 2;
}

unsigned_integer option3(unsigned_integer T,unsigned_integer S)
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

void memory_feature_representation(unsigned_integer F)
{
   unsigned_integer S { power2(F) };
   cout << F << '\t' << T(F) << '\t'
        << kibibyte(option1(F,S)) << '\t'
        << kibibyte(option2(F,S)) << '\t'
        << kibibyte(option3(F,S)) 
        << endl;
}

int main()
{
   cout << "F\tT\tO1\tO2\tO3" << endl;
   for (unsigned_integer F = 2; F <= 32; ++F)
   {
      memory_feature_representation(F);
   }
}
