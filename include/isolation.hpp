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
#include <compare>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace feature_isolation
{

  using system_id_t = unsigned_integer;
  using system_feature_map_t
      = std::unordered_map<system_id_t, feature_ids_t>;

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

  [[nodiscard]] bool subset(const feature_ids_t& s1, const feature_ids_t& s2)
  {
    return feature_ids_t::subset_of(s2, s1);
  }

  template<typename T>
  [[nodiscard]] bool proper_subset(const std::set<T>& s1, const std::set<T>& s2)
  {
    return subset(s1, s2) && s1.size() > s2.size();
  }

  [[nodiscard]] bool proper_subset(const feature_ids_t& s1, const feature_ids_t& s2)
  {
    return subset(s1, s2) && s1.size() > s2.size();
  }

  [[nodiscard]] feature_ids_t intersection_of(const feature_ids_t& lhs,
                                              const feature_ids_t& rhs)
  {
    return feature_ids_t::intersection_of(lhs, rhs);
  }

  template<typename T>
  [[nodiscard]] T intersection_of(const T& lhs, const T& rhs)
  {
    T result {};
    std::ranges::set_intersection(
        lhs, rhs, std::insert_iterator { result, result.begin() });
    return result;
  }

  [[nodiscard]] feature_ids_t union_of(const feature_ids_t& lhs,
                                       const feature_ids_t& rhs)
  {
    return feature_ids_t::union_of(lhs, rhs);
  }

  template<typename T> [[nodiscard]] T union_of(const T& lhs, const T& rhs)
  {
    T result {};
    std::ranges::set_union(
        lhs, rhs, std::insert_iterator { result, result.begin() });
    return result;
  }

  [[nodiscard]] feature_ids_t difference_of(const feature_ids_t& lhs,
                                            const feature_ids_t& rhs)
  {
    return feature_ids_t::difference_of(lhs, rhs);
  }

  template<typename T>
  [[nodiscard]] T difference_of(const T& lhs, const T& rhs)
  {
    T result {};
    std::ranges::set_difference(
        lhs, rhs, std::insert_iterator { result, result.begin() });
    return result;
  }

  [[nodiscard]] bool overlaps(const feature_ids_t& lhs, const feature_ids_t& rhs)
  {
    return feature_ids_t::overlaps(lhs, rhs);
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
          , m_features { m_spl.m_F,
                         theory_system_space(m_spl),
                         std::vector<std::string>(m_spl.m_feature_names.begin(),
                                                  m_spl.m_feature_names.end()),
                         m_spl.m_category_filter }
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
      [[nodiscard]] static differences_t
          search_minimal_differences(const feature_ids_t&     target_features,
                                     const system_ids_t&      i_set,
                                     const system_ids_t&      u_set,
                                     const system_feature_map_t& system_features);
      [[nodiscard]] isolation_result_t
          some_difference(unsigned_integer feature_id) const;
      [[nodiscard]] const feature_ids_t& sid_to_fids(unsigned_integer sid) const;
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
      struct witness_candidate_t
      {
          system_id_t   sid { zero };
          bool          from_intersection { false };
          feature_ids_t kills {};
      };

      [[nodiscard]] static unsigned_integer
          next_internal_feature_id(const feature_ids_t& feature_ids);
      [[nodiscard]] static feature_ids_t
          relevant_feature_universe(const system_ids_t&       i_set,
                                    const system_ids_t&       u_set,
                                    const system_feature_map_t& system_features,
                                    const feature_ids_t&      target_features);
      [[nodiscard]] static std::vector<witness_candidate_t>
          build_witness_candidates(const feature_ids_t&       bad_features,
                                   const system_ids_t&        i_set,
                                   const system_ids_t&        u_set,
                                   const system_feature_map_t& system_features);
      [[nodiscard]] static unsigned_integer
          select_branch_feature(const feature_ids_t& uncovered_features,
                                const std::vector<witness_candidate_t>& candidates,
                                const std::unordered_map<unsigned_integer,
                                                         std::vector<std::size_t>>&
                                    feature_to_candidate_indices,
                                const std::vector<bool>& available_candidates);
      [[nodiscard]] static bool
          has_covering_candidate(unsigned_integer feature_id,
                                 const std::unordered_map<unsigned_integer,
                                                          std::vector<std::size_t>>&
                                     feature_to_candidate_indices,
                                 const std::vector<bool>& available_candidates);
      [[nodiscard]] static bool dominated_by_existing(
          const differences_t& differences,
          const system_ids_t&  current_I,
          const system_ids_t&  current_U);
      static void search_covering_differences(
          const feature_ids_t& uncovered_features,
          system_ids_t&        current_I,
          system_ids_t&        current_U,
          const std::vector<witness_candidate_t>& candidates,
          const std::unordered_map<unsigned_integer, std::vector<std::size_t>>&
                               feature_to_candidate_indices,
          const std::vector<bool>& available_candidates,
          differences_t&             differences);
  };

  difference_t feature_isolation_t::partition(unsigned_integer feature_id) const
  {
    system_ids_t I, U;
    for (const auto& s : m_features.system_space().available)
    {
      const auto& system_features = m_features.m_systems.at(s - 1).all_features();
      if (system_features.contains(feature_id))
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

  const feature_ids_t& feature_isolation_t::sid_to_fids(unsigned_integer sid) const
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
    feature_ids_t result { sid_to_fids(*sids.begin()) };
    if (sids.size() > 1)
    {
      for (auto it { ++sids.begin() }; it != sids.end(); ++it)
      {
        result = intersection_of(result, sid_to_fids(*it));
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
    feature_ids_t result { sid_to_fids(*sids.begin()) };
    if (sids.size() > 1)
    {
      for (auto it { ++sids.begin() }; it != sids.end(); ++it)
      {
        result = union_of(result, sid_to_fids(*it));
      }
    }
    return result;
  }

  feature_ids_t
      feature_isolation_t::flattened_difference(const system_ids_t& i,
                                                const system_ids_t& u) const
  {
    const auto intersection = flatten_by_intersection(i);
    const auto union_set = flatten_by_union(u);
    return difference_of(intersection, union_set);
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

  differences_t feature_isolation_t::search_minimal_differences(
      const feature_ids_t&       target_features,
      const system_ids_t&        i_set,
      const system_ids_t&        u_set,
      const system_feature_map_t& system_features)
  {
    differences_t result {};
    const auto relevant_features
        = relevant_feature_universe(i_set, u_set, system_features, target_features);
    feature_ids_t bad_features { difference_of(relevant_features, target_features) };
    bad_features.insert(next_internal_feature_id(relevant_features));
    const auto candidates
        = build_witness_candidates(bad_features, i_set, u_set, system_features);

    std::unordered_map<unsigned_integer, std::vector<std::size_t>>
        feature_to_candidate_indices {};
    for (std::size_t candidate_index { 0 }; candidate_index < candidates.size();
         ++candidate_index)
    {
      for (const auto feature_id : candidates.at(candidate_index).kills)
      {
        feature_to_candidate_indices[feature_id].push_back(candidate_index);
      }
    }

    system_ids_t current_I {};
    system_ids_t current_U {};
    const std::vector<bool> available_candidates(candidates.size(), true);
    search_covering_differences(bad_features,
                                current_I,
                                current_U,
                                candidates,
                                feature_to_candidate_indices,
                                available_candidates,
                                result);
    return result;
  }

  isolation_result_t feature_isolation_t::minimal_differences(
      unsigned_integer feature_id) const
  {
    isolation_result_t result {};
    auto [I_set, U_set] = partition(feature_id);

    system_feature_map_t system_features {};
    for (const auto sid : m_features.system_space().available)
    {
      system_features.emplace(sid, sid_to_fids(sid));
    }

    result.features    = flattened_difference(I_set, U_set);
    result.differences = search_minimal_differences(
        result.features, I_set, U_set, system_features);

    return result;
  }

  unsigned_integer
      feature_isolation_t::next_internal_feature_id(const feature_ids_t& feature_ids)
  {
    unsigned_integer max_feature_id { zero };
    for (const auto feature_id : feature_ids)
    {
      max_feature_id = std::max(max_feature_id, feature_id);
    }
    return max_feature_id + one;
  }

  feature_ids_t feature_isolation_t::relevant_feature_universe(
      const system_ids_t&        i_set,
      const system_ids_t&        u_set,
      const system_feature_map_t& system_features,
      const feature_ids_t&       target_features)
  {
    feature_ids_t universe { target_features };
    for (const auto sid : i_set)
    {
      universe = union_of(universe, system_features.at(sid));
    }
    for (const auto sid : u_set)
    {
      universe = union_of(universe, system_features.at(sid));
    }
    return universe;
  }

  std::vector<feature_isolation_t::witness_candidate_t>
      feature_isolation_t::build_witness_candidates(
          const feature_ids_t&       bad_features,
          const system_ids_t&        i_set,
          const system_ids_t&        u_set,
          const system_feature_map_t& system_features)
  {
    std::vector<witness_candidate_t> candidates {};
    candidates.reserve(i_set.size() + u_set.size());

    unsigned_integer requirement_feature_id { zero };
    for (const auto feature_id : bad_features)
    {
      requirement_feature_id = feature_id;
    }

    for (const auto sid : i_set)
    {
      const auto present_bad_features
          = intersection_of(system_features.at(sid), bad_features);
      auto kills = difference_of(bad_features, present_bad_features);
      kills.insert(requirement_feature_id);
      candidates.push_back({ sid, true, kills });
    }

    for (const auto sid : u_set)
    {
      const auto kills = intersection_of(system_features.at(sid), bad_features);
      if (!kills.empty())
      {
        candidates.push_back({ sid, false, kills });
      }
    }

    return candidates;
  }

  unsigned_integer feature_isolation_t::select_branch_feature(
      const feature_ids_t& uncovered_features,
      const std::vector<witness_candidate_t>& candidates,
      const std::unordered_map<unsigned_integer, std::vector<std::size_t>>&
          feature_to_candidate_indices,
      const std::vector<bool>& available_candidates)
  {
    unsigned_integer selected_feature { zero };
    std::size_t      smallest_cover_count { std::numeric_limits<std::size_t>::max() };
    for (const auto feature_id : uncovered_features)
    {
      std::size_t cover_count { 0 };
      if (const auto it = feature_to_candidate_indices.find(feature_id);
          it != feature_to_candidate_indices.end())
      {
        for (const auto candidate_index : it->second)
        {
          if (available_candidates.at(candidate_index)
              && candidates.at(candidate_index).kills.contains(feature_id))
          {
            ++cover_count;
          }
        }
      }
      if (cover_count < smallest_cover_count)
      {
        smallest_cover_count = cover_count;
        selected_feature = feature_id;
      }
    }
    return selected_feature;
  }

  bool feature_isolation_t::has_covering_candidate(
      unsigned_integer feature_id,
      const std::unordered_map<unsigned_integer, std::vector<std::size_t>>&
          feature_to_candidate_indices,
      const std::vector<bool>& available_candidates)
  {
    const auto it = feature_to_candidate_indices.find(feature_id);
    if (it == feature_to_candidate_indices.end())
    {
      return false;
    }
    for (const auto candidate_index : it->second)
    {
      if (available_candidates.at(candidate_index))
      {
        return true;
      }
    }
    return false;
  }

  bool feature_isolation_t::dominated_by_existing(const differences_t& differences,
                                                  const system_ids_t&  current_I,
                                                  const system_ids_t&  current_U)
  {
    return std::ranges::any_of(differences,
                               [&](const difference_t& difference) {
                                 return proper_difference_subset(
                                     current_I, current_U, difference);
                               });
  }

  void feature_isolation_t::search_covering_differences(
      const feature_ids_t& uncovered_features,
      system_ids_t&        current_I,
      system_ids_t&        current_U,
      const std::vector<witness_candidate_t>& candidates,
      const std::unordered_map<unsigned_integer, std::vector<std::size_t>>&
          feature_to_candidate_indices,
      const std::vector<bool>& available_candidates,
      differences_t& differences)
  {
    if (dominated_by_existing(differences, current_I, current_U))
    {
      return;
    }

    if (uncovered_features.empty())
    {
      memorize_proper_difference_subset(differences, current_I, current_U);
      return;
    }

    const auto branch_feature = select_branch_feature(uncovered_features,
                                                      candidates,
                                                      feature_to_candidate_indices,
                                                      available_candidates);
    if (branch_feature == zero)
    {
      return;
    }
    if (!has_covering_candidate(branch_feature,
                                feature_to_candidate_indices,
                                available_candidates))
    {
      return;
    }

    const auto& covering_candidates
        = feature_to_candidate_indices.at(branch_feature);
    for (const auto candidate_index : covering_candidates)
    {
      if (!available_candidates.at(candidate_index))
      {
        continue;
      }

      const auto& candidate = candidates.at(candidate_index);
      std::vector<bool> next_available_candidates { available_candidates };
      for (const auto other_index : covering_candidates)
      {
        if (other_index <= candidate_index)
        {
          next_available_candidates.at(other_index) = false;
        }
      }

      auto& target_set = candidate.from_intersection ? current_I : current_U;
      target_set.insert(candidate.sid);
      const auto next_uncovered
          = difference_of(uncovered_features, candidate.kills);
      search_covering_differences(next_uncovered,
                                  current_I,
                                  current_U,
                                  candidates,
                                  feature_to_candidate_indices,
                                  next_available_candidates,
                                  differences);
      target_set.erase(candidate.sid);
    }
  }

  isolation_result_t
      feature_isolation_t::some_difference(unsigned_integer feature_id) const
  {
    auto [i_set, u_set] { partition(feature_id) };
    auto               i { to_vector(i_set) };
    const auto target_features { flattened_difference(i_set, u_set) };
    isolation_result_t result;
    difference_t       difference;
    if (i.size())
    {
      result.features = sid_to_fids(i.at(0));
      difference.I.insert(i.at(0));
      if (result.features == target_features)
      {
        result.differences.insert(difference);
        return result;
      }
    }
    for (auto index { one }; index < i.size(); ++index)
    {
      difference.I.insert(i.at(index));
      result.features = intersection_of(result.features, sid_to_fids(i.at(index)));
      if (result.features == target_features)
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
        result.features = difference_of(result.features, sid_to_fids(u.at(index)));
        if (result.features == target_features)
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
    os << "i" << separator() << "FEATURE" << separator() << "ℓ_T(t_i)"
       << separator() << "CANDIDATE FEATURES" << separator() << "RESULT TYPE" << separator();
    if (all_differences)
    {
      os << "#WITNESSES" << separator() << "MINIMAL DIFFERENCES";
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
    os << paper_feature_index(fid) << separator() << paper_feature_label(fid)
       << separator() << name(fid) << separator() << "{ ";
    for (auto counter { zero }; auto candidate_fid : result.features)
    {
      os << paper_feature_label(candidate_fid);
      if (feature_names)
      {
        os << " = " << name(candidate_fid);
      }
      os << (++counter < result.features.size() ? ", " : "");
    }
    os << " }" << separator()
       << (result.features.size() == one ? "UNIQUE" : "AMBIGUOUS")
       << separator();
    if (all_differences)
    {
      os << result.differences.size() << separator();
    }
    for (auto counter { zero }; const auto& difference : result.differences)
    {
      print(os, difference);
      os << (++counter < result.differences.size() ? std::string { separator() } : "");
    }
    os << std::endl;
  }


  std::vector<bool> operator|(std::vector<bool> lhs, const std::vector<bool>& rhs)
  {
    const auto size = std::min(lhs.size(), rhs.size());
    for (std::size_t i { 0 }; i < size; ++i)
    {
      lhs[i] = lhs[i] || rhs[i];
    }
    return lhs;
  }

  std::vector<bool> operator&(std::vector<bool> lhs, const std::vector<bool>& rhs)
  {
    const auto size = std::min(lhs.size(), rhs.size());
    for (std::size_t i { 0 }; i < size; ++i)
    {
      lhs[i] = lhs[i] && rhs[i];
    }
    return lhs;
  }

  std::vector<bool> operator~(std::vector<bool> value)
  {
    for (std::size_t i { 0 }; i < value.size(); ++i)
    {
      value[i] = !value[i];
    }
    return value;
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

  std::string feature_isolation_t::name(unsigned_integer fid) const
  {
    const auto feature_description { m_features.at(fid) };
    if (m_features.uses_theory_registry())
    {
      const auto& record = m_features.feature_registry().at(fid - one);
      if (!record.display_name.empty())
      {
        return record.display_name;
      }
      if (!record.simplified_term.empty())
      {
        return record.simplified_term;
      }
      if (!record.raw_term.empty())
      {
        return record.raw_term;
      }
      return to_string(record.systems_present);
    }

    std::ostringstream os;
    os << m_features.category_name(fid) << ':' << to_string(feature_description);
    return os.str();
  }

  void feature_isolation_t::print(std::ostream& os,
                                  bool          all_differences,
                                  bool          feature_names) const
  {
    print_header(os, all_differences, feature_names);
    os << std::endl;
    for (auto fid { one }; fid <= m_features.feature_count(); ++fid)
    {
      if (!m_features.includes_feature(fid))
      {
        continue;
      }
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
      os << ' ' << paper_system_label(id) << ' ' << (++i < sids.size() ? op : "");
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
