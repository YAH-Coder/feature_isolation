#ifndef FEATURE_ISOLATION_HPP
#define FEATURE_ISOLATION_HPP

#include "../include/fi_mathhelper.hpp"
#include <algorithm> // Because of iota() and find().
#include <vector>
#include <set>
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <numeric> // Because of CHAR_BIT.

namespace feature_isolation
{
   using feature_t = std::vector<bool>;
   using features_t = std::vector<feature_t>;
   class system_t
   {
     public:
       std::set<unsigned_integer> F,
                                  N,
                                  A,
                                  O,
                                  AN,
                                  ON,
                                  ANF,
                                  ONF;
      [[nodiscard]] auto DF() const
      {
        return N.size() + A.size() + O.size() +
               AN.size() + ON.size() +
               ANF.size() + ONF.size();
      }
      [[nodiscard]] auto T() const
      {
        return DF() + F.size();
      }
      auto all_features() const
      {
        auto result { F };
        for (auto f : N)
        {
          result.insert(f);
        }
        for (auto f : A)
        {
          result.insert(f);
        }
        for (auto f : O)
        {
          result.insert(f);
        }   
        for (auto f : AN)
        {
          result.insert(f);
        }   
        for (auto f : ON)
        {
          result.insert(f);
        }   
        for (auto f : ANF)
        {
          result.insert(f);
        }   
        for (auto f : ONF)
        {
          result.insert(f);
        }   
        return result;
      }
   };
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

