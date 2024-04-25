#ifndef FEATURE_ISOLATION_HPP
#define FEATURE_ISOLATION_HPP

#include "../include/fi_mathhelper.hpp"
#include <algorithm>
#include <vector>
#include <set>
#include <iostream>
#include <string>
#include <sstream>

namespace feature_isolation
{
   using feature_description = std::vector<bool>;
   using features = std::vector<feature_description>;
   using system_description = std::set<unsigned_integer>;
   using systems = std::vector<system_description>;

   constexpr bool MSB_LSB { true }; // Customization point for MSB at
                                    // left-most position.

   std::string to_string(const feature_description& fd)
   {
     std::string result { };
      for(const auto& b : fd)
      {
         if constexpr (MSB_LSB)
         {
             result = std::to_string(b) + result; //  MSB left most place, LSB right most place
         }
         else
         {
            result += std::to_string(b); // LSB left most place, MSB right most place
         }
     
      }
      return result;
   }

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
      std::string result { "{ " };
      for (const auto& i : sd)
      {
         result += std::to_string(to_unsigned_integer(f.at(i))) + " ";
      }
      result += "}";
      return result;
   }

   std::string to_string(const systems& s,const features& f)
   {
      std::string result { };
      for (const auto& system: s)
      {
         result += to_string(system,f) + "\n";
      }
      return result;
   }

   std::string to_string(const features& f)
   {
      std::string result { };
      for(const auto& feature : f)
      {
         result += to_string(feature) + "\n";
      }
      return result;
   }

   static const unsigned_integer F { one };
   static const unsigned_integer O { feature_isolation::power2( 2 ) };
   static const unsigned_integer A { feature_isolation::power2( 3 ) };
   static const unsigned_integer N { feature_isolation::power2( 4 ) };
   static const unsigned_integer FN { F + N };
   static const unsigned_integer ON { feature_isolation::power2( 5 ) };
   static const unsigned_integer AN { feature_isolation::power2( 6 ) };
   static const unsigned_integer ONF { feature_isolation::power2( 7 ) };
   static const unsigned_integer ANF { feature_isolation::power2( 8 ) };
   static const unsigned_integer FOA { F + O + A };
   static const unsigned_integer FOAN { FOA + N };
   static const unsigned_integer FOANONAN { FOAN + ON + AN };
   static const unsigned_integer ALL { FOANONAN + ONF + ANF };

   [[nodiscard]] features initialize_F_features(unsigned_integer F,
                                                unsigned_integer S,
                                                unsigned_integer feature_categories)
   {
      features result;
      if (feature_categories & ::feature_isolation::F)
      {
         result = features(F,feature_description(S,false));
         for (unsigned_integer feature_index { 0 };feature_index < F;++feature_index)
         {
            for (unsigned_integer system_index { 0 };system_index < S;++system_index)
            {
               if (system_index & (one << feature_index))
               {
                  result.at(feature_index).at(system_index) = true;
               }
            }
          }
       }
      return result;   
   }

   [[nodiscard]] features initialize_N_features(unsigned_integer F,
                                                unsigned_integer S,
                                                unsigned_integer feature_categories)
   {
      features result;
      if (feature_categories & ::feature_isolation::N)
      {
         result = features(F,feature_description(S,false));
         for (unsigned_integer feature_index { 0 };feature_index < F;++feature_index)
         {
            for (unsigned_integer system_index { 0 };system_index < S;++system_index)
            {
               if (!(system_index & (one << feature_index)))
               {
                  result.at(feature_index).at(system_index) = true;
               }
            }
          }
       }
      return result;   
   }

class feature_isolation_t
   {
      public:
         explicit feature_isolation_t(unsigned_integer f,
                                      unsigned_integer max_rank,
                                      unsigned_integer feature_categories = ::feature_isolation::ALL)
            : F {f },
              S { power2( F ) },
              O { sum_of_combinations( F, two ) },
              A { O },
              N { F },
              AN { O },
              ON { O },
              ANF { sum_of_combinations( F, 1 ) - 1 },
              ONF { ANF },
              DF { f + 4 * O + 2 * ANF },
              T { F + DF },
              m_max_rank { max_rank },
              m_feature_categories { feature_categories },
              F_features { initialize_F_features(F,S,m_feature_categories) },
              N_features { initialize_N_features(F,S,m_feature_categories) }
         {};
         using cui = const unsigned_integer;
         cui F,
             S,
             O,
             A,
             N,
             AN,
             ON,
             ANF,
             ONF,
             DF,
             T,
             m_max_rank,
             m_feature_categories;
         features F_features,
                  N_features;
   };

std::string to_string(const feature_isolation_t& fi)
   {
      std::ostringstream os;
      os << "F = " << fi.F << " S = " << fi.S 
         << " O = " << fi.O << " A = " << fi.A 
         << " N = " << fi.N << " AN = " << fi.AN << " ON = " << fi.ON
         << " ANF = " << fi.ANF << " ONF = " << fi.ONF
         << " DF = " << fi.DF << " T = " << fi.T << std::endl;
      return os.str();
   }
}
#endif // FEATURE_ISOLATION
