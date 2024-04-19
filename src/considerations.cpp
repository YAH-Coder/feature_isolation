#include "../include/fi_mathhelper.hpp"
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

//////////////////////////////////////////////////
// Playing with some data types //////////////////
//////////////////////////////////////////////////

using feature_description = std::vector<bool>;
using features = std::vector<feature_description>;
using system_description = std::set<unsigned_integer>;
using systems = std::vector<system_description>;

std::string to_string(const feature_description& fd)
{
   string result { };
   for(const auto& b : fd)
   {
      //result += to_string(b); // LSB left most place, MSB right most place
      result = to_string(b) + result; //  MSB left most place, LSB right most place
   }
   return result;
}

// The following function requires a maximum of S = 32
unsigned_integer to_unsigned_integer(const feature_description& fd)
{
   unsigned_integer result { 0 };
   for (unsigned_integer exponent { 0 };const auto& b : fd)
   {
      arithmetic::add_assign(result,(b ? power2(exponent) : 0));
      ++exponent;
   }
   return result;
}

std::string to_string(const system_description& sd,const features& f)
{
   string result { "{ " };
   for (const auto& i : sd)
   {
      result += to_string(to_unsigned_integer(f.at(i))) + " ";
   }
   result += "}";
   return result;
}

std::string to_string(const systems& s,const features& f)
{
   string result { };
   for (const auto& system: s)
   {
      result += to_string(system,f) + "\n";
   }
   return result;
}

std::string to_string(const features& f)
{
   string result { };
   for(const auto& feature : f)
   {
      result += to_string(feature) + "\n";
   }
   return result;
}

void test()
{
   systems s(4);
   features f(32,feature_description(32,false));
   for (unsigned_integer p { 0 };auto& feature : f)
   {
      feature.at(p) = true;
      s.at(p % 4).insert(p);
      ++p;
   }
   cout << to_string(f) << endl;
   cout << to_string(s,f) << endl;
}

int main()
{
   cout << "F\tS\tT\tF1\tF2\tF3\tS1\tS2" << endl;
   for (unsigned_integer F = 2; F <= 20; ++F)
   {
      memory_representation(F);
   }
   test();
}
