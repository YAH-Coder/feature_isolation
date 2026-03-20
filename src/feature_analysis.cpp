/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "../include/feature_analysis.hpp"
#include <cstdint>
#include <deque>
#include <cmath>
#include <limits>
#include <map>

/** @file
 *  @brief Implements report-feasibility estimation, report planning, simplified-term derivation,
 *  and registry construction for the derived feature space.
 */

namespace feature_isolation
{
  report_request_t report_request_t::unrestricted()
  {
    return report_request_t {};
  }

  report_request_t report_request_t::from_category_hint(
      std::optional<std::vector<theory_feature_category_t>> categories)
  {
    report_request_t request {};
    request.requested_categories = std::move(categories);
    if (request.requested_categories.has_value())
    {
      request.include_complex_details
          = std::ranges::find(request.requested_categories.value(),
                              theory_feature_category_t::complex)
            != request.requested_categories.value().end();
    }
    return request;
  }

  bool report_request_t::includes_category(
      theory_feature_category_t category) const
  {
    if (!requested_categories.has_value())
    {
      return true;
    }
    return std::ranges::find(requested_categories.value(), category)
           != requested_categories.value().end();
  }


  std::string to_string(report_mode_t mode)
  {
    switch (mode)
    {
      case report_mode_t::full:
        return "full";
      case report_mode_t::summary:
        return "summary";
      case report_mode_t::isolation_only:
        return "isolation-only";
      case report_mode_t::selected_features:
        return "selected-features";
    }
    return "unknown";
  }

  std::string to_string(feasibility_risk_t risk)
  {
    switch (risk)
    {
      case feasibility_risk_t::safe:
        return "safe";
      case feasibility_risk_t::large:
        return "large";
      case feasibility_risk_t::risky:
        return "risky";
      case feasibility_risk_t::impractical:
        return "impractical";
    }
    return "unknown";
  }

  namespace
  {
    /** @brief Returns 2^exponent and saturates to the unsigned_integer maximum on overflow. */
    [[nodiscard]] unsigned_integer saturated_power2_count(
        unsigned_integer exponent)
    {
      if (exponent >= std::numeric_limits<unsigned_integer>::digits)
      {
        return std::numeric_limits<unsigned_integer>::max();
      }
      return unsigned_integer { 1 } << exponent;
    }

    /** @brief Returns 3^exponent and saturates to the unsigned_integer maximum on overflow. */
    [[nodiscard]] unsigned_integer saturated_power3_count(
        unsigned_integer exponent)
    {
      unsigned_integer value { 1 };
      for (auto index { zero }; index < exponent; ++index)
      {
        if (value > std::numeric_limits<unsigned_integer>::max() / 3)
        {
          return std::numeric_limits<unsigned_integer>::max();
        }
        value *= 3;
      }
      return value;
    }

    /** @brief Adds two counts and saturates to the unsigned_integer maximum on overflow. */
    [[nodiscard]] unsigned_integer saturated_add(unsigned_integer lhs,
                                                unsigned_integer rhs)
    {
      if (lhs > std::numeric_limits<unsigned_integer>::max() - rhs)
      {
        return std::numeric_limits<unsigned_integer>::max();
      }
      return lhs + rhs;
    }

    /** @brief Computes a conservative upper bound for one derived-feature category. */
    [[nodiscard]] unsigned_integer category_upper_bound(
        unsigned_integer          base_feature_count,
        unsigned_integer          theory_feature_count,
        theory_feature_category_t category)
    {
      const auto conjunction_patterns
          = saturated_power3_count(base_feature_count);
      const auto higher_arity_simple_count
          = conjunction_patterns == std::numeric_limits<unsigned_integer>::max()
                ? conjunction_patterns
                : (conjunction_patterns > 1 + 2 * base_feature_count
                       ? conjunction_patterns - 1 - 2 * base_feature_count
                       : 0);

      unsigned_integer bound { 0 };
      switch (category)
      {
        case theory_feature_category_t::impossible:
        case theory_feature_category_t::core:
          bound = 1;
          break;
        case theory_feature_category_t::fundamental:
        case theory_feature_category_t::not_feature:
          bound = base_feature_count;
          break;
        case theory_feature_category_t::and_feature:
        case theory_feature_category_t::or_feature:
          bound = higher_arity_simple_count;
          break;
        case theory_feature_category_t::complex:
          bound = theory_feature_count;
          break;
        default:
          bound = theory_feature_count;
          break;
      }
      return std::min(bound, theory_feature_count);
    }

    /** @brief Appends an observation string only once. */
    void append_observation(std::vector<std::string>& observations,
                            const std::string&        message)
    {
      if (std::ranges::find(observations, message) == observations.end())
      {
        observations.push_back(message);
      }
    }
  } // namespace

  report_feasibility_t estimate_report_feasibility(
      unsigned_integer        base_feature_count,
      const system_space_t&   system_space,
      const report_request_t& report_request)
  {
    constexpr long double shell_runtime_ms_per_feature { 0.00045L };
    constexpr long double simple_detail_runtime_ms_per_feature { 0.01L };
    constexpr long double complex_detail_runtime_ms_per_feature { 0.042L };
    constexpr long double base_output_bytes { 4096.0L };
    constexpr long double shell_output_bytes_per_feature { 48.0L };
    constexpr long double simplified_term_output_bytes_per_feature { 96.0L };
    constexpr long double raw_term_output_bytes_per_feature { 96.0L };
    constexpr long double isolation_output_bytes_per_feature { 128.0L };

    report_feasibility_t estimate {};
    estimate.base_feature_count = base_feature_count;
    estimate.valid_system_count = system_space.valid.size();
    estimate.theory_feature_count
        = saturated_power2_count(estimate.valid_system_count);

    unsigned_integer simple_feature_upper_bound { 0 };
    for (const auto category : { theory_feature_category_t::impossible,
                                 theory_feature_category_t::core,
                                 theory_feature_category_t::fundamental,
                                 theory_feature_category_t::not_feature,
                                 theory_feature_category_t::and_feature,
                                 theory_feature_category_t::or_feature })
    {
      simple_feature_upper_bound
          = saturated_add(simple_feature_upper_bound,
                          category_upper_bound(base_feature_count,
                                               estimate.theory_feature_count,
                                               category));
    }
    simple_feature_upper_bound
        = std::min(estimate.theory_feature_count, simple_feature_upper_bound);
    estimate.simple_feature_upper_bound = simple_feature_upper_bound;
    estimate.complex_feature_lower_bound
        = estimate.theory_feature_count > simple_feature_upper_bound
              ? estimate.theory_feature_count - simple_feature_upper_bound
              : 0;

    if (report_request.mode == report_mode_t::summary)
    {
      estimate.included_feature_upper_bound = 0;
    }
    else if (!report_request.requested_categories.has_value())
    {
      estimate.included_feature_upper_bound = estimate.theory_feature_count;
    }
    else
    {
      unsigned_integer requested_upper_bound { 0 };
      for (const auto category : report_request.requested_categories.value())
      {
        requested_upper_bound
            = saturated_add(requested_upper_bound,
                            category_upper_bound(base_feature_count,
                                                 estimate.theory_feature_count,
                                                 category));
      }
      estimate.included_feature_upper_bound
          = std::min(estimate.theory_feature_count, requested_upper_bound);
    }

    const auto simple_included_upper_bound
        = std::min(estimate.included_feature_upper_bound,
                   estimate.simple_feature_upper_bound);
    const auto complex_included_upper_bound
        = estimate.included_feature_upper_bound > simple_included_upper_bound
              ? estimate.included_feature_upper_bound - simple_included_upper_bound
              : 0;

    estimate.estimated_analysis_runtime_ms
        = static_cast<long double>(estimate.theory_feature_count)
          * shell_runtime_ms_per_feature;
    if (report_request.mode != report_mode_t::summary)
    {
      estimate.estimated_analysis_runtime_ms
          += static_cast<long double>(simple_included_upper_bound)
             * simple_detail_runtime_ms_per_feature;
      if (report_request.include_complex_details)
      {
        estimate.estimated_analysis_runtime_ms
            += static_cast<long double>(complex_included_upper_bound)
               * complex_detail_runtime_ms_per_feature;
      }
    }

    if (report_request.include_isolation
        && report_request.mode != report_mode_t::summary)
    {
      const auto isolation_per_feature_ms
          = 0.005L * std::pow(static_cast<long double>(estimate.valid_system_count), 3.0L);
      estimate.estimated_isolation_runtime_ms
          = static_cast<long double>(estimate.included_feature_upper_bound)
            * isolation_per_feature_ms;
    }

    estimate.estimated_total_runtime_ms
        = estimate.estimated_analysis_runtime_ms
          + estimate.estimated_isolation_runtime_ms;

    estimate.estimated_output_bytes = base_output_bytes;
    if (report_request.mode != report_mode_t::summary)
    {
      estimate.estimated_output_bytes
          += static_cast<long double>(estimate.included_feature_upper_bound)
             * shell_output_bytes_per_feature;
      if (report_request.include_simplified_terms)
      {
        estimate.estimated_output_bytes
            += static_cast<long double>(estimate.included_feature_upper_bound)
               * simplified_term_output_bytes_per_feature;
      }
      if (report_request.include_raw_terms)
      {
        estimate.estimated_output_bytes
            += static_cast<long double>(estimate.included_feature_upper_bound)
               * raw_term_output_bytes_per_feature;
      }
      if (report_request.include_isolation)
      {
        estimate.estimated_output_bytes
            += static_cast<long double>(estimate.included_feature_upper_bound)
               * isolation_output_bytes_per_feature;
      }
    }

    constexpr long double mib { 1024.0L * 1024.0L };
    constexpr long double gib { 1024.0L * 1024.0L * 1024.0L };
    if (estimate.theory_feature_count == std::numeric_limits<unsigned_integer>::max()
        || estimate.estimated_total_runtime_ms > 300000.0L
        || estimate.estimated_output_bytes > gib)
    {
      estimate.risk = feasibility_risk_t::impractical;
    }
    else if (estimate.estimated_total_runtime_ms > 60000.0L
             || estimate.estimated_output_bytes > 256.0L * mib
             || estimate.theory_feature_count > saturated_power2_count(24))
    {
      estimate.risk = feasibility_risk_t::risky;
    }
    else if (estimate.estimated_total_runtime_ms > 10000.0L
             || estimate.estimated_output_bytes > 64.0L * mib
             || estimate.theory_feature_count > saturated_power2_count(20))
    {
      estimate.risk = feasibility_risk_t::large;
    }
    else
    {
      estimate.risk = feasibility_risk_t::safe;
    }

    const auto complex_share_lower_bound
        = estimate.theory_feature_count == 0
              ? 0.0L
              : static_cast<long double>(estimate.complex_feature_lower_bound)
                    / static_cast<long double>(estimate.theory_feature_count);

    if (estimate.valid_system_count >= 12)
    {
      append_observation(
          estimate.observations,
          "Many valid systems enlarge the derived feature space quickly.");
    }
    if (estimate.theory_feature_count > saturated_power2_count(20))
    {
      append_observation(
          estimate.observations,
          "The derived feature space is already large enough that a summary-first workflow is often easier to inspect.");
    }
    if (report_request.include_complex_details
        && report_request.mode != report_mode_t::summary
        && estimate.complex_feature_lower_bound > 0
        && (estimate.complex_feature_lower_bound >= estimate.simple_feature_upper_bound
            || complex_share_lower_bound >= 0.5L))
    {
      append_observation(
          estimate.observations,
          "Complex derived features are likely to dominate the requested detail work.");
    }
    if (!report_request.include_complex_details)
    {
      append_observation(
          estimate.observations,
          "Skipping complex-feature details keeps the same feature IDs while reducing expensive detail generation.");
    }
    if (report_request.include_isolation
        && report_request.mode != report_mode_t::summary
        && estimate.valid_system_count >= 8)
    {
      append_observation(
          estimate.observations,
          "Isolation output becomes more expensive as the number of valid systems grows.");
    }
    if (report_request.mode == report_mode_t::summary)
    {
      append_observation(
          estimate.observations,
          "Summary mode gives a first overview of the derived feature space without materializing per-feature detail sections.");
    }
    if (report_request.requested_categories.has_value()
        && report_request.requested_categories->size() <= 2)
    {
      append_observation(
          estimate.observations,
          "Restricting the report to a small set of feature categories is a good drill-down strategy after the first overview.");
    }
    if (estimate.estimated_output_bytes > 64.0L * mib)
    {
      append_observation(
          estimate.observations,
          "The projected report file is large enough that inspection may be harder than generation.");
    }

    estimate.approval_recommended = estimate.risk != feasibility_risk_t::safe;
    return estimate;
  }

