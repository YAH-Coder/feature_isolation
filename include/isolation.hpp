#ifndef ISOLATION_HPP
#define ISOLATION_HPP

#include "feature_analysis2.hpp"
#include "spl.hpp"

namespace feature_isolation {

   class feature_isoation_t final
   {
      public:
         explicit feature_isoation_t(const std::string& filename):
            m_spl { filename },
            m_features { m_spl.m_f,m_spl.m_max_rank,m_spl.m_feature_categories }
         {
         }
         const spl_specification_t m_spl;
         const feature_analysis_t m_features;
   };
} // namespace feature_isolation
#endif // ISOLATION_HPP
