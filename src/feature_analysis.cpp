/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "../include/feature_analysis.hpp"
#include <limits>
#include <map>

namespace feature_isolation
{
  namespace
  {
    constexpr const char* logical_false_symbol = "⊥";
    constexpr const char* logical_true_symbol  = "⊤";
    constexpr const char* logical_not_symbol   = "¬";
    constexpr const char* logical_and_symbol   = " ∧ ";
    constexpr const char* logical_or_symbol    = " ∨ ";

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


    struct simple_term_descriptor_t
    {
        theory_feature_category_t category {
          theory_feature_category_t::unclassified
        };
        std::string expression {};
        size_t      literal_count { 0 };
    };

    using simple_term_index_t = std::map<feature_t, simple_term_descriptor_t>;

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

    [[nodiscard]] simple_term_index_t build_simple_term_index(
        unsigned_integer               base_feature_count,
        const std::vector<unsigned_integer>& ordered_valid_system_ids,
        const std::vector<std::string>&      feature_names)
    {
      simple_term_index_t result {};
      const auto pattern_count = power(3, base_feature_count);
      for (auto encoded_pattern { zero }; encoded_pattern < pattern_count;
           ++encoded_pattern)
      {
        auto pattern = encoded_pattern;
        std::vector<int> trits(base_feature_count, 0);
        size_t literal_count {};
        for (auto feature_index { zero }; feature_index < base_feature_count;
             ++feature_index)
        {
          trits.at(feature_index) = static_cast<int>(pattern % 3);
          pattern /= 3;
          if (trits.at(feature_index) != 0)
          {
            ++literal_count;
          }
        }
        if (literal_count == zero)
        {
          continue;
        }

        feature_t conjunction_extent(ordered_valid_system_ids.size(), true);
        feature_t disjunction_extent(ordered_valid_system_ids.size(), false);
        std::vector<std::pair<std::string, bool>> conjunction_literals {};
        std::vector<std::pair<std::string, bool>> disjunction_literals {};
        conjunction_literals.reserve(literal_count);
        disjunction_literals.reserve(literal_count);
        for (auto feature_index { zero }; feature_index < base_feature_count;
             ++feature_index)
        {
          const auto trit = trits.at(feature_index);
          if (trit == 0)
          {
            continue;
          }
          const bool negated = trit == 2;
          conjunction_literals.emplace_back(feature_names.at(feature_index), negated);
          disjunction_literals.emplace_back(feature_names.at(feature_index), negated);
        }

        for (auto system_index { zero }; system_index < ordered_valid_system_ids.size();
             ++system_index)
        {
          const auto minterm = ordered_valid_system_ids.at(system_index) - one;
          bool conjunction_matches = true;
          bool disjunction_matches = false;
          for (auto feature_index { zero }; feature_index < base_feature_count;
               ++feature_index)
          {
            const auto trit = trits.at(feature_index);
            if (trit == 0)
            {
              continue;
            }
            const bool bit_value = ((minterm >> feature_index) & one) == one;
            const bool literal_matches
                = trit == 1 ? bit_value : !bit_value;
            conjunction_matches = conjunction_matches && literal_matches;
            disjunction_matches = disjunction_matches || literal_matches;
          }
          conjunction_extent.at(system_index) = conjunction_matches;
          disjunction_extent.at(system_index) = disjunction_matches;
        }

        const auto conjunction_category
            = literal_count == one
                  ? (conjunction_literals.front().second
                         ? theory_feature_category_t::not_feature
                         : theory_feature_category_t::fundamental)
                  : theory_feature_category_t::and_feature;
        record_simple_descriptor(result,
                                 conjunction_extent,
                                 { conjunction_category,
                                   render_simple_literals(conjunction_literals,
                                                          logical_and_symbol),
                                   literal_count });

        if (literal_count >= 2)
        {
          record_simple_descriptor(result,
                                   disjunction_extent,
                                   { theory_feature_category_t::or_feature,
                                     render_simple_literals(disjunction_literals,
                                                            logical_or_symbol),
                                     literal_count });
        }
      }
      return result;
    }

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

    struct implicant_t
    {
        std::vector<int> bits {};
        feature_ids_t    covered_minterms {};
        size_t           literal_count_cache { 0 };
        size_t           specified_ones { 0 };
    };

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

    [[nodiscard]] size_t literal_count(const implicant_t& implicant)
    {
      return implicant.literal_count_cache;
    }

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

    [[nodiscard]] bool same_bits(const implicant_t& lhs, const implicant_t& rhs)
    {
      return lhs.bits == rhs.bits;
    }

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

    struct exact_cover_search_t
    {
        const std::vector<implicant_t>&  prime_implicants;
        const std::vector<feature_ids_t>& cover_sets;
        std::vector<size_t>              best_subset {};
        size_t                           best_literal_count {
          std::numeric_limits<size_t>::max()
        };
        size_t best_implicant_count { std::numeric_limits<size_t>::max() };

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