  std::string report_feasibility_t::summary() const
  {
    std::ostringstream os {};
    os << "fundamental features=" << base_feature_count << ", valid systems="
       << valid_system_count << ", derived features≈" << theory_feature_count
       << ", included features≤" << included_feature_upper_bound
       << ", complex features≥" << complex_feature_lower_bound
       << ", estimated runtime≈" << estimated_total_runtime_ms << " ms"
       << ", estimated output≈" << estimated_output_bytes << " bytes"
       << ", risk=" << to_string(risk);
    return os.str();
  }

  namespace
  {
    /** @brief Maps feasibility risks to an ordering used by the planner. */
    [[nodiscard]] int risk_rank(feasibility_risk_t risk)
    {
      switch (risk)
      {
        case feasibility_risk_t::safe:
          return 0;
        case feasibility_risk_t::large:
          return 1;
        case feasibility_risk_t::risky:
          return 2;
        case feasibility_risk_t::impractical:
          return 3;
      }
      return 4;
    }

    /** @brief Returns the report categories that exclude the complex remainder of the derived feature space. */
    [[nodiscard]] std::vector<theory_feature_category_t>
        all_non_complex_categories()
    {
      return {
        theory_feature_category_t::impossible,
        theory_feature_category_t::core,
        theory_feature_category_t::fundamental,
        theory_feature_category_t::not_feature,
        theory_feature_category_t::and_feature,
        theory_feature_category_t::or_feature
      };
    }

    /** @brief Derives a lighter request by dropping the complex category and its detailed materialization. */
    [[nodiscard]] report_request_t
        make_non_complex_request(const report_request_t& base)
    {
      auto request { base };
      request.include_complex_details = false;
      if (!request.requested_categories.has_value())
      {
        request.requested_categories = all_non_complex_categories();
      }
      else
      {
        auto categories { request.requested_categories.value() };
        categories.erase(std::remove(categories.begin(),
                                     categories.end(),
                                     theory_feature_category_t::complex),
                         categories.end());
        request.requested_categories = std::move(categories);
      }
      return request;
    }

    /** @brief Derives a request that keeps the same feature selection but omits isolation computation. */
    [[nodiscard]] report_request_t
        make_no_isolation_request(const report_request_t& base)
    {
      auto request { base };
      request.include_isolation = false;
      return request;
    }

    /** @brief Derives a summary-only request from an existing report request. */
    [[nodiscard]] report_request_t make_summary_request(const report_request_t& base)
    {
      auto request { base };
      request.mode                     = report_mode_t::summary;
      request.include_complex_details  = false;
      request.include_raw_terms        = false;
      request.include_simplified_terms = false;
      request.include_isolation        = false;
      return request;
    }

    /** @brief Packages one planner alternative for CLI presentation. */
    [[nodiscard]] report_plan_option_t make_option(
        std::string                       label,
        std::string                       rationale,
        std::optional<report_request_t>   request,
        std::optional<report_feasibility_t> feasibility,
        bool executable_now = true)
    {
      return report_plan_option_t {
        .label = std::move(label),
        .rationale = std::move(rationale),
        .request = std::move(request),
        .feasibility = std::move(feasibility),
        .executable_now = executable_now
      };
    }

    /** @brief Returns whether two report requests are semantically identical for planning purposes. */
    [[nodiscard]] bool same_request(const report_request_t& lhs,
                                    const report_request_t& rhs)
    {
      return lhs.mode == rhs.mode
             && lhs.requested_categories == rhs.requested_categories
             && lhs.include_complex_details == rhs.include_complex_details
             && lhs.include_raw_terms == rhs.include_raw_terms
             && lhs.include_simplified_terms == rhs.include_simplified_terms
             && lhs.include_isolation == rhs.include_isolation
             && lhs.minimal_differences_only == rhs.minimal_differences_only
             && lhs.include_feature_names == rhs.include_feature_names;
    }
  }

  report_execution_plan_t plan_report_execution(
      unsigned_integer        base_feature_count,
      const system_space_t&   system_space,
      const report_request_t& report_request)
  {
    const auto requested_feasibility = estimate_report_feasibility(
        base_feature_count, system_space, report_request);

    /** @brief Holds one executable planner fallback candidate together with its measured feasibility. */
    struct executable_candidate_t
    {
        std::string         label {};
        std::string         rationale {};
        report_request_t    request {};
        report_feasibility_t feasibility {};
    };

    std::vector<executable_candidate_t> executable_candidates {};
    executable_candidates.push_back({
        "requested plan",
        "Preserve the report exactly as requested.",
        report_request,
        requested_feasibility });

    if (report_request.mode != report_mode_t::summary)
    {
      const auto non_complex_request = make_non_complex_request(report_request);
      if (!same_request(non_complex_request, report_request))
      {
        executable_candidates.push_back({
            "filtered report without complex details",
            "Keep exact feature IDs, but focus the first report on the non-complex part of the derived feature space.",
            non_complex_request,
            estimate_report_feasibility(
                base_feature_count, system_space, non_complex_request) });
      }

      if (report_request.include_isolation)
      {
        const auto lighter_request = make_no_isolation_request(non_complex_request);
        if (!same_request(lighter_request, report_request)
            && (executable_candidates.empty()
                || !same_request(lighter_request,
                                 executable_candidates.back().request)))
        {
          executable_candidates.push_back({
              "filtered report without complex or isolation details",
              "Start with the lighter theory categories and defer isolation until after the first report review.",
              lighter_request,
              estimate_report_feasibility(
                  base_feature_count, system_space, lighter_request) });
        }
      }
    }

    const auto summary_request = make_summary_request(report_request);
    if (!same_request(summary_request, report_request))
    {
      executable_candidates.push_back({
          "summary report",
          "Start with a compact overview of the derived feature space before drilling into category-specific details.",
          summary_request,
          estimate_report_feasibility(
              base_feature_count, system_space, summary_request) });
    }

    auto best_candidate = executable_candidates.front();
    if (requested_feasibility.risk == feasibility_risk_t::risky
        || requested_feasibility.risk == feasibility_risk_t::impractical)
    {
      for (const auto& candidate : executable_candidates)
      {
        const auto candidate_rank { risk_rank(candidate.feasibility.risk) };
        const auto best_rank { risk_rank(best_candidate.feasibility.risk) };
        if (candidate_rank < best_rank)
        {
          best_candidate = candidate;
          continue;
        }
        if (candidate_rank == best_rank
            && candidate.feasibility.estimated_total_runtime_ms
                   < best_candidate.feasibility.estimated_total_runtime_ms)
        {
          best_candidate = candidate;
        }
      }
    }

    report_execution_plan_t plan {};
    plan.requested_request      = report_request;
    plan.requested_feasibility  = requested_feasibility;
    plan.recommended_request    = best_candidate.request;
    plan.recommended_feasibility = best_candidate.feasibility;
    plan.adjustment_recommended = !same_request(plan.recommended_request,
                                                plan.requested_request);
    if (plan.adjustment_recommended)
    {
      plan.rationale = best_candidate.rationale;
    }
    else if (requested_feasibility.risk == feasibility_risk_t::safe)
    {
      plan.rationale = "The requested report fits within the current conservative feasibility limits for this derived feature space.";
    }
    else if (!requested_feasibility.observations.empty())
    {
      plan.rationale = requested_feasibility.observations.front();
    }
    else
    {
      plan.rationale = "The request is still executable, but the planner will surface lighter alternatives in the next CLI step.";
    }

    for (const auto& candidate : executable_candidates)
    {
      plan.alternatives.push_back(make_option(
          candidate.label,
          candidate.rationale,
          candidate.request,
          candidate.feasibility,
          true));
    }
    plan.alternatives.push_back(make_option(
        "selected feature IDs only",
        "For very large derived feature spaces, restrict generation to an explicit list or range of feature IDs.",
        std::nullopt,
        std::nullopt,
        false));
    plan.alternatives.push_back(make_option(
        "tighten SPL constraints first",
        "Reduce the valid-system space before generating a report; this shrinks the full derived feature space exactly.",
        std::nullopt,
        std::nullopt,
        false));
    return plan;
  }

