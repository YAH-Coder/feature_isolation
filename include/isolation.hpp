/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#ifndef ISOLATION_HPP
#define ISOLATION_HPP

#include "feature_analysis.hpp"
#include "spl.hpp"
#include <algorithm>
#include <bitset>
#include <compare>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace feature_isolation
{

  constexpr size_t MAX_FEATURES = 1024;

  using feature_bitset_t = std::bitset<MAX_FEATURES>;
  using system_id_t      = unsigned_integer;
  using system_feature_map_t
      = std::unordered_map<system_id_t, feature_bitset_t>;

  template<typename T> std::vector<typename T::value_type> to_vector(const T& c)
  {
    std::vector<typename T::value_type> result {};
    result.reserve(c.size());
    for (const auto& e : c)
    {
      result.push_back(e);
    }
    return result;
  }

  template<typename T> std::set<typename T::value_type> to_set(const T& c)
  {
    std::set<typename T::value_type> result {};
    for (const auto& e : c)
    {
      result.insert(e);
    }
    return result;
  }

  template<typename T>
  [[nodiscard]] bool subset(const std::set<T>& s1, const std::set<T>& s2)
  {
    return std::includes(s1.begin(), s1.end(), s2.begin(), s2.end());
  }

  template<typename T>
  [[nodiscard]] bool proper_subset(const std::set<T>& s1, const std::set<T>& s2)
  {
    return subset(s1, s2) && s1.size() > s2.size();
  }

  struct difference_t
  {
      system_ids_t I, U;
  };

  std::strong_ordering operator<=> (const difference_t& lhs,
                                    const difference_t& rhs)
  {
    if (lhs.I < rhs.I or (lhs.I == rhs.I and lhs.U < rhs.U))
    {
      return std::strong_ordering::less;
    }
    if (lhs.I > rhs.I or (lhs.I == rhs.I and lhs.U > rhs.U))
    {
      return std::strong_ordering::greater;
    }
    return std::strong_ordering::equivalent;
  }

  bool operator== (const difference_t& lhs, const difference_t& rhs)
  {
    return lhs.I == rhs.I && lhs.U == rhs.U;
  }

  using differences_t = std::set<difference_t>;

  struct isolation_result_t
  {
      feature_ids_t features;
      differences_t differences;
  };

  class feature_isolation_t final
  {
    public:
      explicit feature_isolation_t(const std::string& filename)
          : m_spl { spl_processor_t { filename }.get_specification() }
          , m_features { m_spl.m_F, m_spl.m_max_rank, m_spl.m_categories }
      { }

      [[nodiscard]] difference_t partition(unsigned_integer feature_id) const;
      [[nodiscard]] feature_ids_t
          flatten_by_intersection(const system_ids_t& sids) const;
      [[nodiscard]] feature_ids_t
          flatten_by_union(const system_ids_t& sids) const;
      [[nodiscard]] feature_ids_t
          flattened_difference(const system_ids_t& i,
                               const system_ids_t& u) const;
      [[nodiscard]] isolation_result_t
          minimal_differences(unsigned_integer feature_id) const;
      [[nodiscard]] isolation_result_t
          some_difference(unsigned_integer feature_id) const;
      [[nodiscard]] feature_ids_t sid_to_fids(unsigned_integer sid) const;
      static void memorize_proper_difference_subset(differences_t&      md,
                                                    const system_ids_t& i,
                                                    const system_ids_t& u);
      [[nodiscard]] static bool proper_difference_subset(const system_ids_t& i,
                                                         const system_ids_t& u,
                                                         const difference_t& d);
      [[nodiscard]] static bool
                                proper_difference_subset(const difference_t& d1,
                                                         const difference_t& d2);
      static void               print_header(std::ostream& os,
                                             bool          all_differences = false,
                                             bool feature_names = false);
      void                      print(std::ostream&    os,
                                      unsigned_integer fid,
                                      bool             all_differences = false,
                                      bool feature_names = false) const;
      void                      print(std::ostream& os,
                                      bool          all_differences = false,
                                      bool feature_names = false) const;
      [[nodiscard]] std::string name(unsigned_integer fid) const;
      static void
          print(std::ostream& os, const system_ids_t& sids, char operation);
      static void print(std::ostream& os, const difference_t& d);

      const spl_specification_t m_spl;
      const feature_analysis_t  m_features;
    private:
      void explore_I_subsets(size_t                  index,
                             feature_bitset_t        current_intersection,
                             system_ids_t&           current_I,
                             const feature_bitset_t& target_features,
                             isolation_result_t&     result,
                             const std::vector<system_id_t>& I_vec,
                             const std::vector<system_id_t>& U_vec,
                             const system_feature_map_t& system_features) const;

      void explore_U_subsets(size_t                  index,
                             const feature_bitset_t& current_union,
                             system_ids_t&           current_U,
                             const feature_bitset_t& current_intersection,
                             const feature_bitset_t& target_features,
                             const system_ids_t&     current_I,
                             isolation_result_t&     result,
                             const std::vector<system_id_t>& U_vec,
                             const system_feature_map_t& system_features) const;
  };

  difference_t feature_isolation_t::partition(unsigned_integer feature_id) const
  {
    system_ids_t I, U;
    for (const auto& s : m_spl.m_available_systems)
    {
      if (m_features.m_systems.at(s - 1).all_features().contains(feature_id))
      {
        I.insert(s);
      }
      else
      {
        U.insert(s);
      }
    }
    return { I, U };
  }

  feature_ids_t feature_isolation_t::sid_to_fids(unsigned_integer sid) const
  {
    return m_features.m_systems.at(sid - 1).all_features();
  }

  feature_ids_t feature_isolation_t::flatten_by_intersection(
      const system_ids_t& sids) const
  {
    if (sids.size() == 0)
    {
      return {};
    }
    auto result { sid_to_fids(*sids.begin()) };
    if (sids.size() > 1)
    {
      for (auto it { ++sids.begin() }; it != sids.end(); ++it)
      {
        auto set1 { result };
        result.clear();
        auto set2 { sid_to_fids(*it) };
        /* result has been specified according to
         * https://stackoverflow.com/questions/71390905/why-cant-i-use-a-set-as-the-output-collection-of-transform-operation-on-a-vecto
         */
        std::ranges::set_intersection(
            set1, set2, std::insert_iterator { result, result.begin() });
      }
    }
    return result;
  }

  feature_ids_t
      feature_isolation_t::flatten_by_union(const system_ids_t& sids) const
  {
    if (sids.size() == 0)
    {
      return {};
    }
    auto result { sid_to_fids(*sids.begin()) };
    if (sids.size() > 1)
    {
      for (auto it { ++sids.begin() }; it != sids.end(); ++it)
      {
        auto set1 { result };
        result.clear();
        auto set2 { sid_to_fids(*it) };
        /* result has been specified according to
         * https://stackoverflow.com/questions/71390905/why-cant-i-use-a-set-as-the-output-collection-of-transform-operation-on-a-vecto
         */
        std::ranges::set_union(
            set1, set2, std::insert_iterator { result, result.begin() });
      }
    }
    return result;
  }

  feature_ids_t
      feature_isolation_t::flattened_difference(const system_ids_t& i,
                                                const system_ids_t& u) const
  {
    feature_ids_t result {};
    std::ranges::set_difference(
        flatten_by_intersection(i),
        flatten_by_union(u),
        std::insert_iterator { result, result.begin() });
    return result;
  }

  std::string d_f_to_string(const feature_ids_t& d_f)
  {
    std::ostringstream oss;
    oss << "\"";
    for (auto it = d_f.begin(); it != d_f.end(); ++it)
    {
      if (it != d_f.begin())
      {
        oss << ";";
      }
      oss << *it;
    }
    oss << "\"";
    return oss.str();
  }

  isolation_result_t feature_isolation_t::minimal_differences(
      unsigned_integer feature_id) const
  {
    isolation_result_t result;
    auto [I_set, U_set] = partition(feature_id);
    std::vector<system_id_t> I_vec(I_set.begin(), I_set.end());
    std::vector<system_id_t> U_vec(U_set.begin(), U_set.end());

    system_feature_map_t system_features;
    for (const auto& sid : m_spl.m_available_systems)
    {
      feature_bitset_t features_bitset;

      const auto& feature_ids = sid_to_fids(sid);
      for (auto fid : feature_ids)
      {
        features_bitset.set(fid - 1); // Adjust for zero-based indexing
      }

      system_features[sid] = features_bitset;
    }

    feature_bitset_t target_features;
    target_features.set(feature_id - 1);

    feature_bitset_t full_intersection;
    full_intersection.set();

    system_ids_t current_I;

    explore_I_subsets(0,
                      full_intersection,
                      current_I,
                      target_features,
                      result,
                      I_vec,
                      U_vec,
                      system_features);

    result.features.insert(feature_id);

    return result;
  }

  void feature_isolation_t::explore_I_subsets(
      size_t                          index,
      feature_bitset_t                current_intersection,
      system_ids_t&                   current_I,
      const feature_bitset_t&         target_features,
      isolation_result_t&             result,
      const std::vector<system_id_t>& I_vec,
      const std::vector<system_id_t>& U_vec,
      const system_feature_map_t&     system_features) const
  {
    // Base case
    if (index == I_vec.size())
    {
      if (!current_I.empty()
          && (current_intersection & target_features) == target_features)
      {
        // Proceed to explore U subsets
        feature_bitset_t empty_union;
        system_ids_t     current_U;
        explore_U_subsets(0,
                          empty_union,
                          current_U,
                          current_intersection,
                          target_features,
                          current_I,
                          result,
                          U_vec,
                          system_features);
      }
      return;
    }

    // Option 1: Include system at index
    system_id_t system_id = I_vec[index];

    // Update current_intersection appropriately
    feature_bitset_t new_intersection;
    if (current_I.empty())
    {
      new_intersection = system_features.at(system_id);
    }
    else
    {
      new_intersection = current_intersection & system_features.at(system_id);
    }

    // Prune if new intersection doesn't contain target features
    if ((new_intersection & target_features) == target_features)
    {
      current_I.insert(system_id);

      // Continue exploring further I subsets including this system
      explore_I_subsets(index + 1,
                        new_intersection,
                        current_I,
                        target_features,
                        result,
                        I_vec,
                        U_vec,
                        system_features);

      current_I.erase(system_id);
    }

    // Option 2: Exclude system at index
    explore_I_subsets(index + 1,
                      current_intersection,
                      current_I,
                      target_features,
                      result,
                      I_vec,
                      U_vec,
                      system_features);
  }

  void feature_isolation_t::explore_U_subsets(
      size_t                          index,
      const feature_bitset_t&         current_union,
      system_ids_t&                   current_U,
      const feature_bitset_t&         current_intersection,
      const feature_bitset_t&         target_features,
      const system_ids_t&             current_I,
      isolation_result_t&             result,
      const std::vector<system_id_t>& U_vec,
      const system_feature_map_t&     system_features) const
  {
    if (index >= U_vec.size())
    {
      // Check if the difference matches the target features
      feature_bitset_t diff = current_intersection & (~current_union);
      if (diff == target_features)
      {
        // Found a minimal difference
        memorize_proper_difference_subset(
            result.differences, current_I, current_U);
      }
      return;
    }

    // Option 1: Include system at index
    system_id_t      system_id = U_vec[index];
    feature_bitset_t new_union = current_union | system_features.at(system_id);

    // Prune if new union contains any target features
    if ((new_union & target_features).none())
    {
      current_U.insert(system_id);
      explore_U_subsets(index + 1,
                        new_union,
                        current_U,
                        current_intersection,
                        target_features,
                        current_I,
                        result,
                        U_vec,
                        system_features);
      current_U.erase(system_id);
    }

    // Option 2: Exclude system at index
    explore_U_subsets(index + 1,
                      current_union,
                      current_U,
                      current_intersection,
                      target_features,
                      current_I,
                      result,
                      U_vec,
                      system_features);
  }

  isolation_result_t
      feature_isolation_t::some_difference(unsigned_integer feature_id) const
  {
    auto [i_set, u_set] { partition(feature_id) };
    auto               i { to_vector(i_set) };
    isolation_result_t result;
    difference_t       difference;
    if (i.size())
    {
      result.features = sid_to_fids(i.at(0));
      difference.I.insert(i.at(0));
    }
    for (auto index { one }; index < i.size(); ++index)
    {
      difference.I.insert(i.at(index));
      auto temp { result.features };
      result.features.clear();
      std::ranges::set_intersection(
          temp,
          sid_to_fids(i.at(index)),
          std::insert_iterator { result.features, result.features.begin() });
      if (result.features.size() <= one)
      {
        result.differences.insert(difference);
        return result;
      }
    }
    auto u { to_vector(u_set) };
    if (u.size())
    {
      for (auto index { zero }; index < u.size(); ++index)
      {
        difference.U.insert(u.at(index));
        auto temp { result.features };
        result.features.clear();
        std::ranges::set_difference(
            temp,
            sid_to_fids(u.at(index)),
            std::insert_iterator { result.features, result.features.begin() });
        if (result.features.size() <= one)
        {
          result.differences.insert(difference);
          return result;
        }
      }
    }
    result.differences.insert(difference);
    return result;
  }

  bool feature_isolation_t::proper_difference_subset(const system_ids_t& i,
                                                     const system_ids_t& u,
                                                     const difference_t& d)
  {
    return proper_subset(i, d.I) or proper_subset(u, d.U);
  }

  bool feature_isolation_t::proper_difference_subset(const difference_t& d1,
                                                     const difference_t& d2)
  {
    return (subset(d1.I, d2.I) && d2.I.size() < d1.I.size())
           || (subset(d1.U, d2.U) && d2.U.size() < d1.U.size());
  }

  void feature_isolation_t::memorize_proper_difference_subset(
      differences_t& md, const system_ids_t& i, const system_ids_t& u)
  {
    difference_t new_diff { i, u };
    for (auto it = md.begin(); it != md.end();)
    {
      const auto& existing_diff = *it;
      if (proper_difference_subset(new_diff, existing_diff))
      {
        // New difference is a proper subset of existing difference; do not add
        // it
        return;
      }
      else if (proper_difference_subset(existing_diff, new_diff))
      {
        // Existing difference is a proper subset of new difference; remove it
        it = md.erase(it);
      }
      else
      {
        ++it;
      }
    }
    md.insert(new_diff);
  }

  void feature_isolation_t::print_header(std::ostream& os,
                                         bool          all_differences,
                                         bool          feature_names)
  {
    os << "FEATURE ID" << separator() << "FEATURE NAME" << separator()
       << "ISOLATION RESULT" << separator();
    if (all_differences)
    {
      os << "#MIN. DIFF" << separator() << "MINIMAL DIFFERENCES";
    }
    else
    {
      os << "SOME DIFFERENCE";
    }
    os << separator();
  }

  void feature_isolation_t::print(std::ostream&    os,
                                  unsigned_integer fid,
                                  bool             all_differences,
                                  bool             feature_names) const
  {
    auto result { all_differences ? minimal_differences(fid)
                                  : some_difference(fid) };
    os << fid << separator() << name(fid) << separator() << "{ ";
    for (auto counter { zero }; auto fid : result.features)
    {
      os << (feature_names ? name(fid) : std::to_string(fid))
         << (++counter < result.features.size() ? ", " : "");
    }
    os << " }" << separator();
    if (all_differences)
    {
      os << result.differences.size() << separator();
    }
    for (auto counter { zero }; const auto& difference : result.differences)
    {
      print(os, difference);
      os << (++counter << result.differences.size()
                 ? std::string { separator() }
                 : "");
    }
    os << std::endl;
  }

  bool feature_present(const std::vector<bool>& anf_feature,
                       const std::vector<bool>& f_feature,
                       unsigned_integer         F)
  {
    auto counter { zero };
    auto half { F / 2 };
    std::cout << "F = " << F << " F/2 = " << half << std::endl;
    for (auto index { zero }; index < anf_feature.size(); ++index)
    {
      if (anf_feature[index] && f_feature[index])
      {
        if (++counter > half)
        {
          std::cout << counter << std::endl;
          return true;
        }
      }
    }
    return false;
  }

  bool feature_and(const std::vector<bool>& and_feature,
                   const std::vector<bool>& f_feature)
  {
    for (auto index { zero }; index < and_feature.size(); ++index)
    {
      if (and_feature[index] && !f_feature[index])
      {
        return false;
      };
    }
    return true;
  }

  bool feature_or(const std::vector<bool>& or_feature,
                  const std::vector<bool>& f_feature)
  {
    for (auto index { zero }; index < f_feature.size(); ++index)
    {
      if (f_feature[index] && !or_feature[index])
      {
        return false;
      };
    }
    return true;
  }

  bool feature_and_not(const std::vector<bool>& and_not_feature,
                       const std::vector<bool>& f_feature)
  {
    for (auto index { zero }; index < and_not_feature.size(); ++index)
    {
      if (and_not_feature[index] && f_feature[index])
      {
        return false;
      };
    }
    return true;
  }

  bool feature_or_not(const std::vector<bool>& or_not_feature,
                      const std::vector<bool>& f_feature)
  {
    for (auto index { zero }; index < f_feature.size(); ++index)
    {
      if (!f_feature[index] && !or_not_feature[index])
      {
        return false;
      };
    }
    return true;
  }

  std::vector<bool> operator| (std::vector<bool>        v1,
                               const std::vector<bool>& v2)
  {
    if (v1.size() != v2.size())
    {
      throw std::range_error(
          "operator |(): bool vectors have different sizes!");
    }
    for (auto i { zero }; i < v1.size(); ++i)
    {
      v1[i] = v1[i] || v2[i];
    }
    return v1;
  }

  std::vector<bool> operator& (std::vector<bool>        v1,
                               const std::vector<bool>& v2)
  {
    if (v1.size() != v2.size())
    {
      throw std::range_error(
          "operator &(): bool vectors have different sizes!");
    }
    for (auto i { zero }; i < v1.size(); ++i)
    {
      v1[i] = v1[i] && v2[i];
    }
    return v1;
  }

  std::vector<bool> operator~(std::vector<bool> v)
  {
    v.flip();
    return v;
  }

  auto operator== (const std::vector<bool>& v1, const std::vector<bool>& v2)
  {
    if (v1.size() != v2.size())
    {
      return false;
    }
    for (auto i { zero }; i < v1.size(); ++i)
    {
      if (v1[i] != v2[i])
      {
        return false;
      };
    }
    return true;
  }

  unsigned_integer dot_product(const std::vector<bool>& v)
  {
    return static_cast<unsigned_integer>(std::ranges::count(v, true));
  }

  bool feature_bool_and(const std::vector<bool>& f1,
                        const std::vector<bool>& f2)
  {
    for (auto index { zero }; index < f1.size(); ++index)
    {
      if (f1[index] && f2[index])
      {
        return true;
      };
    }
    return false;
  }

  std::string feature_isolation_t::name(unsigned_integer fid) const
  {
    auto        cat { one << m_features.category(fid) };
    bool        first { true };
    auto        f_desc { m_features.at(fid) };
    std::string result {};
    switch (cat)
    {
      case ::feature_isolation::F :
        return m_spl.m_feature_names.at(fid - one);
      case ::feature_isolation::N :
        return "!" + m_spl.m_feature_names.at(fid - m_features.F - one);
      case ::feature_isolation::A :
        for (auto index { zero }; const auto& f : m_features.F_features)
        {
          if (feature_and(f_desc, f))
          {
            if (first)
            {
              result = m_spl.m_feature_names.at(index);
              first  = false;
            }
            else
            {
              result += " * " + m_spl.m_feature_names.at(index);
            }
          }
          ++index;
        }
        return result;
      case ::feature_isolation::O :
        for (auto index { zero }; const auto& f : m_features.F_features)
        {
          if (feature_or(f_desc, f))
          {
            if (first)
            {
              result = m_spl.m_feature_names.at(index);
              first  = false;
            }
            else
            {
              result += " + " + m_spl.m_feature_names.at(index);
            }
          }
          ++index;
        }
        return result;
      case ::feature_isolation::AN :
        for (auto index { zero }; const auto& f : m_features.F_features)
        {
          if (feature_and_not(f_desc, f))
          {
            if (first)
            {
              result = "!" + m_spl.m_feature_names.at(index);
              first  = false;
            }
            else
            {
              result += " * !" + m_spl.m_feature_names.at(index);
            }
          }
          ++index;
        }
        return result;
      case ::feature_isolation::ON :
        for (auto index { zero }; const auto& f : m_features.F_features)
        {
          if (feature_or_not(f_desc, f))
          {
            if (first)
            {
              result = "!" + m_spl.m_feature_names.at(index);
              first  = false;
            }
            else
            {
              result += " + !" + m_spl.m_feature_names.at(index);
            }
          }
          ++index;
        }
        return result;
      case ::feature_isolation::ANF :
        // The following is the implementation of a heuristic
        // which works well for correct ANF features. It does
        // not deal correctly with features that aren't ANF features!
        for (auto index { zero }; const auto& f : m_features.F_features)
        {
          if (feature_bool_and(f_desc, f))
          {
            if (first)
            {
              result = m_spl.m_feature_names.at(index);
              first  = false;
            }
            else
            {
              result += " * " + m_spl.m_feature_names.at(index);
            }
          }
          else
          {
            if (first)
            {
              result = "!" + m_spl.m_feature_names.at(index);
              first  = false;
            }
            else
            {
              result += " * !" + m_spl.m_feature_names.at(index);
            }
          }
          ++index;
        }
        return result;
      case ::feature_isolation::ONF :
        for (auto index { zero }; const auto& f : m_features.F_features)
        {
          if (feature_or(f_desc, f))
          {
            if (first)
            {
              result += m_spl.m_feature_names.at(index);
              first   = false;
            }
            else
            {
              result += " + " + m_spl.m_feature_names.at(index);
            }
          }
          else if (feature_or_not(f_desc, f))
          {
            if (first)
            {
              result += "!" + m_spl.m_feature_names.at(index);
              first   = false;
            }
            else
            {
              result += " + !" + m_spl.m_feature_names.at(index);
            }
          }
          ++index;
        }
        return result;
    }
    return "Illegal feature id in feature_isolation_t::name()";
  }

  void feature_isolation_t::print(std::ostream& os,
                                  bool          all_differences,
                                  bool          feature_names) const
  {
    print_header(os, all_differences, feature_names);
    os << std::endl;
    for (auto fid { one }; fid <= m_features.T; ++fid)
    {
      print(os, fid, all_differences, feature_names);
    }
  }

  void feature_isolation_t::print(std::ostream&       os,
                                  const system_ids_t& sids,
                                  char                operation)
  {
    std::string op { operation };
    os << "{";
    for (auto i { zero }; auto id : sids)
    {
      os << ' ' << id << ' ' << (++i < sids.size() ? op : "");
    }
    os << "}";
  }

  void feature_isolation_t::print(std::ostream&       os,
                                  const difference_t& difference)
  {
    print(os, difference.I, '&');
    os << " \\ ";
    print(os, difference.U, '|');
  }
} // namespace feature_isolation
#endif // ISOLATION_HPP
