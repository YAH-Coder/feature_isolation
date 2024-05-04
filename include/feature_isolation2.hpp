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
     [[nodiscard]] auto T() const;
     auto all_features() const;
  };
  using systems_t = std::vector<system_t>;

  class feature_isolation_t; // Pending declaration.

  std::string to_string(const feature_t& fd);

  //These constants are used as bit masks for feature categories.
  // Make these constexpr or is there a reason not to?
  static const unsigned_integer F { one };
  static const unsigned_integer N { feature_isolation::power2( 2 ) };
  static const unsigned_integer A { feature_isolation::power2( 3 ) };
  static const unsigned_integer O { feature_isolation::power2( 4 ) };
  static const unsigned_integer FN { F + N };
  static const unsigned_integer AN { feature_isolation::power2( 5 ) };
  static const unsigned_integer ON { feature_isolation::power2( 6 ) };
  static const unsigned_integer ANF { feature_isolation::power2( 7 ) };
  static const unsigned_integer ONF { feature_isolation::power2( 8 ) };
  static const unsigned_integer FAO { F + A + O };
  static const unsigned_integer FAON { FAO + N };
  static const unsigned_integer FAONANON { FAON + AN + ON };
  static const unsigned_integer ALL { FAONANON + ANF + ONF };

  //This array holds the names of feature categories.
  // But why is it a raw array instead of a std::array or std::vector?
  static const std::string feature_categories[] = {"F","N","A","O","AN","ON","ANF","ONF" };

   class feature_isolation_t
   {
      public:
         explicit feature_isolation_t(unsigned_integer f,
                                      unsigned_integer max_rank,
                                      unsigned_integer feature_categories = ::feature_isolation::ALL,
                                      char separator = '\t')
            : F { f },
              N { F },
              A { sum_of_combinations( F, two ) },
              O { A },
              AN { O },
              ON { O },
              ANF { sum_of_combinations( F, 1 ) - 1 },
              ONF { ANF },
              DF { f + 4 * A + 2 * ANF },
              T { F + DF },
              R { max_rank },
              C { feature_categories },
              S { power2( F ) },
              F_features { initialize_F_features() },
              N_features { initialize_N_features() },
              A_features { initialize_A_features() },
              O_features { initialize_O_features() },
              AN_features { initialize_AN_features() },
              ON_features { initialize_ON_features() },
              ANF_features { initialize_ANF_features() },
              ONF_features { initialize_ONF_features() },
              F_offset { zero },
              N_offset { F_offset + F_offset },
              A_offset { N_offset + N },
              O_offset { A_offset + A },
              AN_offset { O_offset + O },
              ON_offset { AN_offset + AN },
              ANF_offset { ON_offset + ON },
              ONF_offset { ANF_offset + ANF },
              feature_offsets { F_offset,N_offset,A_offset,O_offset,AN_offset,ON_offset,ANF_offset,ONF_offset },
              all_features { &F_features,&N_features,&A_features,&O_features,&AN_features,&ON_features,&ANF_features,&ONF_features },
              m_systems { initialize_systems() },
              m_separator { separator }
         {};
         using cui = const unsigned_integer;
         cui F,
             N,
             A,
             O,
             AN,
             ON,
             ANF,
             ONF,
             DF,
             T,
             R,
             C,
             S;
         const features_t F_features,
                          N_features,
                          A_features,
                          O_features,
                          AN_features,
                          ON_features,
                          ANF_features,
                          ONF_features;
         cui F_offset,
             N_offset,
             A_offset,
             O_offset,
             AN_offset,
             ON_offset,
             ANF_offset,
             ONF_offset;
         cui feature_offsets[8];
         const features_t* all_features[8];
         const systems_t m_systems;
         char m_separator;
         [[nodiscard]] feature_t at(unsigned_integer id) const;
         void print_feature(std::ostream& os,const feature_t& feature) const;
         void print_features(std::ostream& os) const;
         void print_categories(std::ostream& os) const;
         // DELETE void print_system_feature_ids(std::ostream& os,unsigned_integer category,const feature_ids_t& feature_ids) const;
         void print_system_statistics(std::ostream& os,const system_t& system,unsigned_integer id) const;
         void print_systems_statistics(std::ostream& os) const;
         void print_system_features(std::ostream& os,const system_t& system,unsigned_integer id) const;
         void print_systems_features(std::ostream& os) const;
         void print(std::ostream& os) const;
      private:
         [[nodiscard]] features_t initialize_F_features();
         [[nodiscard]] features_t initialize_N_features();
         void and_combine(feature_t& f1,const feature_t& f2);
         [[nodiscard]] features_t initialize_A_features();
         void or_combine(feature_t& f1,const feature_t& f2);
         [[nodiscard]] features_t initialize_O_features();
         [[nodiscard]] features_t initialize_AN_features();
         [[nodiscard]] features_t initialize_ON_features();
         [[nodiscard]] features_t initialize_ANF_features();
         [[nodiscard]] features_t initialize_ONF_features();
         void insert_id(systems_t& systems,const feature_t feature,unsigned_integer feature_id,unsigned_integer category);
         [[nodiscard]] systems_t initialize_systems();
  };