  std::string report_execution_plan_t::summary() const
  {
    std::ostringstream os {};
    os << "requested mode=" << to_string(requested_request.mode)
       << ", requested risk=" << to_string(requested_feasibility.risk)
       << ", recommended mode=" << to_string(recommended_request.mode)
       << ", recommended risk=" << to_string(recommended_feasibility.risk);
    if (!rationale.empty())
    {
      os << ", rationale=" << rationale;
    }
    return os.str();
  }

  namespace
  {
    constexpr const char* logical_false_symbol = "⊥";
    constexpr const char* logical_true_symbol  = "⊤";
    constexpr const char* logical_not_symbol   = "¬";
    constexpr const char* logical_and_symbol   = " ∧ ";
    constexpr const char* logical_or_symbol    = " ∨ ";

    /** @brief Returns the stable category order used by report tables and registry materialization. */
    [[nodiscard]] const std::array<theory_feature_category_t, 7>&
        ordered_theory_categories()
    {
      static const std::array<theory_feature_category_t, 7> categories {
        theory_feature_category_t::impossible,
        theory_feature_category_t::core,
        theory_feature_category_t::fundamental,
        theory_feature_category_t::not_feature,
        theory_feature_category_t::and_feature,
        theory_feature_category_t::or_feature,
        theory_feature_category_t::complex
      };
      return categories;
    }


    /** @brief Stores the shortest currently known simple descriptor for one derived feature extent. */
    struct simple_term_descriptor_t
    {
        theory_feature_category_t category {
          theory_feature_category_t::unclassified
        };
        std::string expression {};
        size_t      literal_count { 0 };
    };

    using simple_term_index_t = std::map<feature_t, simple_term_descriptor_t>;

    /** @brief Renders a conjunction or disjunction from signed feature literals. */
    [[nodiscard]] std::string render_simple_literals(
        const std::vector<std::pair<std::string, bool>>& literals,
        const char* join_symbol)
    {
      if (literals.empty())
      {
        return std::string(logical_true_symbol);
      }

      std::vector<std::string> positive_literals {};
      std::vector<std::string> negative_literals {};
      positive_literals.reserve(literals.size());
      negative_literals.reserve(literals.size());
      for (const auto& [name, negated] : literals)
      {
        if (negated)
        {
          negative_literals.push_back(std::string(logical_not_symbol) + name);
        }
        else
        {
          positive_literals.push_back(name);
        }
      }
      std::ranges::sort(positive_literals);
      std::ranges::sort(negative_literals);

      std::vector<std::string> ordered_literals {};
      ordered_literals.reserve(positive_literals.size() + negative_literals.size());
      ordered_literals.insert(ordered_literals.end(),
                              positive_literals.begin(),
                              positive_literals.end());
      ordered_literals.insert(ordered_literals.end(),
                              negative_literals.begin(),
                              negative_literals.end());

      std::ostringstream os {};
      for (auto index { zero }; index < ordered_literals.size(); ++index)
      {
        if (index > zero)
        {
          os << join_symbol;
        }
        os << ordered_literals.at(index);
      }
      return os.str();
    }

    /** @brief Compares two simplified descriptors and prefers shorter, less negated forms. */
    [[nodiscard]] bool is_better_simple_descriptor(
        const simple_term_descriptor_t& candidate,
        const simple_term_descriptor_t& current)
    {
      if (current.category == theory_feature_category_t::unclassified)
      {
        return true;
      }
      if (candidate.literal_count != current.literal_count)
      {
        return candidate.literal_count < current.literal_count;
      }
      if (candidate.expression.size() != current.expression.size())
      {
        return candidate.expression.size() < current.expression.size();
      }
      return candidate.expression < current.expression;
    }

    /** @brief Updates the per-extent descriptor index when a better simple term is found. */
    void record_simple_descriptor(simple_term_index_t&            index,
                                  const feature_t&                extent,
                                  simple_term_descriptor_t        descriptor)
    {
      const auto existing = index.find(extent);
      if (existing == index.end())
      {
        index.emplace(extent, std::move(descriptor));
        return;
      }
      if (is_better_simple_descriptor(descriptor, existing->second))
      {
        existing->second = std::move(descriptor);
      }
    }

    struct simple_term_state_t
    {
        feature_t                  extent {};
        std::vector<std::int8_t>   literal_states {};
        simple_term_descriptor_t   descriptor {};
    };

    [[nodiscard]] feature_t literal_extent(unsigned_integer feature_index,
                                           bool negated,
                                           const std::vector<unsigned_integer>& ordered_valid_system_ids)
    {
      feature_t extent(ordered_valid_system_ids.size(), false);
      for (auto system_index { zero }; system_index < ordered_valid_system_ids.size(); ++system_index)
      {
        const auto minterm = ordered_valid_system_ids.at(system_index) - one;
        const bool bit_value = ((minterm >> feature_index) & one) == one;
        extent.at(system_index) = negated ? !bit_value : bit_value;
      }
      return extent;
    }

    [[nodiscard]] feature_t intersect_extents(const feature_t& lhs,
                                              const feature_t& rhs)
    {
      feature_t result(lhs.size(), false);
      for (auto index { zero }; index < lhs.size(); ++index)
      {
        result.at(index) = lhs.at(index) && rhs.at(index);
      }
      return result;
    }

    [[nodiscard]] feature_t union_extents(const feature_t& lhs,
                                          const feature_t& rhs)
    {
      feature_t result(lhs.size(), false);
      for (auto index { zero }; index < lhs.size(); ++index)
      {
        result.at(index) = lhs.at(index) || rhs.at(index);
      }
      return result;
    }

    [[nodiscard]] bool is_all_false_extent(const feature_t& extent)
    {
      return std::ranges::none_of(extent, [](bool value) { return value; });
    }

    [[nodiscard]] bool is_all_true_extent(const feature_t& extent)
    {
      return !extent.empty()
             && std::ranges::all_of(extent, [](bool value) { return value; });
    }

    [[nodiscard]] std::vector<std::pair<std::string, bool>>
        literal_pairs_from_states(const std::vector<std::int8_t>& states,
                                  const std::vector<std::string>& feature_names)
    {
      std::vector<std::pair<std::string, bool>> literals {};
      for (auto feature_index { zero }; feature_index < states.size(); ++feature_index)
      {
        if (states.at(feature_index) == 0)
        {
          continue;
        }
        literals.emplace_back(feature_names.at(feature_index), states.at(feature_index) < 0);
      }
      return literals;
    }