   std::string to_string(const system_t& s,const features_t& f)
   {
      std::string result { "{ " };
      for (const auto& i : s.all_features())
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
   static const unsigned_integer A { feature_isolation::power2( 2 ) };
   static const unsigned_integer O { feature_isolation::power2( 3 ) };
   static const unsigned_integer N { feature_isolation::power2( 4 ) };
   static const unsigned_integer FN { F + N };
   static const unsigned_integer AN { feature_isolation::power2( 5 ) };
   static const unsigned_integer ON { feature_isolation::power2( 6 ) };
   static const unsigned_integer ANF { feature_isolation::power2( 7 ) };
   static const unsigned_integer ONF { feature_isolation::power2( 8 ) };
   static const unsigned_integer FAO { F + A + O };
   static const unsigned_integer FAON { FAO + N };
   static const unsigned_integer FAONANON { FAON + AN + ON };
   static const unsigned_integer ALL { FAONANON + ANF + ONF };

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
              DF { f + 4 * A + 2 * ANF },
              T { F + DF },
              R { max_rank },
              C { feature_categories },
              F_features { initialize_F_features() },
              N_features { initialize_N_features() },
              A_features { initialize_A_features() },
              O_features { initialize_O_features() },
              AN_features { initialize_AN_features() },
              ON_features { initialize_ON_features() },
              ANF_features { initialize_ANF_features() },
              ONF_features { initialize_ONF_features() },
              m_systems { initialize_systems() }
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
             R,
             C;
         features_t F_features,
                    N_features,
                    A_features,
                    O_features,
                    AN_features,
                    ON_features,
                    ANF_features,
                    ONF_features;
         systems_t m_systems;
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
        void and_combine(feature_t& f1,const feature_t& f2)
         {
           for (auto index { zero }; index < f1.size(); ++index)
           {
             f1.at(index) = f1.at(index) && f2.at(index);
           }
         }
         [[nodiscard]] features_t initialize_A_features()
         {
           if (R > F)
           {
             throw std::invalid_argument("R > F in feature_isolation_t::initialize_A_features()");
           }
           auto result = features_t(sum_of_combinations(F,2,R),feature_t(S,true));
           std::vector<unsigned_integer> feature_indices(F);
           std::iota(feature_indices.begin(),feature_indices.end(),zero);
           auto feature_index { zero };
           for (auto drawing_size { two }; drawing_size <= R; ++drawing_size)
           {
             combination selection { feature_indices,drawing_size };
             do
             {
               auto indices { selection() };
               for (auto index { zero };index < indices.size(); ++index)
               {
                  and_combine(result.at(feature_index),F_features.at(indices.at(index)));
               }
               ++feature_index;
             } while (selection.next());
           }
           return result;   
         }
         void or_combine(feature_t& f1,const feature_t& f2)
         {
           for (auto index { zero }; index < f1.size(); ++index)
           {
             f1.at(index) = f1.at(index) || f2.at(index);
           }
         }
         [[nodiscard]] features_t initialize_O_features()
         {
           if (R > F)
           {
             throw std::invalid_argument("R > F in feature_isolation_t::initialize_O_features()");
           }
           auto result = features_t(sum_of_combinations(F,2,R),feature_t(S,false));
           std::vector<unsigned_integer> feature_indices(F);
           std::iota(feature_indices.begin(),feature_indices.end(),zero);
           auto feature_index { zero };
           for (auto drawing_size { two }; drawing_size <= R; ++drawing_size)
           {
             combination selection { feature_indices,drawing_size };
             do
             {
               auto indices { selection() };
               for (auto index { zero };index < indices.size(); ++index)
               {
                  or_combine(result.at(feature_index),F_features.at(indices.at(index)));
               }
               ++feature_index;
             } while (selection.next());
           }
           return result;   
         }
         [[nodiscard]] features_t initialize_AN_features()
         {
           if (R > F)
           {
             throw std::invalid_argument("R > F in feature_isolation_t::initialize_AN_features()");
           }
           auto result = features_t(sum_of_combinations(F,2,R),feature_t(S,true));
           std::vector<unsigned_integer> feature_indices(F);
           std::iota(feature_indices.begin(),feature_indices.end(),zero);
           auto feature_index { zero };
           for (auto drawing_size { two }; drawing_size <= R; ++drawing_size)
           {
             combination selection { feature_indices,drawing_size };
             do
             {
               auto indices { selection() };
               for (auto index { zero };index < indices.size(); ++index)
               {
                  and_combine(result.at(feature_index),N_features.at(indices.at(index)));
               }
               ++feature_index;
             } while (selection.next());
           }
           return result;   
         }
         [[nodiscard]] features_t initialize_ON_features()
         {
           if (R > F)
           {
             throw std::invalid_argument("R > F in feature_isolation_t::initialize_ON_features()");
           }
           auto result = features_t(sum_of_combinations(F,1,R) - 1,feature_t(S,false));
           std::vector<unsigned_integer> feature_indices(F);
           std::iota(feature_indices.begin(),feature_indices.end(),zero);
           auto feature_index { zero };
           for (auto drawing_size { two }; drawing_size <= R; ++drawing_size)
           {
             combination selection { feature_indices,drawing_size };
             do
             {
               auto indices { selection() };
               for (auto index { zero };index < indices.size(); ++index)
               {
                  or_combine(result.at(feature_index),N_features.at(indices.at(index)));
               }
               ++feature_index;
             } while (selection.next());
           }
           return result;   
         }
         [[nodiscard]] features_t initialize_ANF_features()
         {
           if (R > F)
           {
             throw std::invalid_argument("R > F in feature_isolation_t::initialize_ANF_features()");
           }
           auto result = features_t(sum_of_combinations(F,1) - 1,feature_t(S,false));
           std::vector<unsigned_integer> feature_indices(F);
           std::iota(feature_indices.begin(),feature_indices.end(),zero);
           auto feature_index { zero };
           for (auto drawing_size { one }; drawing_size < F; ++drawing_size)
           {
             combination selection { feature_indices,drawing_size };
             do
             {
               auto indices { selection() };
               for (auto index { zero };index < indices.size(); ++index)
               {
                 or_combine(result.at(feature_index),F_features.at(indices.at(index)));
               }
               feature_t not_features(S,false);
               for (auto index { zero }; index < F; ++index)
               {
                 if (std::find(indices.begin(),indices.end(),index) == indices.end())
                 {
                   or_combine(not_features,N_features.at(index));
                 }
               }
               and_combine(result.at(feature_index),not_features);
               ++feature_index;
             } while (selection.next());
           }
           return result;   
         }
         [[nodiscard]] features_t initialize_ONF_features()
         {
           if (R > F)
           {
             throw std::invalid_argument("R > F in feature_isolation_t::initialize_ANF_features()");
           }
           auto result = features_t(sum_of_combinations(F,1) - 1,feature_t(S,false));
           std::vector<unsigned_integer> feature_indices(F);
           std::iota(feature_indices.begin(),feature_indices.end(),zero);
           auto feature_index { zero };
           for (auto drawing_size { one }; drawing_size < F; ++drawing_size)
           {
             combination selection { feature_indices,drawing_size };
             do
             {
               auto indices { selection() };
               for (auto index { zero };index < indices.size(); ++index)
               {
                 or_combine(result.at(feature_index),F_features.at(indices.at(index)));
               }
               feature_t not_features(S,false);
               for (auto index { zero }; index < F; ++index)
               {
                 if (std::find(indices.begin(),indices.end(),index) == indices.end())
                 {
                   or_combine(not_features,N_features.at(index));
                 }
               }
               or_combine(result.at(feature_index),not_features);
               ++feature_index;
             } while (selection.next());
           }
           return result;   
         }
         [[nodiscard]] systems_t initialize_systems()
         {
            systems_t result(S,system_t { });
            if (C & feature_isolation::F)
            {
               for (const auto& feature : F)
               {
HIER GEHT ES ERSTMAL NICHT WEITER. ALLE FEATURES ZUSAMMEN MÜSSEN NOCH IN EINEN
GEMEINSAMEN CONTAINER. IDEAL WÄRE ES, WENN SIE DABEI INTERN LEXIKOGRAPHISCH
AUFSTEIGEND SORTIERT WÄREN. DAS IST WOHL ABER NURZU SCHAFFEN, WENN MAN SIE ALLE
IN EIN SET PACKT, WAS DEN SPEICHERBEDARF DANN LOCKER VERDOPPELT.
ANSONSTEN KÖNNTE MAN JEDEM FEATURE AUCH EINEN OFFSET ADDIEREN, DER SICH AUS DER
MENGE DER IN DER VORANGEHENDEN KATEGORIE BEFINDLICHEN FEATURES ERGIBT. DANN WÄRE
DIE ORDNUNG ABER NICHT MEHR SO, WIE ERWARTET.
               }
            }
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
         << " DF = " << fi.DF << " T = " << fi.T
         << " R = " << fi.R << " C = " << fi.C << std::endl;
      return os.str();
   }
}
#endif // FEATURE_ISOLATION
