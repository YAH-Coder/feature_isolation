#ifndef FEATURE_ISOLATION_HPP
#define FEATURE_ISOLATION_HPP

#include "../include/fi_mathhelper.hpp"
#include <algorithm>
#include <vector>
#include <set>
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <numeric>

namespace feature_isolation
{
   using feature_t = std::vector<bool>;
   using features_t = std::vector<feature_t>;
   using system_t = std::set<unsigned_integer>;
   using systems_t = std::vector<system_t>;

   constexpr bool MSB_LSB { true }; // Customization point for MSB at
                                    // left-most position.

   std::string to_bitstring(unsigned_integer b) // For debugging only.
   {
   
      std::string result { };
      for (auto i { zero }; i <= sizeof(unsigned_integer) * 8; ++i)
      {
         result = ((b & one << i) ? "1" : "0") + result;
      }
      return result;
   }

   std::string to_string(const feature_t& fd)
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

   unsigned_integer to_unsigned_integer(const feature_t& fd)
   {
      auto result { zero };
      for (auto exponent { zero };const auto& b : fd)
      {
         arithmetic::add_assign(result,(b ? power2(exponent) : 0));
         ++exponent;
      }
      return result;
   }

   std::string to_string(const system_t& sd,const features_t& f)
   {
      std::string result { "{ " };
      for (const auto& i : sd)
      {
         result += std::to_string(to_unsigned_integer(f.at(i))) + " ";
      }
      result += "}";
      return result;
   }

   std::string to_string(const systems_t& s,const features_t& f)
   {
      std::string result { };
      for (const auto& system: s)
      {
         result += to_string(system,f) + "\n";
      }
      return result;
   }

   std::string to_string(const features_t& f)
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

   [[nodiscard]] features_t initialize_A_features(unsigned_integer F,
                                                  unsigned_integer S,
                                                  unsigned_integer R,
                                                  unsigned_integer feature_categories)
   {
      features_t result;
      if (feature_categories & ::feature_isolation::A)
      {
         if (R > F)
         {
            throw std::invalid_argument("R > F in feature_isolation::initialize_A_features");
         }
         result = features_t(sum_of_combinations(F,2,R),feature_t(S,false));
         std::vector<unsigned_integer> all_features(F);
         std::iota(all_features.begin(),all_features.end(),zero); // fill with zero .. F - 1
         auto feature_index { zero };
         for (auto drawing_size { two }; drawing_size <= R; ++drawing_size)
         {
            combination selection { all_features,drawing_size };
            do
            {
               auto mask { zero };
               //std::cout << "*** ";
               for (auto e : selection())
               {
                  //std::cout << e << " ";
                  //arithmetic::add_assign(mask,power2(e));
                  mask |= (one << e);
               }
               //std::cout << " " << mask << std::endl;
               for (auto system_index { zero };system_index < S;++system_index)
               {
                  if ((system_index & mask) == mask)
                  {
                     //std::cout << "MA " << to_bitstring(mask) << std::endl 
                     //          << "SI " << to_bitstring(system_index) << std::endl;
                     result.at(feature_index).at(system_index) = true;
                  }
               }
               ++feature_index;
            } while (selection.next());
          }
       }
      return result;   
   }

   [[nodiscard]] features_t initialize_O_features(unsigned_integer F,
                                                  unsigned_integer S,
                                                  unsigned_integer R,
                                                  unsigned_integer feature_categories)
   {
      features_t result;
      if (feature_categories & ::feature_isolation::A)
      {
         if (R > F)
         {
            throw std::invalid_argument("R > F in feature_isolation::initialize_O_features");
         }
         result = features_t(sum_of_combinations(F,2,R),feature_t(S,false));
         std::vector<unsigned_integer> all_features(F);
         std::iota(all_features.begin(),all_features.end(),zero); // fill with zero .. F - 1
         auto feature_index { zero };
         for (auto drawing_size { two }; drawing_size <= R; ++drawing_size)
         {
            combination selection { all_features,drawing_size };
            do
            {
               auto mask { zero };
               //std::cout << "*** ";
               for (auto e : selection())
               {
                  //std::cout << e << " ";
                  //arithmetic::add_assign(mask,power2(e));
                  mask |= (one << e);
               }
               //std::cout << " " << mask << std::endl;
               for (auto system_index { zero };system_index < S;++system_index)
               {
                  if (system_index & mask)
                  {
                     //std::cout << "MA " << to_bitstring(mask) << std::endl 
                     //          << "SI " << to_bitstring(system_index) << std::endl;
                     result.at(feature_index).at(system_index) = true;
                  }
               }
               ++feature_index;
            } while (selection.next());
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
              A { sum_of_combinations( F, two ) },
              O { A },
              N { F },
              AN { O },
              ON { O },
              ANF { sum_of_combinations( F, 1 ) - 1 },
              ONF { ANF },
              DF { f + 4 * O + 2 * ANF },
              T { F + DF },
              m_max_rank { max_rank },
              m_feature_categories { feature_categories },
              F_features { initialize_F_features() },
              N_features { initialize_N_features() },
              A_features { initialize_A_features(F,S,m_max_rank,m_feature_categories) },
              O_features { initialize_O_features(F,S,m_max_rank,m_feature_categories) } 
         {};
         using cui = const unsigned_integer;
         cui F,
             S,
             A,
             O,
             N,
             AN,
             ON,
             ANF,
             ONF,
             DF,
             T,
             m_max_rank,
             m_feature_categories;
         features_t F_features,
                    N_features,
                    A_features,
                    O_features;
      private:
         [[nodiscard]] features_t initialize_F_features()
         {
           features_t result;
           result = features_t(F,feature_t(S,false));
           auto stride { one };
           for (auto feature_index { zero };feature_index < F;++feature_index)
           {  
              bool value { true };
              auto counter { zero };
              for (auto system_index { zero };system_index < S;++system_index)
              {
                 if (counter == stride)
                 {
                    value = !value;
                    counter = zero;
                 }
                 ++counter;
                 result.at(feature_index).at(system_index) = value;
              }
              stride *= 2;
           }
           return result;
         }
         [[nodiscard]] features_t initialize_N_features()
         {
           features_t result { F_features };
           for (auto& feature : F_features)
           feature.flip();
           return result;
         }
   };

std::string to_string(const feature_isolation_t& fi)
   {
      std::ostringstream os;
      os << "F = " << fi.F << " S = " << fi.S 
         << " A = " << fi.A << " O = " << fi.O 
         << " N = " << fi.N << " AN = " << fi.AN << " ON = " << fi.ON
         << " ANF = " << fi.ANF << " ONF = " << fi.ONF
         << " DF = " << fi.DF << " T = " << fi.T << std::endl;
      return os.str();
   }
}
#endif // FEATURE_ISOLATION