    void enumerate_simple_closure(simple_term_index_t& result,
                                  unsigned_integer base_feature_count,
                                  const std::vector<unsigned_integer>& ordered_valid_system_ids,
                                  const std::vector<std::string>& feature_names,
                                  bool conjunction_mode)
    {
      std::map<feature_t, simple_term_state_t> best_states {};
      std::deque<simple_term_state_t> worklist {};
      std::vector<feature_t> positive_literal_extents {};
      std::vector<feature_t> negative_literal_extents {};
      positive_literal_extents.reserve(base_feature_count);
      negative_literal_extents.reserve(base_feature_count);
      for (auto feature_index { zero }; feature_index < base_feature_count; ++feature_index)
      {
        positive_literal_extents.push_back(literal_extent(feature_index,
                                                          false,
                                                          ordered_valid_system_ids));
        negative_literal_extents.push_back(literal_extent(feature_index,
                                                          true,
                                                          ordered_valid_system_ids));
      }

      auto try_record_state = [&](simple_term_state_t state) {
        const auto best_it = best_states.find(state.extent);
        if (best_it != best_states.end()
            && !is_better_simple_descriptor(state.descriptor, best_it->second.descriptor))
        {
          return;
        }
        best_states[state.extent] = state;
        record_simple_descriptor(result, state.extent, state.descriptor);
        worklist.push_back(std::move(state));
      };

      for (auto feature_index { zero }; feature_index < base_feature_count; ++feature_index)
      {
        for (const auto negated : { false, true })
        {
          std::vector<std::int8_t> states(base_feature_count, 0);
          states.at(feature_index) = negated ? static_cast<std::int8_t>(-1)
                                             : static_cast<std::int8_t>(1);
          const auto extent = negated ? negative_literal_extents.at(feature_index)
                                      : positive_literal_extents.at(feature_index);
          simple_term_state_t state {
            extent,
            states,
            { negated ? theory_feature_category_t::not_feature
                      : theory_feature_category_t::fundamental,
              render_simple_literals(literal_pairs_from_states(states, feature_names),
                                     conjunction_mode ? logical_and_symbol
                                                      : logical_or_symbol),
              1 } };
          try_record_state(std::move(state));
        }
      }

      while (!worklist.empty())
      {
        const auto current = worklist.front();
        worklist.pop_front();
        for (auto feature_index { zero }; feature_index < base_feature_count; ++feature_index)
        {
          for (const auto negated : { false, true })
          {
            const auto desired_state = negated ? static_cast<std::int8_t>(-1)
                                               : static_cast<std::int8_t>(1);
            const auto current_state = current.literal_states.at(feature_index);
            if (current_state == desired_state)
            {
              continue;
            }
            if (current_state != 0)
            {
              if (conjunction_mode)
              {
                continue;
              }
              if (current_state == -desired_state)
              {
                continue;
              }
            }

            auto next_states = current.literal_states;
            next_states.at(feature_index) = desired_state;
            const auto& literal = negated ? negative_literal_extents.at(feature_index)
                                          : positive_literal_extents.at(feature_index);
            auto next_extent = conjunction_mode
                                   ? intersect_extents(current.extent, literal)
                                   : union_extents(current.extent, literal);
            if (is_all_false_extent(next_extent) || is_all_true_extent(next_extent))
            {
              continue;
            }
            const auto next_literals = literal_pairs_from_states(next_states, feature_names);
            const auto literal_count = next_literals.size();
            if (!conjunction_mode && literal_count < 2)
            {
              continue;
            }
            simple_term_state_t next_state {
              std::move(next_extent),
              std::move(next_states),
              { conjunction_mode ? theory_feature_category_t::and_feature
                                 : theory_feature_category_t::or_feature,
                render_simple_literals(next_literals,
                                       conjunction_mode ? logical_and_symbol
                                                        : logical_or_symbol),
                literal_count } };
            try_record_state(std::move(next_state));
          }
        }
      }
    }

    /** @brief Enumerates simple conjunction/disjunction extents lazily over valid-system memberships. */
    [[nodiscard]] simple_term_index_t build_simple_term_index(
        unsigned_integer               base_feature_count,
        const std::vector<unsigned_integer>& ordered_valid_system_ids,
        const std::vector<std::string>&      feature_names)
    {
      simple_term_index_t result {};
      enumerate_simple_closure(result,
                               base_feature_count,
                               ordered_valid_system_ids,
                               feature_names,
                               true);
      enumerate_simple_closure(result,
                               base_feature_count,
                               ordered_valid_system_ids,
                               feature_names,
                               false);
      return result;
    }

    /** @brief Formats the valid-system subset represented by a feature extent. */
    [[nodiscard]] std::string system_membership_set(
        const feature_t& systems_present,
        const system_space_t& system_space)
    {
      std::vector<unsigned_integer> ordered_valid_system_ids(
          system_space.valid.begin(), system_space.valid.end());
      std::ostringstream os;
      bool first = true;
      os << "{";
      for (auto system_index { zero }; system_index < ordered_valid_system_ids.size();
           ++system_index)
      {
        if (!systems_present.at(system_index))
        {
          continue;
        }
        if (!first)
        {
          os << ", ";
        }
        os << paper_system_label(ordered_valid_system_ids.at(system_index));
        first = false;
      }
      os << "}";
      return first ? std::string{"∅"} : os.str();
    }

    /** @brief Labels one system as available, missing, valid-only, or outside the constrained space. */
    [[nodiscard]] std::string system_status(
        unsigned_integer system_id,
        const system_space_t& system_space)
    {
      if (system_space.available.contains(system_id))
      {
        return "available";
      }
      if (system_space.missing.contains(system_id))
      {
        return "missing";
      }
      if (system_space.valid.contains(system_id))
      {
        return "valid";
      }
      return "invalid";
    }

    /** @brief Formats a set of feature identifiers using the paper-style t_i labels. */
    [[nodiscard]] std::string feature_id_set_string(const feature_ids_t& ids)
    {
      std::ostringstream os;
      bool first = true;
      os << "{";
      for (const auto feature_id : ids)
      {
        if (!first)
        {
          os << ", ";
        }
        os << paper_feature_label(feature_id);
        first = false;
      }
      os << "}";
      return first ? std::string{"∅"} : os.str();
    }

    /** @brief Normalizes the input system partition so valid, available, and missing sets are internally consistent. */
    [[nodiscard]] system_space_t normalize_theory_system_space(
        unsigned_integer   max_system_id,
        const system_space_t& input)
    {
      for (const auto system_id : input.valid)
      {
        if (system_id == zero || system_id > max_system_id)
        {
          throw std::invalid_argument(
              "Valid system id is outside the derived candidate system space.");
        }
      }
      for (const auto system_id : input.available)
      {
        if (!input.valid.contains(system_id))
        {
          throw std::invalid_argument(
              "Available systems must be a subset of the valid system space.");
        }
      }
      for (const auto system_id : input.missing)
      {
        if (!input.valid.contains(system_id))
        {
          throw std::invalid_argument(
              "Missing systems must be a subset of the valid system space.");
        }
      }
      if (std::ranges::any_of(
              input.available,
              [&](const auto system_id) { return input.missing.contains(system_id); }))
      {
        throw std::invalid_argument(
            "Available and missing systems must be disjoint subsets of the valid system space.");
      }

      auto partition = input.available;
      partition.insert(input.missing.begin(), input.missing.end());
      if (partition != input.valid)
      {
        throw std::invalid_argument(
            "Available and missing systems must form a partition of the valid system space.");
      }
      return input;
    }

    /** @brief Represents one implicant used by the minimization-based raw and simplified term derivation. */
    struct implicant_t
    {
        std::vector<int> bits {};
        feature_ids_t    covered_minterms {};
        size_t           literal_count_cache { 0 };
        size_t           specified_ones { 0 };
    };

    /** @brief Builds an implicant that matches exactly one minterm before combination. */
    [[nodiscard]] implicant_t make_implicant(unsigned_integer minterm,
                                             unsigned_integer variable_count,
                                             bool             is_care_minterm)
    {
      implicant_t implicant {};
      implicant.bits.resize(variable_count, 0);
      for (auto index { zero }; index < variable_count; ++index)
      {
        implicant.bits.at(index) = static_cast<int>((minterm >> index) & one);
      }
      implicant.literal_count_cache = variable_count;
      implicant.specified_ones = static_cast<size_t>(std::popcount(minterm));
      if (is_care_minterm)
      {
        implicant.covered_minterms.insert(minterm + one);
      }
      return implicant;
    }

    /** @brief Recomputes covered minterms and bit statistics after an implicant changes. */
    void refresh_implicant_metadata(implicant_t& implicant)
    {
      implicant.literal_count_cache = 0;
      implicant.specified_ones      = 0;
      for (const auto bit : implicant.bits)
      {
        if (bit == -1)
        {
          continue;
        }
        ++implicant.literal_count_cache;
        if (bit == 1)
        {
          ++implicant.specified_ones;
        }
      }
    }

    /** @brief Returns whether an implicant covers a concrete minterm. */
    [[nodiscard]] bool covers(const implicant_t& implicant,
                              unsigned_integer   minterm,
                              unsigned_integer   variable_count)
    {
      for (auto index { zero }; index < variable_count; ++index)
      {
        const auto bit_value = static_cast<int>((minterm >> index) & one);
        if (implicant.bits.at(index) != -1 && implicant.bits.at(index) != bit_value)
        {
          return false;
        }
      }
      return true;
    }

    /** @brief Counts the non-don't-care literals in an implicant. */
    [[nodiscard]] size_t literal_count(const implicant_t& implicant)
    {
      return implicant.literal_count_cache;
    }

    /** @brief Returns whether two implicants differ in exactly one literal and can be merged. */
    [[nodiscard]] bool can_combine(const implicant_t& lhs,
                                   const implicant_t& rhs)
    {
      auto differing_positions { zero };
      for (auto index { zero }; index < lhs.bits.size(); ++index)
      {
        const auto left  = lhs.bits.at(index);
        const auto right = rhs.bits.at(index);
        if (left == right)
        {
          continue;
        }
        if (left == -1 || right == -1)
        {
          return false;
        }
        ++differing_positions;
        if (differing_positions > one)
        {
          return false;
        }
      }
      return differing_positions == one;
    }

    /** @brief Merges two compatible implicants into the next more general implicant. */
    [[nodiscard]] implicant_t combine_implicants(const implicant_t& lhs,
                                                 const implicant_t& rhs)
    {
      implicant_t result {};
      result.bits = lhs.bits;
      for (auto index { zero }; index < lhs.bits.size(); ++index)
      {
        if (lhs.bits.at(index) != rhs.bits.at(index))
        {
          result.bits.at(index) = -1;
        }
      }
      result.covered_minterms = lhs.covered_minterms;
      result.covered_minterms.insert(rhs.covered_minterms.begin(),
                                     rhs.covered_minterms.end());
      refresh_implicant_metadata(result);
      return result;
    }

