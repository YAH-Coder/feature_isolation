/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
/**
 * @file feature_analysis.hpp
 * @brief Derived-feature-space modeling, categorization, planning, and reporting support.
 */
#ifndef FEATURE_ANALYSIS_HPP
#define FEATURE_ANALYSIS_HPP

#include "../include/fi_mathhelper.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <optional>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

namespace feature_isolation
{
  /** @brief Bit-order flag retained by legacy feature-vector formatting helpers. */
  constexpr bool MSB_LSB {
    true
  };
  /** @brief Number of usable bits in @ref unsigned_integer. */
  constexpr unsigned_integer feature_digits { sizeof(unsigned_integer)
                                              * CHAR_BIT };

  /** @brief Bit-vector describing in which valid systems a derived feature is present. */
  using feature_t  = std::vector<bool>;
  /** @brief Sequence of derived-feature presence vectors. */
  using features_t = std::vector<feature_t>;
  /** @brief Generic sorted identifier set. */
  using ids_t      = std::set<unsigned_integer>;

  /** @brief Generic debug-print helper implemented in the analysis module. */
  template<typename T> void print(std::string m, const T& c);

  /**
   * @brief Compact ordered set of one-based feature identifiers.
   *
   * The class stores identifiers as a bitset-backed sparse set while exposing
   * ordered iteration over one-based ids.
   */
  class feature_ids_t
  {
    public:
      using value_type = unsigned_integer;
      using size_type  = std::size_t;

      /** @brief Forward iterator over the contents of a @ref feature_ids_t. */
      class const_iterator
      {
        public:
          using iterator_category = std::forward_iterator_tag;
          using value_type        = unsigned_integer;
          using difference_type   = std::ptrdiff_t;
          using pointer           = const value_type*;
          using reference         = value_type;

          /** @brief Construct the default/end iterator. */
          const_iterator() = default;
          /** @brief Construct an iterator for a given owner and bit position. */
          const_iterator(const feature_ids_t* owner, size_type bit_index)
              : m_owner { owner }
              , m_bit_index { bit_index }
          { }

          /** @brief Return the current one-based feature identifier. */
          [[nodiscard]] value_type operator*() const
          {
            return static_cast<value_type>(m_bit_index + one);
          }

          /** @brief Advance to the next stored identifier. */
          const_iterator& operator++()
          {
            if (m_owner == nullptr)
            {
              return *this;
            }
            m_bit_index = m_owner->next_set_bit(m_bit_index + one);
            return *this;
          }

          /** @brief Post-increment overload. */
          const_iterator operator++(int)
          {
            auto copy { *this };
            ++(*this);
            return copy;
          }

          /** @brief Compare iterators for equality. */
          [[nodiscard]] bool operator==(const const_iterator& other) const
          {
            return m_owner == other.m_owner && m_bit_index == other.m_bit_index;
          }

        private:
          const feature_ids_t* m_owner { nullptr };
          size_type            m_bit_index { 0 };
      };

      /** @brief Construct an empty identifier set. */
      feature_ids_t() = default;

      /** @brief Construct a set from an initializer list of identifiers. */
      feature_ids_t(std::initializer_list<value_type> ids)
      {
        insert(ids.begin(), ids.end());
      }

      /** @brief Construct a set from an iterator range of identifiers. */
      template<typename InputIt>
      feature_ids_t(InputIt first, InputIt last)
      {
        insert(first, last);
      }

      /** @brief Report whether the set is empty. */
      [[nodiscard]] bool empty() const
      {
        return m_size == 0;
      }

      /** @brief Return the number of stored identifiers. */
      [[nodiscard]] size_type size() const
      {
        return m_size;
      }

      /** @brief Remove all identifiers from the set. */
      void clear()
      {
        m_words.clear();
        m_size = 0;
      }

      /** @brief Report whether a one-based identifier is present. */
      [[nodiscard]] bool contains(value_type id) const
      {
        if (id == zero)
        {
          return false;
        }
        const auto bit_index   = static_cast<size_type>(id - one);
        const auto word_index  = bit_index / bits_per_word;
        const auto bit_offset  = bit_index % bits_per_word;
        if (word_index >= m_words.size())
        {
          return false;
        }
        return (m_words.at(word_index) & (word_type { 1 } << bit_offset)) != 0;
      }
      /** @brief Set-style count operation returning either zero or one. */
      [[nodiscard]] size_type count(value_type id) const
      {
        return contains(id) ? one : zero;
      }