// ====================== system_t - member functions ====================== //
   auto system_t::all_features() const
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
   [[nodiscard]] auto system_t::T() const
   {
     return DF() + F_feature_ids.size();
   }

// ====================== feature_isolation_t - member functions ====================== //
  [[nodiscard]] feature_t feature_isolation_t::at(unsigned_integer id) const
  // This method has lots of lines and is potentially inefficient.
  // Remedies?

  //No remedies, just the question if this description is right:
  // The function feature_isolation_t::at is used to retrieve a feature based on a given id.
  // It starts with an index of zero and iterates through the feature_offsets array.
  // During each iteration, it checks if the id is less than the feature offset at the current index.
  // If it is, it increments the index and continues to the next iteration.
  // If it isn't, it breaks out of the loop and returns the feature at the corresponding index in the all_features array.
  {
    auto index { zero };
    while (id < feature_offsets[index])
    {
       ++index;
    }
    return all_features[index]->at(feature_offsets[index]);
/*     if (id < F)
     {
        return F_features.at(id);
     }
     if (id < F + N)
     {
        return N_features.at(id - F);
     }
     if (id < F + N + A)
     {
        return A_features.at(id - F - N);
     }
     if (id < F + N + A + O)
     {
        return O_features.at(id - F - N - A);
     }
     if (id < F + N + A + O + AN)
     {
        return AN_features.at(id - F - N - A - O);
     }
     if (id < F + N + A + O + AN + ON)
     {
        return ON_features.at(id - F - N - A - O - AN);
     }
     if (id < F + N + A + O + AN + ON + ANF)
     {
        return ANF_features.at(id - F - N - A - O - AN - ON);
     }
     return ONF_features.at(id - F - N - A - O - AN - ON - ANF);*/
  }
  void feature_isolation_t::print_feature(std::ostream& os,const feature_t& feature) const
  {
     os << to_string(feature);
  }
  void feature_isolation_t::print_features(std::ostream& os) const
  {
    os << "FEATURE ID" << m_separator
       << "CATEGORY" << m_separator
       << "DESCRIPTION" << std::endl;
    for (auto index { zero }; index < 8; ++index)
    {
      if (C & (1 << index))
      {
        for (auto id { zero };const auto& feature:*(all_features[index]))
        {
          os << (++id + feature_offsets[index]) << m_separator
             << feature_categories[index] << m_separator << '\"';
          print_feature(os,feature);
          os <<'\"' << std::endl;
        }
     }
   }
/*     os << "FEATURE ID" << m_separator
        << "CATEGORY" << m_separator
        << "DESCRIPTION" << std::endl;
     for (auto id { zero };const auto& feature:F_features)
     {
        os << ++id << m_separator << "F" << m_separator << '\"';
        print_feature(os,feature);
        os <<'\"' << std::endl;
     }*/
  }
  void feature_isolation_t::print_categories(std::ostream& os) const
  {
    os << '\"'
       << ((C & ::feature_isolation::F) ? "F" : "-") << '|'
       << ((C & ::feature_isolation::N) ? "N" : "-") << '|'
       << ((C & ::feature_isolation::A) ? "A" : "-") << '|'
       << ((C & ::feature_isolation::O) ? "O" : "-") << '|'
       << ((C & ::feature_isolation::AN) ? "AN" : "--") << '|'
       << ((C & ::feature_isolation::ON) ? "ON" : "--") << '|'
       << ((C & ::feature_isolation::ANF) ? "ANF" : "---") << '|'
       << ((C & ::feature_isolation::ONF) ? "ONF" : "---")
       << '\"';
  }
