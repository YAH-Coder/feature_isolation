#ifndef ISOLATION_HPP
#define ISOLATION_HPP

#include "feature_analysis.hpp"
#include "spl.hpp"
#include <algorithm>
#include <compare>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace feature_isolation
{
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
  // Returns true, if s2 is a subset of s1
  {
    return std::ranges::includes(s1, s2);
  }

  template<typename T>
  [[nodiscard]] bool proper_subset(const std::set<T>& s1, const std::set<T>& s2)
  // Returns true, if s2 is a proper subset of s1
  {
    return subset(s1, s2) and s1.size() > s2.size();
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
          : m_spl { filename }
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
      static void               print(std::ostream& os, const difference_t& d);
      const spl_specification_t m_spl;
      const feature_analysis_t  m_features;
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

  isolation_result_t feature_isolation_t::minimal_differences(
      unsigned_integer feature_id) const
  {
    auto [i, u] { partition(feature_id) };
    isolation_result_t result { { flattened_difference(i, u) } };
    for (auto u_k { zero }; u_k <= u.size(); ++u_k)
    {
      combination u_draw { to_vector(u), u_k };
      do
      {
        for (auto i_k { one }; i_k <= i.size(); ++i_k)
        {
          combination i_draw { to_vector(i), i_k };
          do
          {
            system_ids_t i_s { to_set(i_draw()) };
            system_ids_t u_s { to_set(u_draw()) };
            auto         d_f { flattened_difference(i_s, u_s) };
            if (d_f == result.features)
            {
              memorize_proper_difference_subset(result.differences, i_s, u_s);
            }
          }
          while (i_draw.next());
        }
      }
      while (u_draw.next());
    }
    return result;
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

  void feature_isolation_t::memorize_proper_difference_subset(
      differences_t& md, const system_ids_t& i, const system_ids_t& u)
  {
    for (const auto& d : md)
    {
      if (proper_difference_subset(i, u, d))
      {
        return;
      }
    }
    md.insert(difference_t { i, u });
  }

  /*  void feature_isolation_t::print_header(std::ostream& os,
                                           bool          all_differences,
                                           bool          feature_names)
    {
      os << (feature_names ? "Feature Name" : "Feature ID") << separator()
         << "Isolation Result" << separator()
         << (all_differences ? "# Min. Diff" : "") << separator()
         << (all_differences ? "Minimal Differences" : "Some Difference")
         << std::endl;
    }*/

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

  /*  void feature_isolation_t::print(std::ostream&    os,
                                    unsigned_integer fid,
                                    bool             all_differences,
                                    bool             feature_names) const
    {
      auto result { all_differences ? minimal_differences(fid)
                                    : some_difference(fid) };
      os << (feature_names ? name(fid) : std::to_string(fid)) << separator()
         << "{ ";
      for (auto counter { zero }; auto fid : result.features)
      {
        os << (feature_names ? name(fid) : std::to_string(fid))
           << (++counter < result.features.size() ? ", " : "");
      }
      os << " }" << separator() << result.differences.size() << separator();
      for (auto counter { zero }; auto difference : result.differences)
      {
        os << "{ ";

        for (auto counter { zero }; auto sid : difference.I)
        {
          os << sid << (++counter < difference.I.size() ? ", " : "");
        }
        os << " } \\ { ";
        for (auto counter { zero }; auto sid : difference.U)
        {
          os << sid << (++counter < difference.U.size() ? ", " : "");
        }
        os << " }";
        os << (++counter << result.differences.size()
                   ? std::string { separator() }
                   : "");
      }
      os << std::endl;
    }*/

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
    // for (auto counter { zero }; auto difference : result.differences)
    for (auto counter { zero }; const auto& difference : result.differences)
    {
      /*      os << "{ ";

            for (auto counter { zero }; auto sid : difference.I)
            {
              os << sid << (++counter < difference.I.size() ? ", " : "");
            }
            os << " } \\ { ";
            for (auto counter { zero }; auto sid : difference.U)
            {
              os << sid << (++counter < difference.U.size() ? ", " : "");
            }
            os << " }";*/
      print(os, difference);
      os << (++counter << result.differences.size()
                 ? std::string { separator() }
                 : "");
    }
    os << std::endl;
  }

  bool feature_present(const std::vector<bool>& anf_feature,
                       const std::vector<bool>& f_feature,unsigned_integer F)
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

  std::vector<bool> operator | (std::vector<bool> v1,const std::vector<bool>& v2)
  {
     if (v1.size() != v2.size())
     {
        throw std::range_error("operator |(): bool vectors have different sizes!");
     }
     for (auto i { zero }; i < v1.size(); ++i)
     {
        v1[i] = v1[i] || v2[i];
     }
     return v1;
  }

  std::vector<bool> operator & (std::vector<bool> v1,const std::vector<bool>& v2)
  {
     if (v1.size() != v2.size())
     {
        throw std::range_error("operator &(): bool vectors have different sizes!");
     }
     for (auto i { zero }; i < v1.size(); ++i)
     {
        v1[i] = v1[i] && v2[i];
     }
     return v1;
  }

  std::vector<bool> operator ~ (std::vector<bool> v)
  {
     v.flip();
     return v;
  }

  auto operator == (const std::vector<bool>& v1,const std::vector<bool>& v2)
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
     return std::ranges::count(v,true);  
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
// The following code is error prone!
/*        for (auto index { zero }; const auto& f : m_features.F_features)
        {
          if (feature_and(f_desc, f))
          {
            if (first)
            {
              result += m_spl.m_feature_names.at(index);
              first   = false;
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
              result += "!" + m_spl.m_feature_names.at(index);
              first   = false;
            }
            else
            {
              result += " * !" + m_spl.m_feature_names.at(index);
            }
          }
          ++index;
        }
        return result;*/
        // WARNING: NOT GUARANTEED TO WORK FOR ANY F! JUST A TRY.
/*        for (auto index { zero }; const auto& f : m_features.F_features)
        {
          if (feature_present(f_desc, f, m_features.F))
          {
            if (first)
            {
              result += m_spl.m_feature_names.at(index);
              first   = false;
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
              result += "!" + m_spl.m_feature_names.at(index);
              first   = false;
            }
            else
            {
              result += " * !" + m_spl.m_feature_names.at(index);
            }
          }
          ++index;
        }
        return result; */
        // The following code implements the explicit way (not verified, not
        // tested).
        for (auto index { zero }; const auto& f : m_features.F_features)
        {
          auto f_count { dot_product(f_desc & f) },
               n_count { dot_product(f_desc & ~f) };
          std::cout << "F " << f_count << " N " << n_count << std::endl;
          if (f_count > n_count)
          {
            if (first)
            {
              result += m_spl.m_feature_names.at(index);
              first   = false;
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
              result += "!" + m_spl.m_feature_names.at(index);
              first   = false;
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
      os << ' ' << id << ' '<< (++i < sids.size() ? op : "");
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