      /**
       * @brief Insert a one-based identifier.
       * @throws std::invalid_argument If @p id is zero.
       */
      void insert(value_type id)
      {
        if (id == zero)
        {
          throw std::invalid_argument("Feature identifiers must be positive.");
        }
        const auto bit_index  = static_cast<size_type>(id - one);
        const auto word_index = bit_index / bits_per_word;
        const auto bit_offset = bit_index % bits_per_word;
        if (word_index >= m_words.size())
        {
          m_words.resize(word_index + one, 0);
        }
        const auto mask = word_type { 1 } << bit_offset;
        if ((m_words.at(word_index) & mask) == 0)
        {
          m_words.at(word_index) |= mask;
          ++m_size;
        }
      }

      /** @brief Insert all identifiers from an iterator range. */
      template<typename InputIt> void insert(InputIt first, InputIt last)
      {
        for (; first != last; ++first)
        {
          insert(*first);
        }
      }

      /** @brief Return an iterator to the smallest stored identifier. */
      [[nodiscard]] const_iterator begin() const
      {
        return const_iterator { this, next_set_bit(0) };
      }

      /** @brief Return the past-the-end iterator. */
      [[nodiscard]] const_iterator end() const
      {
        return const_iterator { this, bit_capacity() };
      }

      /** @brief Compare two identifier sets for equality. */
      [[nodiscard]] bool operator==(const feature_ids_t& other) const
      {
        return words_equal(other);
      }

      /** @brief Compare two identifier sets for inequality. */
      [[nodiscard]] bool operator!=(const feature_ids_t& other) const
      {
        return !(*this == other);
      }

      /** @brief Lexicographically order two identifier sets by their sorted contents. */
      [[nodiscard]] bool operator<(const feature_ids_t& other) const
      {
        auto lhs_it = begin();
        auto rhs_it = other.begin();
        const auto lhs_end = end();
        const auto rhs_end = other.end();
        for (; lhs_it != lhs_end && rhs_it != rhs_end; ++lhs_it, ++rhs_it)
        {
          if (*lhs_it < *rhs_it)
          {
            return true;
          }
          if (*rhs_it < *lhs_it)
          {
            return false;
          }
        }
        return lhs_it == lhs_end && rhs_it != rhs_end;
      }

      /** @brief Compute the set intersection of two identifier sets. */
      [[nodiscard]] static feature_ids_t intersection_of(const feature_ids_t& lhs,
                                                         const feature_ids_t& rhs)
      {
        feature_ids_t result {};
        const auto word_count = std::min(lhs.m_words.size(), rhs.m_words.size());
        result.m_words.resize(word_count, 0);
        for (size_type index { 0 }; index < word_count; ++index)
        {
          result.m_words.at(index) = lhs.m_words.at(index) & rhs.m_words.at(index);
        }
        result.recount();
        result.trim_trailing_zero_words();
        return result;
      }

      /** @brief Compute the set union of two identifier sets. */
      [[nodiscard]] static feature_ids_t union_of(const feature_ids_t& lhs,
                                                  const feature_ids_t& rhs)
      {
        feature_ids_t result {};
        const auto word_count = std::max(lhs.m_words.size(), rhs.m_words.size());
        result.m_words.resize(word_count, 0);
        for (size_type index { 0 }; index < word_count; ++index)
        {
          const auto lhs_word
              = index < lhs.m_words.size() ? lhs.m_words.at(index) : word_type { 0 };
          const auto rhs_word
              = index < rhs.m_words.size() ? rhs.m_words.at(index) : word_type { 0 };
          result.m_words.at(index) = lhs_word | rhs_word;
        }
        result.recount();
        result.trim_trailing_zero_words();
        return result;
      }

