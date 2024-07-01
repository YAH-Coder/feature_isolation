#ifndef ISOLATION_HPP
#define ISOLATION_HPP

#include "feature_analysis.hpp"
#include "spl.hpp"
#include <set>
#include <fstream>
#include <algorithm>
#include <vector>
#include <compare>
#include <iostream>
#include <string>

namespace feature_isolation {

   std::string to_category_name(const unsigned_integer& category);
   
   template <typename T>
   std::vector<typename T::value_type> to_vector(const T& c)
   {
      std::vector<typename T::value_type> result {};
      result.reserve(c.size());
      for (const auto& e : c)
      {
         result.push_back(e);
      }
      return result;
   }
   
   template <typename T>
   std::set<typename T::value_type> to_set(const T& c)
   {
      std::set<typename T::value_type> result {};
      for (const auto& e : c)
      {
         result.insert(e);
      }
      return result;
   }
   
   template <typename T>
   [[nodiscard]] bool subset(const std::set<T>& s1,const std::set<T>& s2)
   // Returns true, if s2 is a subset of s1
   {
      return std::ranges::includes(s1,s2);
   }

   template <typename T>
   [[nodiscard]] bool proper_subset(const std::set<T>& s1,const std::set<T>& s2)
   // Returns true, if s2 is a proper subset of s1
   {
      return subset(s1,s2) and s1.size() > s2.size();
   }
   
   struct difference_t
   {
      system_ids_t I,
                   U;
   };

   std::strong_ordering operator<=>(const difference_t& lhs,const difference_t& rhs)
   {
      if (lhs.I < rhs.I or (lhs.I == rhs.I and lhs.U < rhs.U))
            return std::strong_ordering::less;
        if (lhs.I > rhs.I or (lhs.I == rhs.I and lhs.U > rhs.U))
            return std::strong_ordering::greater;
        return std::strong_ordering::equivalent;
   }