    /** @brief Returns whether two implicants encode the same literal pattern. */
    [[nodiscard]] bool same_bits(const implicant_t& lhs, const implicant_t& rhs)
    {
      return lhs.bits == rhs.bits;
    }

    /** @brief Removes duplicate implicants after a combination round. */
    [[nodiscard]] std::vector<implicant_t>
        deduplicate_implicants(const std::vector<implicant_t>& implicants)
    {
      std::vector<implicant_t> result {};
      for (const auto& implicant : implicants)
      {
        const auto duplicate = std::ranges::find_if(
            result,
            [&](const auto& existing) { return same_bits(existing, implicant); });
        if (duplicate == result.end())
        {
          result.push_back(implicant);
        }
        else
        {
          duplicate->covered_minterms.insert(implicant.covered_minterms.begin(),
                                             implicant.covered_minterms.end());
        }
      }
      return result;
    }

    /** @brief Computes the prime implicants for the given minterm set. */
    [[nodiscard]] std::vector<implicant_t>
        prime_implicants(unsigned_integer                    variable_count,
                         const std::vector<unsigned_integer>& minterms,
                         const std::vector<unsigned_integer>& dont_cares)
    {
      std::vector<implicant_t> current {};
      current.reserve(minterms.size() + dont_cares.size());
      for (const auto minterm : minterms)
      {
        current.push_back(make_implicant(minterm, variable_count, true));
      }
      for (const auto minterm : dont_cares)
      {
        current.push_back(make_implicant(minterm, variable_count, false));
      }

      current = deduplicate_implicants(current);
      std::vector<implicant_t> primes {};

      while (!current.empty())
      {
        std::vector<std::vector<size_t>> groups(variable_count + one);
        for (auto index { zero }; index < current.size(); ++index)
        {
          groups.at(current.at(index).specified_ones).push_back(index);
        }

        std::vector<implicant_t> next {};
        std::vector<bool> combined(current.size(), false);
        for (auto group_index { zero }; group_index < variable_count; ++group_index)
        {
          const auto& left_group  = groups.at(group_index);
          const auto& right_group = groups.at(group_index + one);
          for (const auto left_index : left_group)
          {
            for (const auto right_index : right_group)
            {
              if (!can_combine(current.at(left_index), current.at(right_index)))
              {
                continue;
              }
              combined.at(left_index)  = true;
              combined.at(right_index) = true;
              next.push_back(
                  combine_implicants(current.at(left_index), current.at(right_index)));
            }
          }
        }

        for (auto index { zero }; index < current.size(); ++index)
        {
          if (!combined.at(index))
          {
            primes.push_back(current.at(index));
          }
        }

        next = deduplicate_implicants(next);
        if (next.empty())
        {
          break;
        }
        current = std::move(next);
      }

      return deduplicate_implicants(primes);
    }

    /** @brief Builds the minterm cover set of each prime implicant. */
    [[nodiscard]] std::vector<feature_ids_t>
        implicant_cover_sets(unsigned_integer                    variable_count,
                             const std::vector<unsigned_integer>& minterms,
                             const std::vector<implicant_t>&      prime_implicants)
    {
      std::vector<feature_ids_t> cover_sets(prime_implicants.size());
      for (auto prime_index { zero }; prime_index < prime_implicants.size(); ++prime_index)
      {
        for (auto minterm_index { zero }; minterm_index < minterms.size(); ++minterm_index)
        {
          if (covers(prime_implicants.at(prime_index),
                     minterms.at(minterm_index),
                     variable_count))
          {
            cover_sets.at(prime_index).insert(minterm_index + one);
          }
        }
      }
      return cover_sets;
    }

    /** @brief Computes a fast initial cover used to seed the exact cover search. */
    [[nodiscard]] std::vector<size_t>
        greedy_cover_subset(const std::vector<size_t>&      candidate_indices,
                            const std::vector<implicant_t>& prime_implicants,
                            const std::vector<feature_ids_t>& cover_sets,
                            feature_ids_t                   uncovered)
    {
      std::vector<size_t> subset {};
      std::vector<size_t> remaining_indices { candidate_indices.begin(),
                                              candidate_indices.end() };
      while (!uncovered.empty() && !remaining_indices.empty())
      {
        auto   best_index = remaining_indices.front();
        size_t best_gain {};
        size_t best_cost = std::numeric_limits<size_t>::max();
        for (const auto index : remaining_indices)
        {
          const auto overlap
              = feature_ids_t::intersection_of(cover_sets.at(index), uncovered);
          const auto gain = overlap.size();
          const auto cost = literal_count(prime_implicants.at(index));
          if (gain > best_gain || (gain == best_gain && cost < best_cost))
          {
            best_index = index;
            best_gain  = gain;
            best_cost  = cost;
          }
        }
        if (best_gain == zero)
        {
          break;
        }
        subset.push_back(best_index);
        uncovered = feature_ids_t::difference_of(uncovered, cover_sets.at(best_index));
        remaining_indices.erase(std::remove(remaining_indices.begin(),
                                            remaining_indices.end(),
                                            best_index),
                                remaining_indices.end());
      }
      return subset;
    }

    /** @brief Performs branch-and-bound search for a small implicant cover after essentials are fixed. */
    struct exact_cover_search_t
    {
        const std::vector<implicant_t>&  prime_implicants;
        const std::vector<feature_ids_t>& cover_sets;
        std::vector<size_t>              best_subset {};
        size_t                           best_literal_count {
          std::numeric_limits<size_t>::max()
        };
        size_t best_implicant_count { std::numeric_limits<size_t>::max() };

        /** @brief Records a new best subset when it improves the current incumbent. */
        void consider_current(const std::vector<size_t>& subset, size_t literal_total)
        {
          if (literal_total < best_literal_count
              || (literal_total == best_literal_count
                  && subset.size() < best_implicant_count))
          {
            best_subset = subset;
            best_literal_count = literal_total;
            best_implicant_count = subset.size();
          }
        }

        /** @brief Returns a lower bound on the literal cost of the remaining candidates. */
        [[nodiscard]] size_t min_literal_cost(const std::vector<size_t>& candidates) const
        {
          size_t result { std::numeric_limits<size_t>::max() };
          for (const auto index : candidates)
          {
            result = std::min(result, literal_count(prime_implicants.at(index)));
          }
          return result == std::numeric_limits<size_t>::max() ? zero : result;
        }

        /** @brief Orders candidate implicants by how much uncovered mass they remove next. */
        [[nodiscard]] std::vector<std::pair<size_t, size_t>> pivot_coverers(
            const std::vector<size_t>& candidates,
            const feature_ids_t&       uncovered) const
        {
          std::vector<std::pair<size_t, size_t>> best_coverers {};
          size_t best_count = std::numeric_limits<size_t>::max();

          for (const auto uncovered_id : uncovered)
          {
            std::vector<std::pair<size_t, size_t>> coverers {};
            for (auto position { zero }; position < candidates.size(); ++position)
            {
              const auto index = candidates.at(position);
              if (cover_sets.at(index).contains(uncovered_id))
              {
                coverers.emplace_back(position, index);
              }
            }
            if (coverers.empty())
            {
              return {};
            }
            if (coverers.size() < best_count)
            {
              best_count = coverers.size();
              best_coverers = std::move(coverers);
              if (best_count == one)
              {
                break;
              }
            }
          }
          return best_coverers;
        }

        /** @brief Explores candidate subsets until the best exact or near-exact cover is found. */
        void search(const std::vector<size_t>& candidates,
                    const feature_ids_t&       uncovered,
                    std::vector<size_t>&       current_subset,
                    size_t                     current_literal_total)
        {
          if (uncovered.empty())
          {
            consider_current(current_subset, current_literal_total);
            return;
          }
          if (candidates.empty())
          {
            return;
          }
          if (current_literal_total > best_literal_count
              || (current_literal_total == best_literal_count
                  && current_subset.size() >= best_implicant_count))
          {
            return;
          }

          size_t max_gain {};
          for (const auto index : candidates)
          {
            const auto overlap
                = feature_ids_t::intersection_of(cover_sets.at(index), uncovered);
            max_gain = std::max(max_gain, overlap.size());
          }
          if (max_gain == zero)
          {
            return;
          }

          const auto lower_bound_count
              = static_cast<size_t>((uncovered.size() + max_gain - one) / max_gain);
          if (current_subset.size() + lower_bound_count > best_implicant_count)
          {
            return;
          }
          const auto min_literal = min_literal_cost(candidates);
          if (current_literal_total + lower_bound_count * min_literal > best_literal_count)
          {
            return;
          }

          const auto coverers = pivot_coverers(candidates, uncovered);
          if (coverers.empty())
          {
            return;
          }

          for (const auto& [position, index] : coverers)
          {
            current_subset.push_back(index);
            const auto next_uncovered
                = feature_ids_t::difference_of(uncovered, cover_sets.at(index));

            std::vector<size_t> next_candidates {};
            next_candidates.reserve(candidates.size() - position - one);
            for (auto next_position { position + one }; next_position < candidates.size();
                 ++next_position)
            {
              const auto next_index = candidates.at(next_position);
              if (feature_ids_t::overlaps(cover_sets.at(next_index), next_uncovered))
              {
                next_candidates.push_back(next_index);
              }
            }

            search(next_candidates,
                   next_uncovered,
                   current_subset,
                   current_literal_total + literal_count(prime_implicants.at(index)));
            current_subset.pop_back();
          }
        }
    };

