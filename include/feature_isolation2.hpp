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
   constexpr bool MSB_LSB { true }; // Customization point for MSB at
                                    // left-most position.
   constexpr unsigned_integer feature_digits { sizeof(unsigned_integer) * CHAR_BIT };

   using feature_t = std::vector<bool>;
   using features_t = std::vector<feature_t>;
   using feature_ids_t = std::set<unsigned_integer>;

   class system_t
   {
     public:
       feature_ids_t F_feature_ids,
                     N_feature_ids,
                     A_feature_ids,
                     O_feature_ids,
                     AN_feature_ids,
                     ON_feature_ids,
                     ANF_feature_ids,
                     ONF_feature_ids;
      [[nodiscard]] auto DF() const
      {
        return N_feature_ids.size() + 
               A_feature_ids.size() + O_feature_ids.size() +
               AN_feature_ids.size() + ON_feature_ids.size() +
               ANF_feature_ids.size() + ONF_feature_ids.size();
      }
      [[nodiscard]] auto T() const
      {
        return DF() + F_feature_ids.size();
      }
      auto all_features() const
      {
        auto result { F_feature_ids };
        for (auto f : N_feature_ids)
        {
          result.insert(f);
        }
        for (auto f : A_feature_ids)
        {
          result.insert(f);
        }
        for (auto f : O_feature_ids)
        {
          result.insert(f);
        }   
        for (auto f : AN_feature_ids)
        {
          result.insert(f);
        }   
        for (auto f : ON_feature_ids)
        {
          result.insert(f);
        }   
        for (auto f : ANF_feature_ids)
        {
          result.insert(f);
        }   
        for (auto f : ONF_feature_ids)
        {
          result.insert(f);
        }   
        return result;
      }
   };
   using systems_t = std::vector<system_t>;

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

   class feature_isolation_t; // Pending declaration.
   std::string to_string(const system_t& s,const feature_isolation_t& fi);

   std::string to_string(const systems_t& s,const feature_isolation_t& fi)
   {
      std::string result { };
      for (const auto& system: s)
      {
         result += to_string(system,fi) + "\n";
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
         [[nodiscard]] feature_t at(unsigned_integer id) const
         {
            if (id < F)
            {
               return F_features.at(id);
            }
            if (id < N)
            {
               return N_features.at(id - F);
            }
            if (id < A)
            {
               return A_features.at(id - F - N);
            }
            if (id < O)
            {
               return O_features.at(id - F - N - A);
            }
            if (id < AN)
            {
               return AN_features.at(id - F - N - A - O);
            }
            if (id < ON)
            {
               return ON_features.at(id - F - N - A - O - AN);
            }
            if (id < ANF)
            {
               return ANF_features.at(id - F - N - A - O - AN - ON);
            }
            return ONF_features.at(id - F - N - A - O - AN - ON - ANF);
        }
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
         void insert_id(const feature_t feature,unsigned_integer feature_id,unsigned_integer category)
         {
           for (auto system_id { zero };auto system : feature)
           {
             if (system)
             {
               switch (category)
               {
                 case ::feature_isolation::F : m_systems.at(system_id).F_feature_ids.insert(feature_id);
                 case ::feature_isolation::N : m_systems.at(system_id).N_feature_ids.insert(feature_id);
                 case ::feature_isolation::A : m_systems.at(system_id).A_feature_ids.insert(feature_id);
                 case ::feature_isolation::O : m_systems.at(system_id).O_feature_ids.insert(feature_id);
                 case ::feature_isolation::AN : m_systems.at(system_id).AN_feature_ids.insert(feature_id);
                 case ::feature_isolation::ON : m_systems.at(system_id).ON_feature_ids.insert(feature_id);
                 case ::feature_isolation::ANF : m_systems.at(system_id).ANF_feature_ids.insert(feature_id);
                 case ::feature_isolation::ONF : m_systems.at(system_id).ONF_feature_ids.insert(feature_id);
               }
             }
           }
         }
         [[nodiscard]] systems_t initialize_systems()
         {
           systems_t result(S,system_t { });
           auto feature_id { zero };
           if (C & feature_isolation::F)
           {
             for (const auto& feature : F_features)
             {
               insert_id(feature,feature_id++,::feature_isolation::F);
             }
           }
           if (C & feature_isolation::N)
           {
             for (const auto& feature : N_features)
             {
               insert_id(feature,feature_id++,::feature_isolation::N);
             }
           }
           if (C & feature_isolation::A)
           {
             for (const auto& feature : A_features)
             {
               insert_id(feature,feature_id++,::feature_isolation::A);
             }
           }
           if (C & feature_isolation::O)
           {
             for (const auto& feature : O_features)
             {
               insert_id(feature,feature_id++,::feature_isolation::O);
             }
           }
           if (C & feature_isolation::AN)
           {
             for (const auto& feature : AN_features)
             {
               insert_id(feature,feature_id++,::feature_isolation::AN);
             }
           }
           if (C & feature_isolation::ON)
           {
             for (const auto& feature : ON_features)
             {
               insert_id(feature,feature_id++,::feature_isolation::ON);
             }
           }
           if (C & feature_isolation::ANF)
           {
             for (const auto& feature : ANF_features)
             {
               insert_id(feature,feature_id++,::feature_isolation::ANF);
             }
           }
           if (C & feature_isolation::ONF)
           {
             for (const auto& feature : ONF_features)
             {
               insert_id(feature,feature_id++,::feature_isolation::ONF);
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

  std::string to_string(const system_t& s,const feature_isolation_t& fi)
  {
    std::string result { "{ " };
    for (const auto& i : s.all_features())
    {
      result += std::to_string(to_unsigned_integer(fi.at(i))) + " ";
    }
    result += "}";
    return result;
  }
}
#endif // FEATURE_ISOLATION