   bool operator==(const difference_t& lhs,const difference_t& rhs)
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
         explicit feature_isolation_t(const std::string& filename):
            m_spl { filename },
            m_features { m_spl.m_F,m_spl.m_max_rank,m_spl.m_categories }
         {}
         [[nodiscard]] difference_t partition(unsigned_integer feature_id) const;
         [[nodiscard]] feature_ids_t flatten_by_intersection(const system_ids_t& sids) const;
         [[nodiscard]] feature_ids_t flatten_by_union(const system_ids_t& sids) const;
         [[nodiscard]] feature_ids_t flattened_difference(const system_ids_t& i,const system_ids_t& u) const;
         [[nodiscard]] isolation_result_t minimal_differences(unsigned_integer feature_id) const;                          
         [[nodiscard]] feature_ids_t sid_to_fids(unsigned_integer sid) const;
         static void memorize_proper_difference_subset(differences_t& md,const system_ids_t& i,const system_ids_t& u);
         [[nodiscard]] static bool proper_difference_subset(const system_ids_t& i,const system_ids_t& u,const difference_t& d);
         static void print_header(std::ostream& os,bool feature_names = false);
         void print(std::ostream& os,unsigned_integer fid,bool feature_names) const;
         void print(std::ostream& os,bool feature_names = false) const;
         [[nodiscard]] std::string name(unsigned_integer fid) const;
         static void print(std::ostream& os,const system_ids_t& sids);
         static void print(std::ostream& os,const difference_t& d);
         const spl_specification_t m_spl;
         const feature_analysis_t m_features;
         unsigned_integer category(const unsigned_integer& fid) const;
   };

   difference_t feature_isolation_t::partition(unsigned_integer feature_id) const
   {
      system_ids_t I,U;
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
      return { I,U };
   }
   feature_ids_t feature_isolation_t::sid_to_fids(unsigned_integer sid) const
   {
      return m_features.m_systems.at(sid - 1).all_features();
   }
   feature_ids_t feature_isolation_t::flatten_by_intersection(const system_ids_t& sids) const
   {
      if (sids.size() == 0)
      {
         return { };
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
            std::ranges::set_intersection(set1,set2,std::insert_iterator { result,result.begin() });
         }
      }
      return result;
   }
   feature_ids_t feature_isolation_t::flatten_by_union(const system_ids_t& sids) const
   {
      if (sids.size() == 0)
      {
         return { };
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
            std::ranges::set_union(set1,set2,std::insert_iterator { result,result.begin() });
         }
      }
      return result;
   }
   feature_ids_t feature_isolation_t::flattened_difference(const system_ids_t& i,const system_ids_t& u) const 
   {
      feature_ids_t result {};
      std::ranges::set_difference(flatten_by_intersection(i),flatten_by_union(u),std::insert_iterator { result,result.begin() });
      return result;
   }
   isolation_result_t feature_isolation_t::minimal_differences(unsigned_integer feature_id) const
   {
      auto [i,u] { partition(feature_id) };
      isolation_result_t result { { flattened_difference(i,u) } };
      for (auto u_k { zero }; u_k <= u.size(); ++u_k)
      {
         combination u_draw { to_vector(u),u_k };
         do
         {
            for (auto i_k { one }; i_k <= i.size(); ++i_k)
            {
               combination i_draw { to_vector(i),i_k };
               do
               {
                  system_ids_t i_s { to_set(i_draw()) };
                  system_ids_t u_s { to_set(u_draw()) };
                  auto d_f { flattened_difference(i_s,u_s) };
                  if (d_f == result.features)
                  {
                     memorize_proper_difference_subset(result.differences,i_s,u_s);
                  }
               }
               while (i_draw.next());
            }
         } while(u_draw.next());
      }
      return result;
   }
   bool feature_isolation_t::proper_difference_subset(const system_ids_t& i,const system_ids_t& u,const difference_t& d)
   {
     return proper_subset(i,d.I) or proper_subset(u,d.U);
   }
   void feature_isolation_t::memorize_proper_difference_subset(differences_t& md,const system_ids_t& i,const system_ids_t& u)
   {
      for (const auto& d : md)
      {
         if (proper_difference_subset(i,u,d))
         {
            return;
         }
      }
      md.insert(difference_t { i,u });
   }
    void feature_isolation_t::print_header(std::ostream& os,bool feature_names)
    {
       os << (feature_names ? "Feature Name" : "Feature ID") << separator()
          << "Isolation Result" << separator()
          << "# Min. Diff" << separator()
          << "Minimal Differences" << std::endl;
    }
    void feature_isolation_t::print(std::ostream& os,unsigned_integer fid,bool feature_names) const
    {
       auto result { minimal_differences(fid) };
       os << (feature_names ? name(fid) : std::to_string(fid)) << separator()
          << "{ ";
       for (auto counter { zero };auto fid : result.features)
       {
          os << (feature_names ? name(fid) : std::to_string(fid))
             << (++counter < result.features.size() ? ", " : "");
       }
       os << " }" << separator()
                  << result.differences.size()
                  << separator();
       for (auto counter { zero };auto difference : result.differences)
       {
          os << "{ ";

          for (auto counter { zero };auto sid : difference.I)
          {
             os << sid
                << (++counter < difference.I.size() ? ", " : "");
          }
          os << " } \\ { ";
          for (auto counter { zero };auto sid : difference.U)
          {
             os << sid
                << (++counter < difference.U.size() ? ", " : "");
          }
          os << " }";
          os << (++counter << result.differences.size() ? std::string { separator() } : "");
      }
      os << std::endl;
    }
    std::string feature_isolation_t::name(unsigned_integer fid) const
    {
       std::cerr << "feature_isolation::name(unsigned_integer fid) const" << "Not yet implemented" << std::endl;
       return "Not yet implemented";
    }
    void feature_isolation_t::print(std::ostream& os,bool feature_names) const
    {
       print_header(os,feature_names);
       for (auto fid { one };fid <= m_features.T;++fid)
       {
          print(os,fid,feature_names);
       }
    }
    void feature_isolation_t::print(std::ostream& os,const system_ids_t& sids)
    {
       os << "{";
       for (auto i { zero };auto id : sids)
       {
          os << " " << id << (++i < sids.size() ? "," : "");
       }
       os << "}";
    }
    void feature_isolation_t::print(std::ostream& os,const difference_t& difference)
    {
       print(os,difference.I);
       os << " \\ ";
       print(os,difference.U);
       os << std::endl;
    }
    unsigned_integer feature_isolation_t::category(const unsigned_integer& fid) const
    {
       auto feature_description { m_features.at(fid) };
       auto ones { std::ranges::count(m_features.at(fid),true) };
       auto lsb1 { feature_description.at(0) };
       auto lsb0 { !lsb1 };
       auto msb1 { feature_description.at(feature_description.size() - 1) };
       auto msb0 { !msb1 };
       auto o_eq_z { ones * 2 == feature_description.size() };
       auto o_leq_2 { ones <= two };
       auto o_geq_1 { ones >= one };
//       std::cout << "FID " << fid << " ones " << ones << " lsb1 " << lsb1 << " msb1 " << msb1 << " o_eq_z " << o_eq_z << " o_leq_2 " << o_leq_2 << " o_geq_1 " << o_geq_1 << " z_leq_2 " << z_leq_2 << std::endl;       
       if (lsb0 and msb0)
          return ::feature_isolation::ANF;
       if (lsb1 and msb1)
          return ::feature_isolation::ONF;
       if (lsb1 and o_eq_z)
          return ::feature_isolation::F;
       if (msb1 and o_eq_z)
          return ::feature_isolation::N;
       if (lsb1 and o_leq_2)
          return ::feature_isolation::A;
       if (lsb1 and o_geq_1)
          return ::feature_isolation::O;
       if (lsb0 and  o_leq_2)
          return ::feature_isolation::AN;
       if (msb1 and lsb0)
          return ::feature_isolation::ON;
       return ~zero;
    }
    std::string to_category_name(const unsigned_integer& category)
    {
       auto index { zero };
       while (category != one << index)
       {
          ++index;
       }
       //std::cout << "INDEX " << index << std::endl;
       return (index < 8 ? feature_categories[index] : "ILLEGAL CATEGORY");
    }
} // namespace feature_isolation
#endif // ISOLATION_HPP