      /** @brief Compute the set difference @p lhs \ @p rhs. */
      [[nodiscard]] static feature_ids_t difference_of(const feature_ids_t& lhs,
                                                       const feature_ids_t& rhs)
      {
        feature_ids_t result {};
        result.m_words.resize(lhs.m_words.size(), 0);
        for (size_type index { 0 }; index < lhs.m_words.size(); ++index)
        {
          const auto rhs_word
              = index < rhs.m_words.size() ? rhs.m_words.at(index) : word_type { 0 };
          result.m_words.at(index) = lhs.m_words.at(index) & ~rhs_word;
        }
        result.recount();
        result.trim_trailing_zero_words();
        return result;
      }

      /** @brief Report whether the two sets overlap. */
      [[nodiscard]] static bool overlaps(const feature_ids_t& lhs,
                                         const feature_ids_t& rhs)
      {
        const auto word_count = std::min(lhs.m_words.size(), rhs.m_words.size());
        for (size_type index { 0 }; index < word_count; ++index)
        {
          if ((lhs.m_words.at(index) & rhs.m_words.at(index)) != 0)
          {
            return true;
          }
        }
        return false;
      }

      /** @brief Report whether @p lhs is a subset of @p rhs. */
      [[nodiscard]] static bool subset_of(const feature_ids_t& lhs,
                                          const feature_ids_t& rhs)
      {
        const auto word_count = std::max(lhs.m_words.size(), rhs.m_words.size());
        for (size_type index { 0 }; index < word_count; ++index)
        {
          const auto lhs_word
              = index < lhs.m_words.size() ? lhs.m_words.at(index) : word_type { 0 };
          const auto rhs_word
              = index < rhs.m_words.size() ? rhs.m_words.at(index) : word_type { 0 };
          if ((lhs_word & ~rhs_word) != 0)
          {
            return false;
          }
        }
        return true;
      }

    private:
      /** @brief Storage word type for the internal bitset representation. */
      using word_type = std::uint64_t;
      /** @brief Number of bits per storage word. */
      static constexpr size_type bits_per_word = sizeof(word_type) * CHAR_BIT;

      std::vector<word_type> m_words {}; /**< Bitset words encoding the stored identifiers. */
      size_type              m_size { 0 }; /**< Cached set size. */

      /** @brief Return the current representable bit capacity. */
      [[nodiscard]] size_type bit_capacity() const
      {
        return m_words.size() * bits_per_word;
      }

      /** @brief Find the next set bit at or after @p start. */
      [[nodiscard]] size_type next_set_bit(size_type start) const
      {
        if (m_words.empty())
        {
          return 0;
        }
        auto word_index = start / bits_per_word;
        if (word_index >= m_words.size())
        {
          return bit_capacity();
        }

        auto bit_offset = start % bits_per_word;
        auto word = m_words.at(word_index) & (~word_type { 0 } << bit_offset);
        while (true)
        {
          if (word != 0)
          {
            const auto bit_in_word = static_cast<size_type>(std::countr_zero(word));
            return word_index * bits_per_word + bit_in_word;
          }
          ++word_index;
          if (word_index >= m_words.size())
          {
            return bit_capacity();
          }
          word = m_words.at(word_index);
        }
      }

      /** @brief Recompute the cached set size from the storage words. */
      void recount()
      {
        m_size = 0;
        for (const auto word : m_words)
        {
          m_size += static_cast<size_type>(std::popcount(word));
        }
      }

      /** @brief Remove trailing zero-only storage words. */
      void trim_trailing_zero_words()
      {
        while (!m_words.empty() && m_words.back() == 0)
        {
          m_words.pop_back();
        }
      }

      /** @brief Compare internal bit patterns modulo trailing zeros. */
      [[nodiscard]] bool words_equal(const feature_ids_t& other) const
      {
        const auto word_count = std::max(m_words.size(), other.m_words.size());
        for (size_type index { 0 }; index < word_count; ++index)
        {
          const auto lhs_word
              = index < m_words.size() ? m_words.at(index) : word_type { 0 };
          const auto rhs_word
              = index < other.m_words.size() ? other.m_words.at(index) : word_type { 0 };
          if (lhs_word != rhs_word)
          {
            return false;
          }
        }
        return true;
      }
  };