    /** @brief Selects a compact implicant cover from the prime implicants of one feature extent. */
    [[nodiscard]] std::vector<implicant_t>
        select_cover(unsigned_integer                    variable_count,
                     const std::vector<unsigned_integer>& minterms,
                     const std::vector<implicant_t>&      prime_implicants)
    {
      if (minterms.empty())
      {
        return {};
      }

      const auto cover_sets = implicant_cover_sets(variable_count,
                                                   minterms,
                                                   prime_implicants);

      std::vector<implicant_t> selected {};
      feature_ids_t uncovered {};
      std::vector<bool> chosen(prime_implicants.size(), false);
      for (auto minterm_index { zero }; minterm_index < minterms.size(); ++minterm_index)
      {
        uncovered.insert(minterm_index + one);
      }

      for (auto minterm_index { zero }; minterm_index < minterms.size(); ++minterm_index)
      {
        std::vector<size_t> covering_indices {};
        const auto minterm_id = minterm_index + one;
        for (auto index { zero }; index < prime_implicants.size(); ++index)
        {
          if (cover_sets.at(index).contains(minterm_id))
          {
            covering_indices.push_back(index);
          }
        }
        if (covering_indices.size() == one)
        {
          const auto essential = covering_indices.front();
          if (!chosen.at(essential))
          {
            chosen.at(essential) = true;
            selected.push_back(prime_implicants.at(essential));
            uncovered = feature_ids_t::difference_of(uncovered, cover_sets.at(essential));
          }
        }
      }

      std::vector<size_t> remaining_indices {};
      for (auto index { zero }; index < prime_implicants.size(); ++index)
      {
        if (!chosen.at(index) && feature_ids_t::overlaps(cover_sets.at(index), uncovered))
        {
          remaining_indices.push_back(index);
        }
      }

      if (!uncovered.empty())
      {
        exact_cover_search_t search_state { prime_implicants, cover_sets };
        search_state.best_subset
            = greedy_cover_subset(remaining_indices,
                                  prime_implicants,
                                  cover_sets,
                                  uncovered);
        search_state.best_literal_count = 0;
        for (const auto index : search_state.best_subset)
        {
          search_state.best_literal_count += literal_count(prime_implicants.at(index));
        }
        search_state.best_implicant_count = search_state.best_subset.size();

        std::vector<size_t> current_subset {};
        current_subset.reserve(remaining_indices.size());
        search_state.search(remaining_indices, uncovered, current_subset, 0);

        for (const auto index : search_state.best_subset)
        {
          selected.push_back(prime_implicants.at(index));
        }
      }

      std::ranges::sort(selected,
                        [](const auto& lhs, const auto& rhs)
                        {
                          if (literal_count(lhs) != literal_count(rhs))
                          {
                            return literal_count(lhs) < literal_count(rhs);
                          }
                          return lhs.bits < rhs.bits;
                        });
      return selected;
    }

    /** @brief Selects whether minimized implicants are rendered as DNF or CNF. */
    enum class normal_form_t
    {
      dnf,
      cnf
    };

    /** @brief Renders one implicant as a single clause in the requested normal form. */
    [[nodiscard]] std::string clause_to_string(const implicant_t&             implicant,
                                               const std::vector<std::string>& feature_names,
                                               normal_form_t                  normal_form)
    {
      std::vector<std::string> positive_literals {};
      std::vector<std::string> negative_literals {};
      for (auto index { zero }; index < implicant.bits.size(); ++index)
      {
        const auto bit = implicant.bits.at(index);
        if (bit == -1)
        {
          continue;
        }

        const auto negated = normal_form == normal_form_t::dnf ? bit == 0 : bit == 1;
        if (negated)
        {
          negative_literals.push_back(std::string(logical_not_symbol)
                                      + feature_names.at(index));
        }
        else
        {
          positive_literals.push_back(feature_names.at(index));
        }
      }

      std::ranges::sort(positive_literals);
      std::ranges::sort(negative_literals);

      std::vector<std::string> literals {};
      literals.reserve(positive_literals.size() + negative_literals.size());
      literals.insert(literals.end(), positive_literals.begin(), positive_literals.end());
      literals.insert(literals.end(), negative_literals.begin(), negative_literals.end());

      if (literals.empty())
      {
        return normal_form == normal_form_t::dnf ? logical_true_symbol
                                                 : logical_false_symbol;
      }

      std::ostringstream oss {};
      for (auto index { zero }; index < literals.size(); ++index)
      {
        if (index > zero)
        {
          oss << (normal_form == normal_form_t::dnf ? logical_and_symbol
                                                    : logical_or_symbol);
        }
        oss << literals.at(index);
      }
      return oss.str();
    }

    /** @brief Renders a minimized implicant cover as a whole DNF or CNF expression. */
    [[nodiscard]] std::string
        normal_form_to_string(const std::vector<implicant_t>& implicants,
                              const std::vector<std::string>& feature_names,
                              normal_form_t                  normal_form)
    {
      if (implicants.empty())
      {
        return normal_form == normal_form_t::dnf ? logical_false_symbol
                                                 : logical_true_symbol;
      }

      if (implicants.size() == one && literal_count(implicants.front()) == zero)
      {
        return normal_form == normal_form_t::dnf ? logical_true_symbol
                                                 : logical_false_symbol;
      }

      /** @brief Stores one rendered clause together with sort keys used for stable output ordering. */
      struct rendered_clause_t
      {
          std::string text {};
          size_t      negated_literals { 0 };
          size_t      literal_total { 0 };
      };

      std::vector<rendered_clause_t> rendered_clauses {};
      rendered_clauses.reserve(implicants.size());
      for (const auto& implicant : implicants)
      {
        const auto clause = clause_to_string(implicant, feature_names, normal_form);
        size_t negated_literals {};
        for (const auto bit : implicant.bits)
        {
          if (bit == -1)
          {
            continue;
          }
          const auto negated = normal_form == normal_form_t::dnf ? bit == 0 : bit == 1;
          if (negated)
          {
            ++negated_literals;
          }
        }
        rendered_clauses.push_back(
            { clause, negated_literals, literal_count(implicant) });
      }

      if (normal_form == normal_form_t::cnf)
      {
        std::ranges::sort(rendered_clauses,
                          [](const auto& lhs, const auto& rhs)
                          {
                            if (lhs.negated_literals != rhs.negated_literals)
                            {
                              return lhs.negated_literals < rhs.negated_literals;
                            }
                            if (lhs.literal_total != rhs.literal_total)
                            {
                              return lhs.literal_total < rhs.literal_total;
                            }
                            return lhs.text > rhs.text;
                          });
      }

      std::ostringstream oss {};
      for (auto index { zero }; index < rendered_clauses.size(); ++index)
      {
        if (index > zero)
        {
          oss << (normal_form == normal_form_t::dnf ? logical_or_symbol
                                                    : logical_and_symbol);
        }

        const auto& clause = rendered_clauses.at(index);
        const auto needs_parentheses
            = rendered_clauses.size() > one && clause.literal_total > one;
        if (needs_parentheses)
        {
          oss << '(' << clause.text << ')';
        }
        else
        {
          oss << clause.text;
        }
      }
      return oss.str();
    }
  } // namespace

  [[nodiscard]] unsigned_integer system_t::T() const
  {
    return feature_ids.size();
  }

  const feature_ids_t& system_t::all_features() const
  {
    return feature_ids;
  }

  std::string to_string(theory_feature_category_t category)
  {
    switch (category)
    {
      case theory_feature_category_t::impossible :
        return "Impossible";
      case theory_feature_category_t::core :
        return "Core";
      case theory_feature_category_t::fundamental :
        return "Fundamental";
      case theory_feature_category_t::not_feature :
        return "Not";
      case theory_feature_category_t::and_feature :
        return "And";
      case theory_feature_category_t::or_feature :
        return "Or";
      case theory_feature_category_t::complex :
        return "Complex";
      case theory_feature_category_t::unclassified :
      default :
        return "Unclassified";
    }
  }

  feature_analysis_t::feature_analysis_t(
      unsigned_integer                                  f,
      const system_space_t&                             system_space,
      std::vector<std::string>                          feature_names,
      report_request_t                                  report_request)
      : m_base_feature_count { f }
      , m_valid_system_count {
          static_cast<unsigned_integer>(system_space.valid.size())
        }
      , m_max_system_id { power2(f) }
      , m_system_space {
          normalize_theory_system_space(power2(f), system_space)
        }
      , m_feature_names { feature_names.empty() ? std::vector<std::string> {}
                                                : std::move(feature_names) }
      , m_report_request { std::move(report_request) }
      , m_resolved_feature_names {
          [this]()
          {
            std::vector<std::string> names {};
            names.reserve(m_base_feature_count);
            for (auto index { zero }; index < m_base_feature_count; ++index)
            {
              if (index < m_feature_names.size())
              {
                names.push_back(m_feature_names.at(index));
              }
              else
              {
                names.push_back("f" + std::to_string(index + one));
              }
            }
            return names;
          }()
        }
      , m_ordered_valid_system_ids {
          [this]()
          {
            return std::vector<unsigned_integer>(m_system_space.valid.begin(),
                                                m_system_space.valid.end());
          }()
        }
      , m_valid_system_positions {
          [this]()
          {
            std::unordered_map<unsigned_integer, std::size_t> positions {};
            positions.reserve(m_ordered_valid_system_ids.size());
            for (std::size_t index { 0 }; index < m_ordered_valid_system_ids.size(); ++index)
            {
              positions.emplace(m_ordered_valid_system_ids.at(index), index);
            }
            return positions;
          }()
        }
      , m_feature_registry { initialize_feature_registry() }
      , m_systems { m_report_request.mode == report_mode_t::summary
                        ? systems_t {}
                        : initialize_registry_backed_systems() }
  { }

