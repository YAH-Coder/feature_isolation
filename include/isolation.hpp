#ifndef ISOLATION_HPP
#define ISOLATION_HPP

#include "feature_analysis.hpp"
#include "spl.hpp"
#include <set>
#include <fstream>
#include <algorithm>
#include <string>

template <class T>
void print(std::string m,const T& c)
{
   auto s { c.size() };
   std::cout << m << ": { ";
   for (const auto& e : c)
   {
      std::cout << e << ((--s > 0) ? ", " : " ");
   }
   std::cout << "}" << std::endl;
}

namespace feature_isolation {

   struct difference_t
   {
         system_ids_t I,
                      U;
   };

   using minimal_differences_t = std::set<difference_t>;

   struct isolation_result_t
   {
      feature_ids_t features;
      minimal_differences_t differences;
   };
   
   class feature_isolation_t final
   {
      public:
         explicit feature_isolation_t(const std::string& filename):
            m_spl { filename },
            m_features { m_spl.m_F,m_spl.m_max_rank,m_spl.m_categories }
         {
            m_spl.output_to_csv("SPL_RESULT.csv");
            std::ofstream out { "SPL_FEATURES.csv" };
            m_features.print(out);
         }
         [[nodiscard]] difference_t partition(unsigned_integer feature_id) const;
         [[nodiscard]] isolation_result_t isolate(unsigned_integer feature_id) const;
         [[nodiscard]] feature_ids_t flatten_by_intersection(const system_ids_t& sids) const;
         [[nodiscard]] feature_ids_t flatten_by_union(const system_ids_t& sids) const;
         [[nodiscard]] feature_ids_t flattened_difference(unsigned_integer feature_id) const;
         [[nodiscard]] feature_ids_t sid_to_fids(unsigned_integer sid) const;
         const spl_specification_t m_spl;
         const feature_analysis_t m_features;
   };

   isolation_result_t feature_isolation_t::isolate(unsigned_integer feature_id) const
   {
      return isolation_result_t { };
   }
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
            std::ranges::set_intersection(set1,set2,std::insert_iterator<decltype(result)>(result,result.begin()));
/*            std::set_intersection(set1.begin(),set1.end(),
                                  set2.begin(),set2.end(),
                                  std::insert_iterator<decltype(result)>(result,result.begin())); */
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
            std::ranges::set_union(set1,set2,std::insert_iterator<decltype(result)>(result,result.begin()));
         }
      }
      return result;
   }
   feature_ids_t feature_isolation_t::flattened_difference(unsigned_integer feature_id) const
   {
      auto [i,u] { partition(feature_id) };
      feature_ids_t result {};
      std::ranges::set_difference(flatten_by_intersection(i),flatten_by_union(u),std::insert_iterator<decltype(result)>(result,result.begin()));
      return result;
   }
} // namespace feature_isolation
#endif // ISOLATION_HPP