  /** @brief Identifier set used for systems. */
  struct system_ids_t: ids_t
  { };

  /** @brief Classification of a derived feature in the report model. */
  enum class theory_feature_category_t
  {
    unclassified, /**< Category has not yet been assigned. */
    impossible, /**< Presence vector is not a valid derived feature. */
    core, /**< Feature occurs in every valid system. */
    fundamental, /**< Feature corresponds to a fundamental feature. */
    not_feature, /**< Feature simplifies to a negated fundamental feature. */
    and_feature, /**< Feature simplifies to a conjunction of literals. */
    or_feature, /**< Feature simplifies to a disjunction of literals. */
    complex /**< Feature does not reduce to the basic categories. */
  };

  /** @brief Registry entry for one derived feature. */
  struct feature_record_t
  {
      unsigned_integer          id { zero }; /**< One-based derived feature identifier. */
      feature_t                 systems_present {}; /**< Presence vector over valid systems. */
      theory_feature_category_t category {
        theory_feature_category_t::unclassified
      };
      std::string raw_term {}; /**< Unsimplified textual term, if materialized. */
      std::string simplified_term {}; /**< Simplified textual term, if materialized. */
      std::string display_name {}; /**< Preferred display label. */
  };

  /** @brief Partition of the system space into valid, available, and missing systems. */
  struct system_space_t
  {
      system_ids_t valid {}; /**< All valid systems. */
      system_ids_t available {}; /**< Valid systems that are available. */
      system_ids_t missing {}; /**< Valid systems that are missing. */
  };

  /** @brief Serialize a feature presence vector. */
  std::string to_string(const feature_t& fd);
  /** @brief Serialize a feature category to its display name. */
  std::string to_string(theory_feature_category_t category);
  /** @brief Return the active field separator, defaulting to the global CSV separator when @p s is zero. */
  [[nodiscard]] char separator(const char s = 0);

  /** @brief Convert a one-based feature id to the paper-style zero-based index. */
  [[nodiscard]] constexpr unsigned_integer paper_feature_index(unsigned_integer feature_id)
  {
    return feature_id - one;
  }

  /** @brief Format a feature id as the paper-style label t_i. */
  [[nodiscard]] inline std::string paper_feature_label(unsigned_integer feature_id)
  {
    return "t" + std::to_string(paper_feature_index(feature_id));
  }

  /** @brief Format a system id as the paper-style label s_i. */
  [[nodiscard]] inline std::string paper_system_label(unsigned_integer system_id)
  {
    return "s" + std::to_string(system_id - one);
  }

  /** @brief Registry entry for one system and the derived features present in it. */
  class system_t
  {
    public:
      feature_ids_t feature_ids {}; /**< Derived features present in the system. */
      /** @brief Return the number of derived features present in the system. */
      [[nodiscard]] unsigned_integer T() const;
      /** @brief Return all derived feature identifiers present in the system. */
      [[nodiscard]] const feature_ids_t& all_features() const;
  };

  /** @brief Sequence of system registry entries. */
  using systems_t = std::vector<system_t>;

  /** @brief High-level report modes supported by the analysis/CLI pipeline. */
  enum class report_mode_t
  {
    full, /**< Full report output. */
    summary, /**< Summary-oriented output. */
    isolation_only, /**< Isolation-only output. */
    selected_features /**< Restricted output for selected categories/features. */
  };

  /** @brief Planner risk buckets for report execution. */
  enum class feasibility_risk_t
  {
    safe, /**< Request is comfortably executable. */
    large, /**< Request is large but still expected to be manageable. */
    risky, /**< Request may be expensive or produce a large output. */
    impractical /**< Request should generally be reduced before execution. */
  };

  /** @brief Normalized report request consumed by analysis and isolation. */
  struct report_request_t final
  {
      report_mode_t mode { report_mode_t::full }; /**< Requested output mode. */
      std::optional<std::vector<theory_feature_category_t>> requested_categories {}; /**< Optional category restriction. */
      bool include_complex_details { true }; /**< Whether complex-feature details may be materialized. */
      bool include_raw_terms { true }; /**< Whether unsimplified terms may be printed. */
      bool include_simplified_terms { true }; /**< Whether simplified terms may be printed. */
      bool include_isolation { true }; /**< Whether isolation output is requested. */
      bool minimal_differences_only { true }; /**< Whether only minimal witness differences should be shown. */
      bool include_feature_names { false }; /**< Whether feature names should accompany paper labels. */