  feature_analysis_t::feature_analysis_t(
      unsigned_integer                                  f,
      const system_space_t&                             system_space,
      std::vector<std::string>                          feature_names,
      std::optional<std::vector<theory_feature_category_t>> category_filter)
      : feature_analysis_t(f,
                           system_space,
                           std::move(feature_names),
                           report_request_t::from_category_hint(
                               std::move(category_filter)))
  { }

  [[nodiscard]] feature_t feature_analysis_t::at(unsigned_integer feature_id) const
  {
    if (feature_id == zero || feature_id > m_feature_registry.size())
    {
      throw std::out_of_range("Illegal derived feature id.");
    }
    return m_feature_registry.at(feature_id - one).systems_present;
  }

  unsigned_integer feature_analysis_t::category(unsigned_integer feature_id) const
  {
    if (feature_id == zero || feature_id > m_feature_registry.size())
    {
      throw std::out_of_range("Illegal derived feature id.");
    }
    return static_cast<unsigned_integer>(m_feature_registry.at(feature_id - one).category);
  }

  std::string feature_analysis_t::category_name(unsigned_integer feature_id) const
  {
    return to_string(static_cast<theory_feature_category_t>(category(feature_id)));
  }

  bool feature_analysis_t::uses_theory_registry() const
  {
    return true;
  }

  bool feature_analysis_t::includes_category(
      theory_feature_category_t category) const
  {
    return m_report_request.includes_category(category);
  }

  bool feature_analysis_t::includes_feature(unsigned_integer feature_id) const
  {
    if (feature_id == zero || feature_id > m_feature_registry.size())
    {
      throw std::out_of_range("Illegal derived feature id.");
    }
    return includes_category(m_feature_registry.at(feature_id - one).category);
  }

  const system_space_t& feature_analysis_t::system_space() const
  {
    return m_system_space;
  }

  const report_request_t& feature_analysis_t::report_request() const
  {
    return m_report_request;
  }

  const std::vector<feature_record_t>& feature_analysis_t::feature_registry() const
  {
    return m_feature_registry;
  }

  unsigned_integer feature_analysis_t::feature_count() const
  {
    return power2(m_valid_system_count);
  }

  unsigned_integer feature_analysis_t::system_count() const
  {
    return m_max_system_id;
  }

  unsigned_integer feature_analysis_t::base_feature_count() const
  {
    return m_base_feature_count;
  }

  unsigned_integer feature_analysis_t::valid_system_count() const
  {
    return m_valid_system_count;
  }

  const system_t& feature_analysis_t::system_by_id(unsigned_integer system_id) const
  {
    const auto position = m_valid_system_positions.find(system_id);
    if (position == m_valid_system_positions.end())
    {
      throw std::out_of_range("System id is not part of the valid system registry.");
    }
    return m_systems.at(position->second);
  }

  void feature_analysis_t::print_feature(std::ostream&    os,
                                         const feature_t& feature) const
  {
    os << to_string(feature);
  }

  void feature_analysis_t::print_features(std::ostream& os) const
  {
    os << "CATEGORY" << separator() << "i" << separator() << "FEATURE"
       << separator() << "t_i" << separator() << "β(t_i)" << separator()
       << "ℓ_T(t_i)(simplified)" << separator() << "ℓ_T(t_i)(raw term)"
       << std::endl;
    for (const auto category : ordered_theory_categories())
    {
      if (!includes_category(category))
      {
        continue;
      }
      for (const auto& record : m_feature_registry)
      {
        if (record.category != category)
        {
          continue;
        }
        os << to_string(record.category) << separator()
           << paper_feature_index(record.id) << separator()
           << paper_feature_label(record.id) << separator()
           << system_membership_set(record.systems_present, m_system_space)
           << separator() << to_string(record.systems_present) << separator()
           << record.simplified_term << separator() << record.raw_term << std::endl;
      }
    }
  }

  void feature_analysis_t::print_categories(std::ostream& os) const
  {
    os << "CATEGORY" << separator() << "COUNT" << std::endl;
    for (const auto category : ordered_theory_categories())
    {
      if (!includes_category(category))
      {
        continue;
      }
      const auto count = std::ranges::count_if(
          m_feature_registry,
          [&](const feature_record_t& record) { return record.category == category; });
      os << to_string(category) << separator() << count << std::endl;
    }
  }

  void feature_analysis_t::print_system_statistics(std::ostream&    os,
                                                   const system_t&  system,
                                                   unsigned_integer id) const
  {
    std::array<unsigned_integer, 7> counts { zero, zero, zero, zero, zero, zero, zero };
    const auto& system_features = system.all_features();
    for (const auto feature_id : system_features)
    {
      const auto category = m_feature_registry.at(feature_id - one).category;
      switch (category)
      {
        case theory_feature_category_t::impossible :
          ++counts.at(0);
          break;
        case theory_feature_category_t::core :
          ++counts.at(1);
          break;
        case theory_feature_category_t::fundamental :
          ++counts.at(2);
          break;
        case theory_feature_category_t::not_feature :
          ++counts.at(3);
          break;
        case theory_feature_category_t::and_feature :
          ++counts.at(4);
          break;
        case theory_feature_category_t::or_feature :
          ++counts.at(5);
          break;
        case theory_feature_category_t::complex :
          ++counts.at(6);
          break;
        case theory_feature_category_t::unclassified :
        default :
          break;
      }
    }
    os << id << separator() << paper_system_label(id) << separator()
       << system_status(id, m_system_space);
    for (const auto count : counts)
    {
      os << separator() << count;
    }
    os << separator() << system.T() << std::endl;
  }

  void feature_analysis_t::print_systems_statistics(std::ostream& os) const
  {
    os << "SYSTEM ID" << separator() << "SYSTEM" << separator() << "STATUS"
       << separator() << "Impossible" << separator() << "Core"
       << separator() << "Fundamental" << separator() << "Not" << separator()
       << "And" << separator() << "Or" << separator() << "Complex"
       << separator() << "T" << std::endl;
    for (auto system_index { zero }; system_index < m_ordered_valid_system_ids.size(); ++system_index)
    {
      print_system_statistics(os,
                              m_systems.at(system_index),
                              m_ordered_valid_system_ids.at(system_index));
    }
  }

  void feature_analysis_t::print_system_features(std::ostream&    os,
                                                 const system_t&  system,
                                                 unsigned_integer system_id) const
  {
    os << system_id << separator() << paper_system_label(system_id) << separator()
       << system_status(system_id, m_system_space) << separator();

    std::array<feature_ids_t, 7> grouped {};
    const auto& system_features = system.all_features();
    for (const auto feature_id : system_features)
    {
      const auto category = m_feature_registry.at(feature_id - one).category;
      switch (category)
      {
        case theory_feature_category_t::impossible :
          grouped.at(0).insert(feature_id);
          break;
        case theory_feature_category_t::core :
          grouped.at(1).insert(feature_id);
          break;
        case theory_feature_category_t::fundamental :
          grouped.at(2).insert(feature_id);
          break;
        case theory_feature_category_t::not_feature :
          grouped.at(3).insert(feature_id);
          break;
        case theory_feature_category_t::and_feature :
          grouped.at(4).insert(feature_id);
          break;
        case theory_feature_category_t::or_feature :
          grouped.at(5).insert(feature_id);
          break;
        case theory_feature_category_t::complex :
          grouped.at(6).insert(feature_id);
          break;
        case theory_feature_category_t::unclassified :
        default :
          break;
      }
    }
    for (const auto& ids : grouped)
    {
      os << feature_id_set_string(ids) << separator();
    }
    os << std::endl;
  }

  void feature_analysis_t::print_systems_features(std::ostream& os) const
  {
    os << "\nSYSTEM ID" << separator() << "SYSTEM" << separator() << "STATUS"
       << separator() << "Impossible" << separator() << "Core"
       << separator() << "Fundamental" << separator() << "Not" << separator()
       << "And" << separator() << "Or" << separator() << "Complex"
       << std::endl;
    for (auto system_index { zero }; system_index < m_ordered_valid_system_ids.size(); ++system_index)
    {
      print_system_features(os,
                            m_systems.at(system_index),
                            m_ordered_valid_system_ids.at(system_index));
    }
  }

  void feature_analysis_t::print(std::ostream& os) const
  {
    os << "MODEL" << separator() << "|F|" << separator() << "|S_valid|"
       << separator() << "|S_available|" << separator() << "|S_missing|"
       << separator() << "|T|" << std::endl;
    os << "THEORY" << separator() << m_base_feature_count << separator()
       << m_system_space.valid.size() << separator() << m_system_space.available.size()
       << separator() << m_system_space.missing.size() << separator() << feature_count()
       << std::endl << std::endl;
    print_categories(os);
    os << std::endl;
    print_systems_statistics(os);
    print_systems_features(os);
  }

  std::string feature_analysis_t::feature_name(unsigned_integer feature_index) const
  {
    if (feature_index < m_resolved_feature_names.size())
    {
      return m_resolved_feature_names.at(feature_index);
    }
    return "f" + std::to_string(feature_index + one);
  }

  std::string feature_analysis_t::raw_term_for(const feature_t& systems_present) const
  {
    if (systems_present.empty()
        || std::ranges::none_of(systems_present, [](bool value) { return value; }))
    {
      return logical_false_symbol;
    }

    std::vector<implicant_t> implicants {};
    implicants.reserve(systems_present.size());
    for (auto system_index { zero }; system_index < systems_present.size(); ++system_index)
    {
      if (!systems_present.at(system_index))
      {
        continue;
      }
      implicants.push_back(
          make_implicant(m_ordered_valid_system_ids.at(system_index) - one,
                         m_base_feature_count,
                         false));
    }

    return normal_form_to_string(implicants,
                                 m_resolved_feature_names,
                                 normal_form_t::dnf);
  }

