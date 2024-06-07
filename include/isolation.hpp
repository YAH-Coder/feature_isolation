#ifndef ISOLATION_HPP
#define ISOLATION_HPP

#include "feature_analysis.hpp"
#include "spl.hpp"
#include <set>
#include <fstream>

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
} // namespace feature_isolation
#endif // ISOLATION_HPP