      /** @brief Construct the default unrestricted request. */
      [[nodiscard]] static report_request_t unrestricted();
      /** @brief Build a report request from an optional category filter hint. */
      [[nodiscard]] static report_request_t
          from_category_hint(
              std::optional<std::vector<theory_feature_category_t>> categories);
      /** @brief Report whether the request includes a given category. */
      [[nodiscard]] bool includes_category(
          theory_feature_category_t category) const;
  };

  /** @brief Planner estimate for the cost and risk of a requested report. */
  struct report_feasibility_t final
  {
      unsigned_integer base_feature_count { 0 }; /**< Number of fundamental features. */
      unsigned_integer valid_system_count { 0 }; /**< Number of valid systems. */
      unsigned_integer theory_feature_count { 0 }; /**< Total number of derived features. */
      unsigned_integer simple_feature_upper_bound { 0 }; /**< Upper bound on simple features. */
      unsigned_integer complex_feature_lower_bound { 0 }; /**< Lower bound on complex features. */
      unsigned_integer included_feature_upper_bound { 0 }; /**< Upper bound on features included in the report. */
      bool             complex_workload_uncertain { false }; /**< Whether complex-detail cost remains too uncertain for a confident default recommendation. */
      long double      estimated_analysis_runtime_ms { 0.0L }; /**< Estimated analysis runtime in milliseconds. */
      long double      estimated_isolation_runtime_ms { 0.0L }; /**< Estimated isolation runtime in milliseconds. */
      long double      estimated_total_runtime_ms { 0.0L }; /**< Combined runtime estimate in milliseconds. */
      long double      estimated_output_bytes { 0.0L }; /**< Estimated output size in bytes. */
      feasibility_risk_t risk { feasibility_risk_t::safe }; /**< Risk bucket assigned by the planner. */
      bool               approval_recommended { false }; /**< Whether explicit approval is recommended before execution. */
      std::vector<std::string> observations {}; /**< Human-readable planner observations. */

      /** @brief Render a compact feasibility summary. */
      /** @brief Render a compact execution-plan summary. */
      [[nodiscard]] std::string summary() const;
  };

  /** @brief One alternative execution option proposed by the planner. */
  struct report_plan_option_t final
  {
      std::string                       label {}; /**< Short user-facing label. */
      std::string                       rationale {}; /**< Explanation for the option. */
      std::optional<report_request_t>   request {}; /**< Concrete request, when executable. */
      std::optional<report_feasibility_t> feasibility {}; /**< Feasibility associated with @ref request. */
      bool                              executable_now { true }; /**< Whether the option can be executed immediately. */
  };

  /** @brief Planner output combining the requested report with recommendations and alternatives. */
  struct report_execution_plan_t final
  {
      report_request_t                  requested_request {}; /**< Original request. */
      report_feasibility_t              requested_feasibility {}; /**< Feasibility of the original request. */
      report_request_t                  recommended_request {}; /**< Planner-recommended request. */
      report_feasibility_t              recommended_feasibility {}; /**< Feasibility of the recommendation. */
      bool                              adjustment_recommended { false }; /**< Whether the requested report should be adjusted. */
      std::string                       rationale {}; /**< Planner rationale for the recommendation. */
      std::vector<report_plan_option_t> alternatives {}; /**< Alternative options. */

      /** @brief Render a compact feasibility summary. */
      /** @brief Render a compact execution-plan summary. */
      [[nodiscard]] std::string summary() const;
  };

  /** @brief Format a runtime estimate in human-readable units. */
  [[nodiscard]] std::string human_readable_duration(long double runtime_ms);
  /** @brief Format an output-size estimate in human-readable units. */
  [[nodiscard]] std::string human_readable_size(long double bytes);