  std::string feature_analysis_t::simplified_term_for(
      const feature_t& systems_present,
      const std::vector<unsigned_integer>& dont_cares) const
  {
    if (systems_present.empty()
        || std::ranges::none_of(systems_present, [](bool value) { return value; }))
    {
      return logical_false_symbol;
    }

    std::vector<unsigned_integer> minterms {};
    std::vector<unsigned_integer> complement_minterms {};
    minterms.reserve(m_ordered_valid_system_ids.size());
    complement_minterms.reserve(m_ordered_valid_system_ids.size());

    for (auto system_index { zero }; system_index < m_ordered_valid_system_ids.size();
         ++system_index)
    {
      const auto encoded_system = m_ordered_valid_system_ids.at(system_index) - one;
      if (systems_present.at(system_index))
      {
        minterms.push_back(encoded_system);
      }
      else
      {
        complement_minterms.push_back(encoded_system);
      }
    }

    const auto dnf_primes
        = prime_implicants(m_base_feature_count, minterms, dont_cares);
    const auto dnf_cover = select_cover(m_base_feature_count, minterms, dnf_primes);
    const auto dnf_expression = normal_form_to_string(dnf_cover,
                                                      m_resolved_feature_names,
                                                      normal_form_t::dnf);

    const auto cnf_primes = prime_implicants(m_base_feature_count,
                                             complement_minterms,
                                             dont_cares);
    const auto cnf_cover
        = select_cover(m_base_feature_count, complement_minterms, cnf_primes);
    const auto cnf_expression = normal_form_to_string(cnf_cover,
                                                      m_resolved_feature_names,
                                                      normal_form_t::cnf);

    if (dnf_expression.size() < cnf_expression.size())
    {
      return dnf_expression;
    }
    return cnf_expression;
  }

  theory_feature_category_t feature_analysis_t::classify_simplified_term(
      const std::string& simplified_term) const
  {
    if (simplified_term == logical_false_symbol)
    {
      return theory_feature_category_t::impossible;
    }
    if (simplified_term == logical_true_symbol)
    {
      return theory_feature_category_t::core;
    }

    const auto has_or  = simplified_term.find(logical_or_symbol) != std::string::npos;
    const auto has_and = simplified_term.find(logical_and_symbol) != std::string::npos;
    const auto has_not = simplified_term.find(logical_not_symbol) != std::string::npos;

    if (!has_or && !has_and)
    {
      return has_not ? theory_feature_category_t::not_feature
                     : theory_feature_category_t::fundamental;
    }
    if (!has_or && has_and)
    {
      return theory_feature_category_t::and_feature;
    }
    if (has_or && !has_and)
    {
      return theory_feature_category_t::or_feature;
    }
    return theory_feature_category_t::complex;
  }

  bool feature_analysis_t::should_materialize_details(
      theory_feature_category_t category) const
  {
    if (!includes_category(category))
    {
      return false;
    }
    if (category == theory_feature_category_t::complex
        && !m_report_request.include_complex_details)
    {
      return false;
    }
    return should_materialize_simplified_term(category)
           || should_materialize_raw_term(category);
  }

  bool feature_analysis_t::should_materialize_simplified_term(
      theory_feature_category_t category) const
  {
    if (!m_report_request.include_simplified_terms)
    {
      return false;
    }
    if (!includes_category(category))
    {
      return false;
    }
    return category != theory_feature_category_t::complex
           || m_report_request.include_complex_details;
  }

  bool feature_analysis_t::should_materialize_raw_term(
      theory_feature_category_t category) const
  {
    if (!m_report_request.include_raw_terms)
    {
      return false;
    }
    if (!includes_category(category))
    {
      return false;
    }
    return category != theory_feature_category_t::complex
           || m_report_request.include_complex_details;
  }

  [[nodiscard]] std::vector<feature_record_t>
      feature_analysis_t::initialize_feature_registry() const
  {
    std::vector<feature_record_t> result {};
    const auto registry_size = power2(m_valid_system_count);
    const auto simple_term_index = build_simple_term_index(m_base_feature_count,
                                                           m_ordered_valid_system_ids,
                                                           m_resolved_feature_names);
    result.reserve(static_cast<size_t>(registry_size));
    std::vector<std::size_t> pending_complex_simplifications {};

    auto build_invalid_system_minterms = [this]() {
      std::vector<unsigned_integer> invalid_minterms {};
      if (m_valid_system_count == m_max_system_id)
      {
        return invalid_minterms;
      }
      invalid_minterms.reserve(
          static_cast<size_t>(m_max_system_id - m_ordered_valid_system_ids.size()));
      for (auto system_id { one }; system_id <= m_max_system_id; ++system_id)
      {
        if (!m_system_space.valid.contains(system_id))
        {
          invalid_minterms.push_back(system_id - one);
        }
      }
      return invalid_minterms;
    };

    for (auto encoded_subset { zero }; encoded_subset < registry_size; ++encoded_subset)
    {
      feature_record_t record {};
      record.id = encoded_subset + one;
      record.systems_present = feature_t(m_valid_system_count, false);
      for (auto system_index { zero }; system_index < m_valid_system_count; ++system_index)
      {
        const auto shift
            = static_cast<unsigned_integer>(m_valid_system_count - system_index - one);
        record.systems_present.at(system_index) = ((encoded_subset >> shift) & one) == one;
      }

      const auto any_present = std::ranges::any_of(
          record.systems_present, [](bool value) { return value; });
      const auto all_present = !record.systems_present.empty()
                               && std::ranges::all_of(
                                   record.systems_present,
                                   [](bool value) { return value; });
      if (!any_present)
      {
        record.category = theory_feature_category_t::impossible;
        if (should_materialize_simplified_term(record.category))
        {
          record.simplified_term = logical_false_symbol;
          record.display_name    = record.simplified_term;
        }
        if (should_materialize_raw_term(record.category))
        {
          record.raw_term = logical_false_symbol;
          if (record.display_name.empty())
          {
            record.display_name = record.raw_term;
          }
        }
        result.push_back(record);
        continue;
      }
      if (all_present)
      {
        record.category = theory_feature_category_t::core;
        if (should_materialize_simplified_term(record.category))
        {
          record.simplified_term = logical_true_symbol;
          record.display_name    = record.simplified_term;
        }
        if (should_materialize_raw_term(record.category))
        {
          record.raw_term = raw_term_for(record.systems_present);
          if (record.display_name.empty())
          {
            record.display_name = record.raw_term;
          }
        }
        result.push_back(record);
        continue;
      }

      const auto simple_match = simple_term_index.find(record.systems_present);
      if (simple_match != simple_term_index.end())
      {
        record.category = simple_match->second.category;
        if (should_materialize_simplified_term(record.category))
        {
          record.simplified_term = simple_match->second.expression;
          record.display_name    = record.simplified_term;
        }
        if (should_materialize_raw_term(record.category))
        {
          record.raw_term = raw_term_for(record.systems_present);
          if (record.display_name.empty())
          {
            record.display_name = record.raw_term;
          }
        }
        result.push_back(record);
        continue;
      }

      record.category = theory_feature_category_t::complex;
      if (should_materialize_raw_term(record.category))
      {
        record.raw_term = raw_term_for(record.systems_present);
        if (record.display_name.empty())
        {
          record.display_name = record.raw_term;
        }
      }
      if (should_materialize_simplified_term(record.category))
      {
        pending_complex_simplifications.push_back(result.size());
      }
      result.push_back(std::move(record));
    }

    if (!pending_complex_simplifications.empty())
    {
      const auto invalid_minterms = build_invalid_system_minterms();
      for (const auto index : pending_complex_simplifications)
      {
        auto& record = result.at(index);
        record.simplified_term = simplified_term_for(record.systems_present,
                                                     invalid_minterms);
        record.display_name = record.simplified_term;
      }
    }

    return result;
  }

  systems_t feature_analysis_t::initialize_registry_backed_systems() const
  {
    systems_t result(m_ordered_valid_system_ids.size(), system_t {});
    for (const auto& record : m_feature_registry)
    {
      for (auto system_index { zero }; system_index < m_ordered_valid_system_ids.size();
           ++system_index)
      {
        if (!record.systems_present.at(system_index))
        {
          continue;
        }
        result.at(system_index).feature_ids.insert(record.id);
      }
    }
    return result;
  }

  /** @brief Returns or updates the current tabular output separator used by print helpers. */
  [[nodiscard]] char separator(const char s)
  {
    static char sep = '\t';
    if (s != 0)
    {
      sep = s;
    }
    return sep;
  }

  /** @brief Renders a bit-vector feature extent in the repository's binary order. */
  std::string to_string(const feature_t& fd)
  {
    std::string result {};
    for (const auto& b : fd)
    {
      if constexpr (MSB_LSB)
      {
        result = std::to_string(b) + result;
      }
      else
      {
        result += std::to_string(b);
      }
    }
    return result;
  }

  /** @brief Prints a generic container for debugging and ad hoc inspection. */
  template<typename T> void print(std::string m, const T& c)
  {
    auto s { c.size() };
    std::cout << m << ": { ";
    for (const auto& e : c)
    {
      std::cout << e << ((--s > 0) ? ", " : " ");
    }
    std::cout << "}" << std::endl;
  }
} // namespace feature_isolation