/*DELETE  void feature_isolation_t::print_system_feature_ids(std::ostream& os,unsigned_integer category,const feature_ids_t& feature_ids) const
  {
    if (category > ::feature_isolation::ONF)
    {
      throw std::invalid_argument("category > ::feature_isolation::F in feature_isolation_t::print_system_feature_ids()");
    }
    os << m_separator << feature_categories[category] << " IDs" << std::endl;
    for (unsigned counter { 0 };const auto& id : feature_ids)
    {
       os << m_separator << (id);
       if (++counter == 5)
       {
          os << std::endl;
          counter = 0;
       }
    }
  }*/
  void feature_isolation_t::print_system_statistics(std::ostream& os,const system_t& system,unsigned_integer id) const
  {
/*DELETE    os << "SYSTEM ID" << m_separator << id
       << "F" << system.F_feature_ids.size() << m_separator
       << "N" << system.N_feature_ids.size() << m_separator
       << "A" << system.A_feature_ids.size() << m_separator
       << "O" << system.O_feature_ids.size() << m_separator
       << "AN" << system.AN_feature_ids.size() << m_separator
       << "ON" << system.ON_feature_ids.size() << m_separator
       << "ANF" << system.ANF_feature_ids.size() << m_separator
       << "ONF" << system.ONF_feature_ids.size() << std::endl;*/
       os << id << m_separator << system.F_feature_ids.size() << m_separator
          << system.N_feature_ids.size() << m_separator
          << system.A_feature_ids.size() << m_separator
          << system.O_feature_ids.size() << m_separator
          << system.AN_feature_ids.size() << m_separator
          << system.ON_feature_ids.size() << m_separator
          << system.ANF_feature_ids.size() << m_separator
          << system.ONF_feature_ids.size() << m_separator
          << system.DF() << m_separator
          << (system.DF() + system.F_feature_ids.size()) << std::endl; ;
          //DELETE print_system_feature_ids(os,::feature_isolation::F,system.F_feature_ids);
  }
  void feature_isolation_t::print_systems_statistics(std::ostream& os) const
  {
    os << "SYSTEM ID" << m_separator
       << "F" << m_separator
       << "N" << m_separator
       << "A" << m_separator
       << "O" << m_separator
       << "AN" << m_separator
       << "ON" << m_separator
       << "ANF" << m_separator
       << "ONF" << m_separator
       << "DF" << m_separator
       << "T" << std::endl;
     for (auto id { zero };const auto& system : m_systems)
     {
        print_system_statistics(os,system,++id);
        //DELETE os << std::endl;
     }
  }
  void feature_isolation_t::print_system_features(std::ostream& os,const system_t& system,unsigned_integer id) const
  {
    auto counter { zero };
    //DELETE os << m_separator;
    for (auto feature_id : system.all_features())
    {
       if (++counter == 10)
       {
          os << std::endl << m_separator;
          counter = 0;
       }
       os << m_separator << (feature_id + 1);
    }
  }
  void feature_isolation_t::print_systems_features(std::ostream& os) const
  {
     for (auto id { zero };const auto& system : m_systems)
     {
        os << "SYSTEM ID" << m_separator << ++id;
        print_system_features(os,system,id);
        os << std::endl;
     }
  }
  void feature_isolation_t::print(std::ostream& os) const
  {
    os << m_separator
       << "F" << m_separator
       << "N" << m_separator
       << "A" << m_separator
       << "O" << m_separator
       << "AN" << m_separator
       << "ON"<< m_separator
       << "ANF" << m_separator
       << "ONF" << m_separator
       << "DF" << m_separator
       << "T" << m_separator
       << "S" << m_separator
       << "R" << m_separator
       << "C" << m_separator
       << "\nIN PRINCIPLE" << m_separator
       << F << m_separator
       << N << m_separator
       << A << m_separator
       << O << m_separator
       << AN << m_separator
       << ON << m_separator
       << ANF << m_separator
       << ONF << m_separator
       << DF << m_separator
       << T << m_separator
       << S << m_separator
       << F << m_separator
       << "F|N|A|O|AN|ON|ANF|ONF";
    auto actual_df { N_features.size() + A_features.size() +
                     O_features.size() + AN_features.size() +
                     ON_features.size() + ANF_features.size() +
                     ONF_features.size() };
    os << "\nACTUALLY" << m_separator
       << F_features.size() << m_separator
       << N_features.size() << m_separator
       << A_features.size() << m_separator
       << O_features.size() << m_separator
       << AN_features.size() << m_separator
       << ON_features.size() << m_separator
       << ANF_features.size() << m_separator
       << ONF_features.size() << m_separator
       << actual_df << m_separator
       << (F_features.size() + actual_df) << m_separator
       << S << m_separator
       << R << m_separator;
       print_categories(os);
       os << std::endl;
  }
  [[nodiscard]] features_t feature_isolation_t::initialize_F_features()
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
  [[nodiscard]] features_t feature_isolation_t::initialize_N_features()
  {
    features_t result { F_features };
    for (auto& feature : result)
    feature.flip();
    return result;
  }
  void feature_isolation_t::and_combine(feature_t& f1,const feature_t& f2)
  {
    for (auto index { zero }; index < f1.size(); ++index)
    {
      f1.at(index) = f1.at(index) && f2.at(index);
    }
  }
  [[nodiscard]] features_t feature_isolation_t::initialize_A_features()
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
  void feature_isolation_t::or_combine(feature_t& f1,const feature_t& f2)
  {
    for (auto index { zero }; index < f1.size(); ++index)
    {
      f1.at(index) = f1.at(index) || f2.at(index);
    }
  }
  [[nodiscard]] features_t feature_isolation_t::initialize_O_features()
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
  [[nodiscard]] features_t feature_isolation_t::initialize_AN_features()
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
  [[nodiscard]] features_t feature_isolation_t::initialize_ON_features()
  {
    if (R > F)
    {
      throw std::invalid_argument("R > F in feature_isolation_t::initialize_ON_features()");
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
           or_combine(result.at(feature_index),N_features.at(indices.at(index)));
        }
        ++feature_index;
      } while (selection.next());
    }
    return result;
  }
  [[nodiscard]] features_t feature_isolation_t::initialize_ANF_features()
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
  [[nodiscard]] features_t feature_isolation_t::initialize_ONF_features()
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
  void feature_isolation_t::insert_id(systems_t& systems,const feature_t feature,unsigned_integer feature_id,unsigned_integer category)
  // This method is potentially inefficient.
  // Remedies?

  // Only Idea would be to use a map instead of the switch case statement
  {
    for (auto system_id { zero };auto system : feature)
    {
      if (system)
      {
        switch (category)
        {
          case ::feature_isolation::F : systems.at(system_id).F_feature_ids.insert(feature_id);break;
          case ::feature_isolation::N : systems.at(system_id).N_feature_ids.insert(feature_id);break;
          case ::feature_isolation::A : systems.at(system_id).A_feature_ids.insert(feature_id);break;
          case ::feature_isolation::O : systems.at(system_id).O_feature_ids.insert(feature_id);break;
          case ::feature_isolation::AN : systems.at(system_id).AN_feature_ids.insert(feature_id);break;
          case ::feature_isolation::ON : systems.at(system_id).ON_feature_ids.insert(feature_id);break;
          case ::feature_isolation::ANF : systems.at(system_id).ANF_feature_ids.insert(feature_id);break;
          case ::feature_isolation::ONF : systems.at(system_id).ONF_feature_ids.insert(feature_id);break;
        }
      }
      ++system_id;
    }
  }
  [[nodiscard]] systems_t feature_isolation_t::initialize_systems()
  // This method has many lines and is potentially inefficient.
  // Remedies?

  // Could we define a vector of pairs, where each pair contains a feature set and its corresponding feature type
  // Then we could loop over that and could check for each pair if it is included in C and then add it to result?
  /*Something like this:
  [[nodiscard]] systems_t feature_isolation_t::initialize_systems() {
    systems_t result(S,system_t { });

    std::vector<std::pair<std::vector<feature_t>, feature_isolation>> feature_sets = {
        {F_features, feature_isolation::F},
        {N_features, feature_isolation::N},
        {A_features, feature_isolation::A},
        {O_features, feature_isolation::O},
        {AN_features, feature_isolation::AN},
        {ON_features, feature_isolation::ON},
        {ANF_features, feature_isolation::ANF},
        {ONF_features, feature_isolation::ONF}
    };

    auto feature_id { zero };

    for (const auto& [features, feature_type] : feature_sets) {
        if (C & feature_type) {
            for (const auto& feature : features) {
                insert_id(result, feature, feature_id++, feature_type);
            }
        }
    }

    return result;
    }*/
  {
    systems_t result(S,system_t { });
    auto feature_id { zero };
    if (C & feature_isolation::F)
    {
      for (const auto& feature : F_features)
      {
        insert_id(result,feature,feature_id++,::feature_isolation::F);
      }
    }
    feature_id = F;
    if (C & feature_isolation::N)
    {
      for (const auto& feature : N_features)
      {
        insert_id(result,feature,feature_id++,::feature_isolation::N);
      }
    }
    feature_id = F + N;
    if (C & feature_isolation::A)
    {
      for (const auto& feature : A_features)
      {
        insert_id(result,feature,feature_id++,::feature_isolation::A);
      }
    }
    feature_id = F + N + A;
    if (C & feature_isolation::O)
    {
      for (const auto& feature : O_features)
      {
        insert_id(result,feature,feature_id++,::feature_isolation::O);
      }
    }
    feature_id = F + N + A + O;
    if (C & feature_isolation::AN)
    {
      for (const auto& feature : AN_features)
      {
        insert_id(result,feature,feature_id++,::feature_isolation::AN);
      }
    }
    feature_id = F + N + A + O + AN;
    if (C & feature_isolation::ON)
    {
      for (const auto& feature : ON_features)
      {
        insert_id(result,feature,feature_id++,::feature_isolation::ON);
      }
    }
    feature_id = F + N + A + O + AN + ON;
    if (C & feature_isolation::ANF)
    {
      for (const auto& feature : ANF_features)
      {
        insert_id(result,feature,feature_id++,::feature_isolation::ANF);
      }
    }
    feature_id = F + N + A + O + AN + ON + ANF;
    if (C & feature_isolation::ONF)
    {
      for (const auto& feature : ONF_features)
      {
        insert_id(result,feature,feature_id++,::feature_isolation::ONF);
      }
    }
    return result;
  }

// ====================== free functions ====================== //
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
}
#endif // FEATURE_ISOLATION