  /** @brief Serialize a report mode. */
  [[nodiscard]] std::string to_string(report_mode_t mode);
  /** @brief Serialize a feasibility risk bucket. */
  [[nodiscard]] std::string to_string(feasibility_risk_t risk);
  /** @brief Estimate cost, size, and risk for a requested report. */
  [[nodiscard]] report_feasibility_t
      estimate_report_feasibility(unsigned_integer      base_feature_count,
                                  const system_space_t& system_space,
                                  const report_request_t& report_request);
  /** @brief Produce a recommended execution plan for a requested report. */
  [[nodiscard]] report_execution_plan_t
      plan_report_execution(unsigned_integer      base_feature_count,
                            const system_space_t& system_space,
                            const report_request_t& report_request);

  /**
   * @brief Core derived-feature analysis engine.
   *
   * The class materializes the derived feature registry, classifies derived
   * features, derives per-system memberships, and prints the report views used
   * by the CLI.
   */
  class feature_analysis_t
  {
    public:
      //TODO
      std::string isolation_string{"TODO"};

      [[nodiscard]] std::vector<feature_record_t> initialize_feature_registry(std::vector<std::string> systems_from_alignment);

      explicit feature_analysis_t(
          unsigned_integer                                  f,
          const system_space_t&                             system_space,
          std::vector<std::string> systems_found_in_alignment,
          std::vector<std::string>                          feature_names = {},
          report_request_t                                  report_request
              = report_request_t::unrestricted());

      /**
       * @brief Construct the analysis from a feature count, system space, optional feature names, and request.
       * @param f Number of fundamental features.
       * @param system_space Normalized valid/available/missing system partition to analyze.
       * @param feature_names Optional display names for the fundamental features. Missing names fall back
       * to generated identifiers.
       * @param report_request Normalized request that decides which categories and detail columns are materialized.
       */
      explicit feature_analysis_t(
          unsigned_integer                                  f,
          const system_space_t&                             system_space,
          std::vector<std::string>                          feature_names = {},
          report_request_t                                  report_request
              = report_request_t::unrestricted());
      /**
       * @brief Compatibility constructor that derives a report request from an optional category filter.
       * @param f Number of fundamental features.
       * @param system_space Normalized valid/available/missing system partition to analyze.
       * @param feature_names Optional display names for the fundamental features.
       * @param category_filter Optional legacy category hint converted to a normalized @ref report_request_t.
       */
      explicit feature_analysis_t(
          unsigned_integer                                  f,
          const system_space_t&                             system_space,
          std::vector<std::string>                          feature_names,
          std::optional<std::vector<theory_feature_category_t>> category_filter);

      /**
       * @brief Return the presence vector for a one-based derived feature id.
       * @throws std::out_of_range If @p feature_id is zero or larger than @ref feature_count().
       */
      [[nodiscard]] feature_t         at(unsigned_integer feature_id) const;
      /**
       * @brief Return the numeric category code for a one-based derived feature id.
       * @throws std::out_of_range If @p feature_id is zero or larger than @ref feature_count().
       */
      [[nodiscard]] unsigned_integer  category(unsigned_integer feature_id) const;
      /**
       * @brief Return the display name of the category assigned to a one-based derived feature id.
       * @throws std::out_of_range If @p feature_id is zero or larger than @ref feature_count().
       */
      [[nodiscard]] std::string       category_name(unsigned_integer feature_id) const;
      /**
       * @brief Report whether the implementation exposes registry-backed derived-feature metadata.
       * @note The current implementation always returns @c true.
       */
      [[nodiscard]] bool uses_theory_registry() const;
      /** @brief Report whether the request includes a given category. */
      [[nodiscard]] bool includes_category(
          theory_feature_category_t category) const;
      /**
       * @brief Report whether the current request includes the category of a one-based derived feature id.
       * @throws std::out_of_range If @p feature_id is zero or larger than @ref feature_count().
       */
      [[nodiscard]] bool includes_feature(unsigned_integer feature_id) const;
      /** @brief Return the analyzed system space. */
      [[nodiscard]] const system_space_t& system_space() const;
      /** @brief Return the normalized report request. */
      [[nodiscard]] const report_request_t& report_request() const;
      /** @brief Return the derived-feature registry. */
      [[nodiscard]] const std::vector<feature_record_t>& feature_registry() const;
      /** @brief Print one derived-feature presence vector. */
      void print_feature(std::ostream& os, const feature_t& feature) const;
      /** @brief Print the full derived-feature table. */
      void print_features(std::ostream& os) const;
      /** @brief Print per-category feature groupings. */
      void print_categories(std::ostream& os) const;
      /** @brief Print derived-feature statistics for one system. */
      void print_system_statistics(std::ostream&    os,
                                   const system_t&  system,
                                   unsigned_integer id) const;
      /** @brief Print statistics for all systems. */
      void print_systems_statistics(std::ostream& os) const;
      /** @brief Print the derived features present in one system. */
      void print_system_features(std::ostream&    os,
                                 const system_t&  system,
                                 unsigned_integer system_id) const;
      /** @brief Print derived-feature memberships for all systems. */
      void print_systems_features(std::ostream& os) const;
      /** @brief Print the analysis according to the current request. */
      void print(std::ostream& os) const;
      /** @brief Return the number of derived features. */
      [[nodiscard]] unsigned_integer feature_count() const;
      /** @brief Return the number of system registry entries. */
      [[nodiscard]] unsigned_integer system_count() const;
      /** @brief Return the number of fundamental features tracked by the analysis. */
      [[nodiscard]] unsigned_integer base_feature_count() const;
      /** @brief Return the number of valid systems. */
      [[nodiscard]] unsigned_integer valid_system_count() const;
      /** @brief Return the sparse system registry entry for one valid system id. */
      [[nodiscard]] const system_t& system_by_id(unsigned_integer system_id) const;

