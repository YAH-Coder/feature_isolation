#ifndef ISOLATION_HPP
#define ISOLATION_HPP

#include "feature_analysis2.hpp"
#include "spl.hpp"
#include <set>
#include <fstream>

namespace feature_isolation {

   class feature_isolation_t final
   {
      public:
         using systems_t = spl_specification_t::systems_t;
         struct decomposed_ids_t
         {
            systems_t i, // IDs of systems to be intersected.
                      u; // IDs of systems to be united.
         };
         explicit feature_isolation_t(const std::string& filename):
            m_spl { filename },
            m_features { m_spl.m_F,m_spl.m_max_rank,m_spl.m_categories }
         {
            m_spl.output_to_csv("SPL_RESULT.csv");
            std::ofstream out { "SPL_FEATURES.csv" };
            m_features.print(out);
         }
         [[nodiscard]] decomposed_ids_t decompose(unsigned_integer feature_id) const;
         const spl_specification_t m_spl;
         const feature_analysis_t m_features;
   };

   feature_isolation_t::decomposed_ids_t feature_isolation_t::decompose(unsigned_integer feature_id) const
   {
      systems_t i,u;
      for (const auto& s : m_spl.m_available_systems)
      {
         if (m_features.m_systems.at(s - 1).all_features().contains(feature_id))
         {
            i.insert(s);
         }
         else
         {
            u.insert(s);
         }
      }
      return { i,u };
   }
} // namespace feature_isolation
#endif // ISOLATION_HPP