        [[nodiscard]] size_t min_literal_cost(const std::vector<size_t>& candidates) const
        {
          size_t result { std::numeric_limits<size_t>::max() };
          for (const auto index : candidates)
          {
            result = std::min(result, literal_count(prime_implicants.at(index)));
          }
          return result == std::numeric_limits<size_t>::max() ? zero : result;
        }

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

    enum class normal_form_t
    {
      dnf,
      cnf
    };

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
      std::optional<std::vector<theory_feature_category_t>> category_filter)
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
      , m_category_filter { std::move(category_filter) }
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
      , m_invalid_system_minterms {
          [this]()
          {
            std::vector<unsigned_integer> invalid_minterms {};
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
          }()
        }
      , m_feature_registry { initialize_feature_registry() }
      , m_systems { initialize_registry_backed_systems() }
  { }

  [[nodiscard]] feature_t feature_analysis_t::at(unsigned_integer feature_id) const
  {
    if (feature_id == zero || feature_id > m_feature_registry.size())
    {
      throw std::out_of_range("Illegal theory feature id.");
    }
    return m_feature_registry.at(feature_id - one).systems_present;
  }

  unsigned_integer feature_analysis_t::category(unsigned_integer feature_id) const
  {
    if (feature_id == zero || feature_id > m_feature_registry.size())
    {
      throw std::out_of_range("Illegal theory feature id.");
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
    if (!m_category_filter.has_value())
    {
      return true;
    }
    return std::ranges::find(m_category_filter.value(), category)
           != m_category_filter.value().end();
  }

  bool feature_analysis_t::includes_feature(unsigned_integer feature_id) const
  {
    if (feature_id == zero || feature_id > m_feature_registry.size())
    {
      throw std::out_of_range("Illegal theory feature id.");
    }
    return includes_category(m_feature_registry.at(feature_id - one).category);
  }

  const system_space_t& feature_analysis_t::system_space() const
  {
    return m_system_space;
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
    for (auto id { zero }; const auto& system : m_systems)
    {
      print_system_statistics(os, system, ++id);
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
    for (auto id { zero }; const auto& system : m_systems)
    {
      print_system_features(os, system, ++id);
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
      const feature_t& systems_present) const
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
        = prime_implicants(m_base_feature_count, minterms, m_invalid_system_minterms);
    const auto dnf_cover = select_cover(m_base_feature_count, minterms, dnf_primes);
    const auto dnf_expression = normal_form_to_string(dnf_cover,
                                                      m_resolved_feature_names,
                                                      normal_form_t::dnf);

    const auto cnf_primes = prime_implicants(m_base_feature_count,
                                             complement_minterms,
                                             m_invalid_system_minterms);
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
    return includes_category(category);
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
        if (should_materialize_details(record.category))
        {
          record.simplified_term = logical_false_symbol;
          record.raw_term        = logical_false_symbol;
          record.display_name    = record.simplified_term;
        }
        result.push_back(record);
        continue;
      }
      if (all_present)
      {
        record.category = theory_feature_category_t::core;
        if (should_materialize_details(record.category))
        {
          record.simplified_term = logical_true_symbol;
          record.raw_term        = raw_term_for(record.systems_present);
          record.display_name    = record.simplified_term;
        }
        result.push_back(record);
        continue;
      }

      const auto simple_match = simple_term_index.find(record.systems_present);
      if (simple_match != simple_term_index.end())
      {
        record.category = simple_match->second.category;
        if (should_materialize_details(record.category))
        {
          record.simplified_term = simple_match->second.expression;
          record.raw_term        = raw_term_for(record.systems_present);
          record.display_name    = record.simplified_term;
        }
        result.push_back(record);
        continue;
      }

      record.category = theory_feature_category_t::complex;
      if (should_materialize_details(record.category))
      {
        record.raw_term = raw_term_for(record.systems_present);
        record.simplified_term = simplified_term_for(record.systems_present);
        record.display_name = record.simplified_term;
      }
      result.push_back(record);
    }
    return result;
  }

  systems_t feature_analysis_t::initialize_registry_backed_systems() const
  {
    systems_t result(m_max_system_id, system_t {});
    for (const auto& record : m_feature_registry)
    {
      for (auto system_index { zero }; system_index < m_ordered_valid_system_ids.size();
           ++system_index)
      {
        if (!record.systems_present.at(system_index))
        {
          continue;
        }
        const auto system_id = m_ordered_valid_system_ids.at(system_index);
        if (system_id == zero || system_id > result.size())
        {
          continue;
        }
        result.at(system_id - one).feature_ids.insert(record.id);
      }
    }
    return result;
  }

  [[nodiscard]] char separator(const char s)
  {
    static char sep = '\t';
    if (s != 0)
    {
      sep = s;
    }
    return sep;
  }

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