    private:
      /** @brief Resolve the display name for a fundamental feature index. */
      [[nodiscard]] std::string feature_name(unsigned_integer feature_index) const;
      /** @brief Build the unsimplified textual term for a presence vector. */
      [[nodiscard]] std::string raw_term_for(const feature_t& systems_present) const;
      /** @brief Build the simplified textual term for a presence vector. */
      [[nodiscard]] std::string simplified_term_for(
          const feature_t& systems_present,
          const std::vector<unsigned_integer>& dont_cares) const;
      /** @brief Classify a simplified term into the documented feature categories. */
      [[nodiscard]] theory_feature_category_t
          classify_simplified_term(const std::string& simplified_term) const;
      /** @brief Report whether full details should be materialized for a category. */
      [[nodiscard]] bool should_materialize_details(
          theory_feature_category_t category) const;
      /** @brief Report whether simplified terms should be materialized for a category. */
      [[nodiscard]] bool should_materialize_simplified_term(
          theory_feature_category_t category) const;
      /** @brief Report whether raw terms should be materialized for a category. */
      [[nodiscard]] bool should_materialize_raw_term(
          theory_feature_category_t category) const;
      /** @brief Build the full derived-feature registry. */
      [[nodiscard]] std::vector<feature_record_t> initialize_feature_registry() const;
      /** @brief Build per-system derived-feature memberships from the registry. */
      [[nodiscard]] systems_t initialize_registry_backed_systems() const;

      const unsigned_integer              m_base_feature_count; /**< Number of fundamental features. */
      const unsigned_integer              m_valid_system_count; /**< Number of valid systems. */
      const unsigned_integer              m_max_system_id; /**< Largest system id representable in the current model. */
      const system_space_t                m_system_space; /**< Analyzed system space. */
      const std::vector<std::string>      m_feature_names; /**< Raw feature names provided by the specification. */
      const report_request_t              m_report_request; /**< Normalized report request. */
      const std::vector<std::string>      m_resolved_feature_names; /**< Resolved display names for the fundamental features. */
      const std::vector<unsigned_integer> m_ordered_valid_system_ids; /**< Valid system ids in iteration order. */
      const std::unordered_map<unsigned_integer, std::size_t> m_valid_system_positions; /**< Maps one-based valid system ids to sparse registry indices. */

    public:
      const std::vector<feature_record_t> m_feature_registry; /**< Registry of derived features in one-based id order. */
      const systems_t                     m_systems; /**< Per-system derived-feature memberships. */
  };

} // namespace feature_isolation

#endif // FEATURE_ANALYSIS_HPP
