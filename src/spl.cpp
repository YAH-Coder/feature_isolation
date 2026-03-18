/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "../include/spl.hpp"
#include <algorithm>
#include <sstream>
#include <type_traits>
#include <utility>

namespace feature_isolation
{
  char csv_separator { '\t' };

  namespace error_msgs
  {
    const std::string FEATURE_NOT_FOUND
        = "Feature {} not found at line {}, column {}";
    const std::string FEATUREINDEX_NOT_FOUND
        = "Feature {} is not a specified feature name.";
    const std::string SYSTEM_CONSTRAINT
        = "System {} violates {} constraint for feature {} and dependent "
          "feature {}";
    const std::string UNEXPECTED = "Unexpected {}: {} at line {}, column {}";
    const std::string SPECIFIED_MORE_THAN_ONCE
        = "{} specified more than once in file {}";
    const std::string EXPECTED_BUT_GOT = "Expected {} but got: {}";
    const std::string ERROR_READING
        = "Error occurred when reading {} in file {}";
    const std::string DUPLICATE_FOUND
        = "Duplicate {} found: {} at line {}, column {}";
    const std::string FAILED_TO = "Failed to {} {}: {}";
  } // namespace error_msgs

  const std::unordered_map<std::string_view, token_type_t> kw::keywords = {
    {            kw::COMMENT, token_type_t::COMMENT },
    {                kw::SPL, token_type_t::KEYWORD },
    {           kw::FEATURES, token_type_t::KEYWORD },
    {                kw::END, token_type_t::KEYWORD },
    {    kw::MISSING_SYSTEMS, token_type_t::KEYWORD },
    {  kw::AVAILABLE_SYSTEMS, token_type_t::KEYWORD },
    {        kw::CONSTRAINTS, token_type_t::KEYWORD },
    {             kw::ASSERT, token_type_t::KEYWORD },
    {    kw::CATEGORY_FILTER, token_type_t::KEYWORD },
    {       kw::TRUE_LITERAL, token_type_t::KEYWORD },
    {      kw::FALSE_LITERAL, token_type_t::KEYWORD },
    {              kw::COUNT, token_type_t::KEYWORD },
    {                 kw::IN, token_type_t::KEYWORD },
    {             kw::NOT_OP, token_type_t::KEYWORD },
    {             kw::AND_OP, token_type_t::KEYWORD },
    {              kw::OR_OP, token_type_t::KEYWORD },
    {         kw::IMPLIES_OP, token_type_t::KEYWORD },
    {             kw::IFF_OP, token_type_t::KEYWORD },
    {             kw::LPAREN, token_type_t::KEYWORD },
    {             kw::RPAREN, token_type_t::KEYWORD },
    {           kw::LBRACKET, token_type_t::KEYWORD },
    {           kw::RBRACKET, token_type_t::KEYWORD },
    {              kw::COMMA, token_type_t::KEYWORD },
    {              kw::EQ_OP, token_type_t::KEYWORD },
    {              kw::NE_OP, token_type_t::KEYWORD },
    {              kw::LT_OP, token_type_t::KEYWORD },
    {              kw::LE_OP, token_type_t::KEYWORD },
    {              kw::GT_OP, token_type_t::KEYWORD },
    {              kw::GE_OP, token_type_t::KEYWORD },
  };

  namespace
  {
    [[nodiscard]] theory_formula_ptr_t clone_formula(const theory_formula_t& formula)
    {
      return std::make_shared<theory_formula_t>(formula);
    }

    [[nodiscard]] std::string join_strings(const std::vector<std::string>& values,
                                           const std::string& delimiter)
    {
      std::ostringstream os;
      for (std::size_t i { 0 }; i < values.size(); ++i)
      {
        if (i > 0)
        {
          os << delimiter;
        }
        os << values[i];
      }
      return os.str();
    }

    template<typename T>
    [[nodiscard]] std::string stringify(const T& value)
    {
      std::ostringstream os;
      os << value;
      return os.str();
    }

    void replace_next_placeholder(std::string& message, const std::string& replacement)
    {
      const auto pos = message.find("{}");
      if (pos != std::string::npos)
      {
        message.replace(pos, 2, replacement);
      }
    }

    template<typename... Args>
    [[nodiscard]] std::string format_message(const std::string_view& message,
                                             Args&&... args)
    {
      std::string formatted { message };
      (replace_next_placeholder(formatted, stringify(std::forward<Args>(args))), ...);
      return formatted;
    }
  } // namespace

  theory_formula_t::theory_formula_t(payload_t payload)
      : m_payload(std::move(payload))
  { }

  [[nodiscard]] const theory_formula_t::payload_t& theory_formula_t::payload() const
  {
    return m_payload;
  }

  [[nodiscard]] theory_formula_kind_t theory_formula_t::kind() const
  {
    return std::visit(
        [](const auto& node) -> theory_formula_kind_t
        {
          using node_t = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<node_t, feature_ref_t>)
          {
            return theory_formula_kind_t::feature_ref;
          }
          else if constexpr (std::is_same_v<node_t, true_literal_t>)
          {
            return theory_formula_kind_t::true_literal;
          }
          else if constexpr (std::is_same_v<node_t, false_literal_t>)
          {
            return theory_formula_kind_t::false_literal;
          }
          else if constexpr (std::is_same_v<node_t, negation_t>)
          {
            return theory_formula_kind_t::negation;
          }
          else if constexpr (std::is_same_v<node_t, conjunction_t>)
          {
            return theory_formula_kind_t::conjunction;
          }
          else if constexpr (std::is_same_v<node_t, disjunction_t>)
          {
            return theory_formula_kind_t::disjunction;
          }
          else if constexpr (std::is_same_v<node_t, implication_t>)
          {
            return theory_formula_kind_t::implication;
          }
          else if constexpr (std::is_same_v<node_t, equivalence_t>)
          {
            return theory_formula_kind_t::equivalence;
          }
          else if constexpr (std::is_same_v<node_t, theory_count_compare_t>)
          {
            return theory_formula_kind_t::count_compare;
          }
          else
          {
            return theory_formula_kind_t::count_interval;
          }
        },
        m_payload);
  }

  theory_formula_t theory_formula_t::feature(
      const std::string&               name,
      std::optional<unsigned_integer> compiled_feature_index)
  {
    return theory_formula_t(feature_ref_t { name, compiled_feature_index });
  }

  theory_formula_t theory_formula_t::true_literal()
  {
    return theory_formula_t(true_literal_t {});
  }

  theory_formula_t theory_formula_t::false_literal()
  {
    return theory_formula_t(false_literal_t {});
  }

  theory_formula_t theory_formula_t::negation(const theory_formula_t& operand)
  {
    return theory_formula_t(negation_t { clone_formula(operand) });
  }

  theory_formula_t theory_formula_t::conjunction(const std::vector<theory_formula_t>& operands)
  {
    std::vector<theory_formula_ptr_t> formula_operands {};
    for (const auto& operand : operands)
    {
      formula_operands.push_back(clone_formula(operand));
    }
    return theory_formula_t(conjunction_t { formula_operands });
  }

  theory_formula_t theory_formula_t::disjunction(const std::vector<theory_formula_t>& operands)
  {
    std::vector<theory_formula_ptr_t> formula_operands {};
    for (const auto& operand : operands)
    {
      formula_operands.push_back(clone_formula(operand));
    }
    return theory_formula_t(disjunction_t { formula_operands });
  }

  theory_formula_t theory_formula_t::implication(const theory_formula_t& lhs,
                                                 const theory_formula_t& rhs)
  {
    return theory_formula_t(implication_t { clone_formula(lhs), clone_formula(rhs) });
  }

  theory_formula_t theory_formula_t::equivalence(const theory_formula_t& lhs,
                                                 const theory_formula_t& rhs)
  {
    return theory_formula_t(equivalence_t { clone_formula(lhs), clone_formula(rhs) });
  }

  theory_formula_t theory_formula_t::count_compare_formula(
      const feature_names_t&               features,
      const std::vector<unsigned_integer>& compiled_feature_indices,
      theory_count_compare_op_t            op,
      unsigned_integer                     value)
  {
    return theory_formula_t(
        theory_count_compare_t { features, compiled_feature_indices, op, value });
  }

  theory_formula_t theory_formula_t::count_interval_formula(
      const feature_names_t&               features,
      const std::vector<unsigned_integer>& compiled_feature_indices,
      unsigned_integer                     lower,
      unsigned_integer                     upper)
  {
    return theory_formula_t(theory_count_interval_t {
        features, compiled_feature_indices, lower, upper });
  }

  [[nodiscard]] std::string to_string(theory_count_compare_op_t op)
  {
    switch (op)
    {
      case theory_count_compare_op_t::eq:
        return "=";
      case theory_count_compare_op_t::ne:
        return "!=";
      case theory_count_compare_op_t::lt:
        return "<";
      case theory_count_compare_op_t::le:
        return "<=";
      case theory_count_compare_op_t::gt:
        return ">";
      case theory_count_compare_op_t::ge:
        return ">=";
    }
    return "?";
  }

  [[nodiscard]] std::string to_string(const theory_formula_t& formula)
  {
    return std::visit(
        [](const auto& node) -> std::string
        {
          using node_t = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<node_t, theory_formula_t::feature_ref_t>)
          {
            return node.feature_name;
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::true_literal_t>)
          {
            return "TRUE";
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::false_literal_t>)
          {
            return "FALSE";
          }
          else if constexpr (std::is_same_v<node_t, theory_formula_t::negation_t>)
          {
            return "(!" + to_string(*node.operand) + ")";
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::conjunction_t>)
          {
            std::vector<std::string> rendered {};
            for (const auto& operand : node.operands)
            {
              rendered.push_back(to_string(*operand));
            }
            return "(" + join_strings(rendered, " && ") + ")";
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::disjunction_t>)
          {
            std::vector<std::string> rendered {};
            for (const auto& operand : node.operands)
            {
              rendered.push_back(to_string(*operand));
            }
            return "(" + join_strings(rendered, " || ") + ")";
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::implication_t>)
          {
            return "(" + to_string(*node.lhs) + " -> " + to_string(*node.rhs)
                   + ")";
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::equivalence_t>)
          {
            return "(" + to_string(*node.lhs) + " <-> " + to_string(*node.rhs)
                   + ")";
          }
          else if constexpr (std::is_same_v<node_t, theory_count_compare_t>)
          {
            return "COUNT(" + join_strings(node.features, ", ") + ") "
                   + to_string(node.op) + " " + std::to_string(node.value);
          }
          else
          {
            return "COUNT(" + join_strings(node.features, ", ") + ") IN ["
                   + std::to_string(node.lower) + ","
                   + std::to_string(node.upper) + "]";
          }
        },
        formula.payload());
  }

  [[nodiscard]] std::string to_string(const theory_category_filter_t& filter)
  {
    std::vector<std::string> rendered {};
    rendered.reserve(filter.size());
    for (const auto category : filter)
    {
      rendered.push_back(::feature_isolation::to_string(category));
    }
    return join_strings(rendered, ", ");
  }


  template<typename... Args>
  [[noreturn]] void error(const std::string_view& msg, Args&&... args)
  {
    std::string formatted_msg
        = format_message(msg, std::forward<Args>(args)...);
    throw std::runtime_error(formatted_msg);
  }

  std::string toupper(std::string s)
  {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
  }

  [[nodiscard]] bool system_holds_feature(unsigned_integer system_id,
                                          unsigned_integer feature_index)
  {
    const auto system = system_id - one;
    return ((system >> feature_index) & one) == one;
  }

  [[nodiscard]] unsigned_integer resolve_feature_index(
      const feature_names_t& feature_names,
      const std::string&     feature_name)
  {
    const auto feature_it = std::find(feature_names.begin(),
                                      feature_names.end(),
                                      feature_name);
    if (feature_it == feature_names.end())
    {
      error("Unknown feature referenced in theory formula: {}", feature_name);
    }
    return static_cast<unsigned_integer>(
        std::distance(feature_names.begin(), feature_it));
  }

  [[nodiscard]] bool system_holds_feature(const feature_names_t& feature_names,
                                          unsigned_integer       system_id,
                                          const theory_formula_t::feature_ref_t& feature_ref)
  {
    const auto feature_index = feature_ref.compiled_feature_index.value_or(
        resolve_feature_index(feature_names, feature_ref.feature_name));
    return system_holds_feature(system_id, feature_index);
  }

  [[nodiscard]] unsigned_integer count_true_features(
      const feature_names_t&               feature_names,
      const feature_names_t&               counted_features,
      const std::vector<unsigned_integer>& compiled_feature_indices,
      unsigned_integer                     system_id)
  {
    if (!compiled_feature_indices.empty())
    {
      return static_cast<unsigned_integer>(std::count_if(
          compiled_feature_indices.begin(),
          compiled_feature_indices.end(),
          [&](const auto feature_index)
          {
            return system_holds_feature(system_id, feature_index);
          }));
    }

    return static_cast<unsigned_integer>(std::count_if(
        counted_features.begin(),
        counted_features.end(),
        [&](const auto& feature_name)
        {
          return system_holds_feature(
              feature_names,
              system_id,
              theory_formula_t::feature_ref_t { feature_name, std::nullopt });
        }));
  }

  [[nodiscard]] bool evaluate_formula(const feature_names_t& feature_names,
                                      const theory_formula_t& formula,
                                      unsigned_integer        system_id)
  {
    return std::visit(
        [&](const auto& node) -> bool
        {
          using node_t = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<node_t, theory_formula_t::feature_ref_t>)
          {
            return system_holds_feature(feature_names, system_id, node);
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::true_literal_t>)
          {
            return true;
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::false_literal_t>)
          {
            return false;
          }
          else if constexpr (std::is_same_v<node_t, theory_formula_t::negation_t>)
          {
            return !evaluate_formula(feature_names, *node.operand, system_id);
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::conjunction_t>)
          {
            return std::ranges::all_of(
                node.operands,
                [&](const auto& operand)
                {
                  return evaluate_formula(feature_names, *operand, system_id);
                });
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::disjunction_t>)
          {
            return std::ranges::any_of(
                node.operands,
                [&](const auto& operand)
                {
                  return evaluate_formula(feature_names, *operand, system_id);
                });
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::implication_t>)
          {
            return !evaluate_formula(feature_names, *node.lhs, system_id)
                   || evaluate_formula(feature_names, *node.rhs, system_id);
          }
          else if constexpr (std::is_same_v<node_t,
                                            theory_formula_t::equivalence_t>)
          {
            return evaluate_formula(feature_names, *node.lhs, system_id)
                   == evaluate_formula(feature_names, *node.rhs, system_id);
          }
          else if constexpr (std::is_same_v<node_t, theory_count_compare_t>)
          {
            const auto satisfied = count_true_features(feature_names,
                                                       node.features,
                                                       node.compiled_feature_indices,
                                                       system_id);
            switch (node.op)
            {
              case theory_count_compare_op_t::eq:
                return satisfied == node.value;
              case theory_count_compare_op_t::ne:
                return satisfied != node.value;
              case theory_count_compare_op_t::lt:
                return satisfied < node.value;
              case theory_count_compare_op_t::le:
                return satisfied <= node.value;
              case theory_count_compare_op_t::gt:
                return satisfied > node.value;
              case theory_count_compare_op_t::ge:
                return satisfied >= node.value;
            }
            return false;
          }
          else
          {
            const auto satisfied = count_true_features(feature_names,
                                                       node.features,
                                                       node.compiled_feature_indices,
                                                       system_id);
            return node.lower <= satisfied && satisfied <= node.upper;
          }
        },
        formula.payload());
  }

  [[nodiscard]] bool system_satisfies_constraints(
      const feature_names_t& feature_names,
      const theory_constraints_t& constraints,
      unsigned_integer system_id)
  {
    return std::ranges::all_of(
        constraints,
        [&](const auto& constraint)
        {
          return evaluate_formula(feature_names, constraint, system_id);
        });
  }

  [[nodiscard]] system_space_t compute_system_space(
      unsigned_integer max_available_systems,
      const feature_names_t& feature_names,
      const theory_constraints_t& constraints,
      const spl_specification_t::systems_t& available_systems,
      const spl_specification_t::systems_t& missing_systems,
      bool available_systems_specified,
      bool missing_systems_specified)
  {
    system_space_t result {};
    for (auto system_id { one }; system_id <= max_available_systems; ++system_id)
    {
      if (system_satisfies_constraints(feature_names, constraints, system_id))
      {
        result.valid.insert(system_id);
      }
    }

    if (available_systems_specified)
    {
      for (const auto system_id : available_systems)
      {
        if (result.valid.contains(system_id))
        {
          result.available.insert(system_id);
        }
      }
      result.missing = result.valid;
      for (const auto system_id : result.available)
      {
        result.missing.erase(system_id);
      }
      return result;
    }

    if (missing_systems_specified)
    {
      for (const auto system_id : missing_systems)
      {
        if (result.valid.contains(system_id))
        {
          result.missing.insert(system_id);
        }
      }
      result.available = result.valid;
      for (const auto system_id : result.missing)
      {
        result.available.erase(system_id);
      }
      return result;
    }

    result.available = result.valid;
    return result;
  }

  [[nodiscard]] bool system_satisfies_constraints(
      const spl_specification_t& spec, unsigned_integer system_id)
  {
    return system_satisfies_constraints(
        spec.m_feature_names, spec.m_constraints, system_id);
  }

  [[nodiscard]] system_ids_t valid_system_ids(const spl_specification_t& spec)
  {
    return spec.system_space().valid;
  }

  [[nodiscard]] system_ids_t available_valid_system_ids(
      const spl_specification_t& spec)
  {
    return spec.system_space().available;
  }

  [[nodiscard]] system_ids_t missing_valid_system_ids(
      const spl_specification_t& spec)
  {
    return spec.system_space().missing;
  }

  [[nodiscard]] system_space_t theory_system_space(const spl_specification_t& spec)
  {
    return spec.system_space();
  }

  [[nodiscard]] theory_dsl_coverage_t
      theory_dsl_coverage(const spl_specification_t& spec)
  {
    theory_dsl_coverage_t result {};

    result.supported_capabilities.push_back(
        "FEATURES defines the Boolean feature basis used to enumerate candidate systems.");

    if (spec.m_constraints_specified || !spec.m_constraints.empty())
    {
      result.supported_capabilities.push_back(
          "CONSTRAINTS stores first-class Boolean and cardinality formulas.");
    }
    else
    {
      result.supported_capabilities.push_back(
          "If CONSTRAINTS is omitted, the valid system space is the unconstrained powerset over FEATURES.");
    }

    if (spec.m_category_filter_specified && spec.m_category_filter.has_value())
    {
      result.supported_capabilities.push_back(
          "CATEGORY_FILTER stores theory-native post-classification category filters.");
    }

    if (spec.m_available_systems_specified && spec.m_missing_systems_specified)
    {
      result.supported_capabilities.push_back(
          "AVAILABLE_SYSTEMS and MISSING_SYSTEMS can partition the valid system space.");
    }
    else if (spec.m_available_systems_specified)
    {
      result.supported_capabilities.push_back(
          "AVAILABLE_SYSTEMS constrains the observed system space without changing validity.");
    }
    else if (spec.m_missing_systems_specified)
    {
      result.supported_capabilities.push_back(
          "MISSING_SYSTEMS marks valid-but-unavailable systems on top of the valid system space.");
    }
    else
    {
      result.supported_capabilities.push_back(
          "If no availability section is given, all valid systems are treated as available.");
    }

    result.unsupported_capabilities.push_back(
        "Feature attributes, multi-valued features, and repeated-feature multiplicity are outside the current DSL.");

    return result;
  }

  spl_specification_t::spl_specification_t(
      const std::string&                    filename,
      const std::optional<std::string>&     spl_name,
      unsigned_integer                      F,
      unsigned_integer                      max_available_systems,
      const feature_names_t&                feature_names,
      const theory_constraints_t&           constraints,
      const std::optional<theory_category_filter_t>& category_filter,
      const spl_specification_t::systems_t& available_systems,
      const spl_specification_t::systems_t& missing_systems,
      bool                                  spl_specified,
      bool                                  feature_names_specified,
      bool                                  constraints_specified,
      bool                                  category_filter_specified,
      bool                                  missing_systems_specified,
      bool                                  available_systems_specified)
      : m_filename(filename)
      , m_spl_name(spl_name.value_or("NOT SPECIFIED"))
      , m_F(F)
      , m_max_available_systems(max_available_systems)
      , m_feature_names(feature_names)
      , m_constraints(constraints)
      , m_category_filter(category_filter)
      , m_available_systems(available_systems)
      , m_missing_systems(missing_systems)
      , m_system_space(compute_system_space(max_available_systems,
                                            feature_names,
                                            constraints,
                                            available_systems,
                                            missing_systems,
                                            available_systems_specified,
                                            missing_systems_specified))
      , m_spl_specified(spl_specified)
      , m_feature_names_specified(feature_names_specified)
      , m_constraints_specified(constraints_specified)
      , m_category_filter_specified(category_filter_specified)
      , m_missing_systems_specified(missing_systems_specified)
      , m_available_systems_specified(available_systems_specified)
  {
    if (!validate_systems())
    {
      error(
          "Failed to validate systems during SPL specification construction.");
    }
  }

  const system_space_t& spl_specification_t::system_space() const
  {
    return m_system_space;
  }

  bool spl_specification_t::validate_systems() const
  {
    const auto& valid = m_system_space.valid;

    const auto validate_explicit_systems = [&](const systems_t& systems,
                                               const std::string_view section)
    {
      for (const auto system_id : systems)
      {
        if (!valid.contains(system_id))
        {
          error("System {} listed in {} is not valid under the declared constraints.",
                system_id,
                section);
        }
      }
    };

    if (m_available_systems_specified)
    {
      validate_explicit_systems(m_available_systems, kw::AVAILABLE_SYSTEMS);
    }
    if (m_missing_systems_specified)
    {
      validate_explicit_systems(m_missing_systems, kw::MISSING_SYSTEMS);
    }

    if (m_available_systems_specified && m_missing_systems_specified)
    {
      system_ids_t available_specified {};
      system_ids_t missing_specified {};
      for (const auto system_id : m_available_systems)
      {
        available_specified.insert(system_id);
      }
      for (const auto system_id : m_missing_systems)
      {
        if (available_specified.contains(system_id))
        {
          error("System {} cannot be listed in both AVAILABLE_SYSTEMS and MISSING_SYSTEMS.",
                system_id);
        }
        missing_specified.insert(system_id);
      }

      system_ids_t declared = available_specified;
      declared.insert(missing_specified.begin(), missing_specified.end());
      if (declared != valid)
      {
        error("AVAILABLE_SYSTEMS and MISSING_SYSTEMS must form a disjoint partition of the valid system space.");
      }
    }

    return true;
  }


  token_t::token_t(const token_type_t& tt,
                   const std::string&  v,
                   const position_t&   b,
                   const position_t&   e)
      : m_token_type(tt)
      , m_value(v)
      , m_begin(b)
      , m_end(e)
  { }

  [[nodiscard]] const token_type_t& token_t::type() const
  {
    return m_token_type;
  }

  [[nodiscard]] const std::string& token_t::value() const
  {
    if (m_token_type == token_type_t::EMPTY)
    {
      error("token_t::value() MUST not be called if m_token_type == "
            "token_type_t::EMPTY.");
    }
    return m_value;
  }

  [[nodiscard]] const position_t& token_t::begin() const
  {
    if (m_token_type == token_type_t::EMPTY)
    {
      error("token_t::begin() MUST not be called if m_token_type == "
            "token_type_t::EMPTY.");
    }
    return m_begin;
  }

  [[nodiscard]] const position_t& token_t::end() const
  {
    if (m_token_type == token_type_t::EMPTY)
    {
      error("token_t::end() MUST not be called if m_token_type == "
            "token_type_t::EMPTY.");
    }
    return m_end;
  }

  [[nodiscard]] const std::string token_t::position_line() const
  {
    return (std::to_string(m_begin.first));
  }

  [[nodiscard]] const std::string token_t::position_column() const
  {
    return (std::to_string(m_begin.second));
  }

  tokenizer_t::tokenizer_t(std::istream& is): m_is { is }, m_position { 1, 0 }
  { }

  token_t tokenizer_t::next()
  {
    while (m_is)
    {
      char c = static_cast<char>(m_is.peek());

      if (is_whitespace(c))
      {
        consume_char(c);
        continue;
      }

      if (c == '/')
      {
        consume_char(c); // Consume '/'
        if (m_is.peek() == '/')
        {
          consume_char(c); // Consume second '/'
          skip_line();
          continue;
        }
        else
        {
          // Not a comment, ignore the '/'
          continue;
        }
      }

      if (is_token_char(c))
      {
        return parse_token();
      }
      if (c == '(' || c == ')' || c == '[' || c == ']' || c == ',' || c == '='
          || c == '<' || c == '>' || c == '!' || c == '-' || c == '&'
          || c == '|')
      {
        const auto token_begin = m_position;
        std::string symbol;
        consume_char(c);
        symbol.push_back(c);

        if ((symbol == "-" && m_is.peek() == '>')
            || (symbol == "<" && m_is.peek() == '-')
            || (symbol == "<" && m_is.peek() == '=')
            || (symbol == ">" && m_is.peek() == '=')
            || (symbol == "!" && m_is.peek() == '=')
            || (symbol == "&" && m_is.peek() == '&')
            || (symbol == "|" && m_is.peek() == '|'))
        {
          char next_char;
          consume_char(next_char);
          symbol.push_back(next_char);

          if (symbol == "<-")
          {
            if (m_is.peek() == '>')
            {
              consume_char(next_char);
              symbol.push_back(next_char);
            }
          }
        }

        const auto token_end = m_position;
        auto       keyword_check = check_keyword(symbol);
        if (keyword_check.has_value())
        {
          return token_t(keyword_check->type(), symbol, token_begin, token_end);
        }

        error(error_msgs::UNEXPECTED,
              "token",
              symbol,
              std::to_string(token_begin.first),
              std::to_string(token_begin.second));
      }
      else
      {
        // Unknown character, consume and skip
        consume_char(c);
        continue;
      }
    }

    // At EOF
    return token_t(token_type_t::EMPTY, "", m_position, m_position);
  }

  bool tokenizer_t::is_token_char(char c) const
  {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '#'
           || c == '.';
  }

  bool tokenizer_t::is_whitespace(char c) const
  {
    return std::isspace(static_cast<unsigned char>(c));
  }

  void tokenizer_t::consume_char(char& c)
  {
    m_is.get(c);
    if (c == '\n')
    {
      m_position.first++;
      m_position.second = 1;
    }
    else
    {
      m_position.second++;
    }
  }

  void tokenizer_t::skip_line()
  {
    char c;
    while (m_is.get(c))
    {
      if (c == '\n')
      {
        m_position.first++;
        m_position.second = 1;
        break;
      }
      else
      {
        m_position.second++;
      }
    }
  }

  token_t tokenizer_t::parse_token()
  {
    std::string token;
    position_t  token_begin = m_position;
    char        c;

    while (m_is && is_token_char(m_is.peek()))
    {
      consume_char(c);
      token += c;
    }

    position_t token_end = m_position;

    // Determine token type
    if (std::all_of(token.begin(),
                    token.end(),
                    [](unsigned char ch) { return std::isdigit(ch) != 0; }))
    {
      return token_t(token_type_t::VALUE, token, token_begin, token_end);
    }
    else
    {
      auto keyword_check = check_keyword(toupper(token));
      if (keyword_check.has_value())
      {
        return token_t(keyword_check->type(), token, token_begin, token_end);
      }
      else
      {
        return token_t(token_type_t::NAME, token, token_begin, token_end);
      }
    }
  }

  std::optional<token_t> tokenizer_t::check_keyword(const std::string& token)
  {
    using namespace kw;
    std::string upper_token = toupper(token);

    auto it = keywords.find(upper_token);
    if (it != keywords.end())
    {
      return token_t(it->second, token, m_position, m_position);
    }
    return std::nullopt;
  }

  parser_t::parser_t(std::istream& is, std::string filename)
      : m_tokenizer { is }
      , m_spec { parse_specification(filename) }
  { }

  token_t parser_t::next_token()
  {
    if (m_buffered_token.has_value())
    {
      auto token = *m_buffered_token;
      m_buffered_token.reset();
      return token;
    }
    return m_tokenizer.next();
  }

  token_t parser_t::peek_token()
  {
    if (!m_buffered_token.has_value())
    {
      m_buffered_token = m_tokenizer.next();
    }
    return *m_buffered_token;
  }

  bool parser_t::is_section_keyword(const token_t& token) const
  {
    if (token.type() != token_type_t::KEYWORD)
    {
      return false;
    }
    const auto upper = toupper(token.value());
    return upper == kw::SPL || upper == kw::FEATURES || upper == kw::CONSTRAINTS
           || upper == kw::AVAILABLE_SYSTEMS || upper == kw::MISSING_SYSTEMS
           || upper == kw::CATEGORY_FILTER;
  }

  spl_specification_t parser_t::parse_specification(const std::string& filename)
  {
    std::optional<std::string>      spl_name;
    unsigned_integer                max_available_systems = 0;
    unsigned_integer                F                     = 3;
    feature_names_t                 feature_names;
    theory_constraints_t            constraints {};
    std::optional<theory_category_filter_t> category_filter = std::nullopt;
    spl_specification_t::systems_t  available_systems;
    spl_specification_t::systems_t  missing_systems;
    bool spl_specified = false, feature_names_specified = false,
         constraints_specified = false, category_filter_specified = false,
         missing_systems_specified = false, available_systems_specified = false;

    while (true)
    {
      auto token = next_token();
      if (token.type() == token_type_t::EMPTY)
      {
        break;
      }
      if (token.type() == token_type_t::COMMENT)
      {
        continue;
      }
      if (token.type() != token_type_t::KEYWORD)
      {
        error(error_msgs::UNEXPECTED,
              "",
              token.value(),
              token.position_line(),
              token.position_column());
      }

      const auto keyword = toupper(token.value());
      if (keyword == kw::SPL)
      {
        spl_name      = parse_spl();
        spl_specified = true;
      }
      else if (keyword == kw::FEATURES)
      {
        auto [parsed_F, parsed_features] = parse_features();
        F                                = parsed_F;
        feature_names                    = parsed_features;
        feature_names_specified          = true;
      }
      else if (keyword == kw::CONSTRAINTS)
      {
        constraints           = parse_constraints(feature_names);
        constraints_specified = true;
      }
      else if (keyword == kw::CATEGORY_FILTER)
      {
        category_filter           = parse_category_filter();
        category_filter_specified = true;
      }
      else if (keyword == kw::AVAILABLE_SYSTEMS)
      {
        available_systems           = parse_available_systems();
        available_systems_specified = true;
      }
      else if (keyword == kw::MISSING_SYSTEMS)
      {
        missing_systems           = parse_missing_systems();
        missing_systems_specified = true;
      }
      else
      {
        error(error_msgs::UNEXPECTED,
              "keyword",
              token.value(),
              token.position_line(),
              token.position_column());
      }
    }

    max_available_systems = (1 << F);

    return spl_specification_t(filename,
                               spl_name,
                               F,
                               max_available_systems,
                               feature_names,
                               constraints,
                               category_filter,
                               available_systems,
                               missing_systems,
                               spl_specified,
                               feature_names_specified,
                               constraints_specified,
                               category_filter_specified,
                               missing_systems_specified,
                               available_systems_specified);
  }
  std::optional<signed int>
      parser_t::feature_index(const std::string&     feature,
                              const feature_names_t& feature_names) const
  {
    auto it = std::find(feature_names.begin(), feature_names.end(), feature);
    if (it == feature_names.end())
    {
      error(error_msgs::FEATUREINDEX_NOT_FOUND, feature);
    }
    return std::distance(feature_names.begin(), it);
  }

  std::optional<std::string> parser_t::parse_spl()
  {
    auto token = next_token();
    return token.value();
  }

  std::pair<unsigned_integer, feature_names_t> parser_t::parse_features()
  {
    auto             token = next_token();
    unsigned_integer f;

    try
    {
      f = std::stoull(token.value());
    }
    catch (const std::invalid_argument&)
    {
      error("Failed to parse SPL specification: expected a numeric feature "
            "count, but got '{}'",
            token.value());
    }
    catch (const std::out_of_range&)
    {
      error("Failed to parse SPL specification: feature count '{}' is out of "
            "range.",
            token.value());
    }

    feature_names_t feature_names(f);
    for (auto& feature : feature_names)
    {
      token   = next_token();
      feature = token.value();
    }
    return { f, feature_names };
  }

  theory_constraints_t parser_t::parse_constraints(const feature_names_t& feature_names)
  {
    theory_constraints_t constraints {};
    while (true)
    {
      auto token = next_token();
      if (token.type() == token_type_t::EMPTY)
      {
        error(error_msgs::EXPECTED_BUT_GOT, std::string(kw::END), "EOF");
      }
      if (token.type() == token_type_t::KEYWORD && toupper(token.value()) == kw::END)
      {
        break;
      }
      if (token.type() != token_type_t::KEYWORD || toupper(token.value()) != kw::ASSERT)
      {
        error(error_msgs::EXPECTED_BUT_GOT,
              std::string(kw::ASSERT),
              token.value(),
              token.position_line(),
              token.position_column());
      }
      constraints.push_back(parse_formula(feature_names));
    }
    return constraints;
  }

  std::optional<theory_category_filter_t> parser_t::parse_category_filter()
  {
    theory_category_filter_t filter {};
    std::unordered_set<std::string> seen {};

    while (true)
    {
      auto token = peek_token();
      if (token.type() == token_type_t::EMPTY)
      {
        break;
      }
      if (token.type() == token_type_t::KEYWORD)
      {
        const auto upper = toupper(token.value());
        if (upper == kw::END)
        {
          next_token();
          break;
        }
        if (is_section_keyword(token))
        {
          break;
        }
      }

      token = next_token();
      const auto upper = toupper(token.value());
      if (seen.contains(upper))
      {
        error(error_msgs::DUPLICATE_FOUND,
              "theory category",
              token.value(),
              token.position_line(),
              token.position_column());
      }
      filter.push_back(parse_theory_category(token));
      seen.insert(upper);
    }

    return filter;
  }

  theory_formula_t parser_t::parse_formula(const feature_names_t& feature_names)
  {
    return parse_equivalence(feature_names);
  }

  theory_formula_t parser_t::parse_equivalence(const feature_names_t& feature_names)
  {
    auto lhs = parse_implication(feature_names);
    while (true)
    {
      auto token = peek_token();
      if (token.type() == token_type_t::KEYWORD && token.value() == kw::IFF_OP)
      {
        next_token();
        auto rhs = parse_implication(feature_names);
        lhs = theory_formula_t::equivalence(lhs, rhs);
        continue;
      }
      break;
    }
    return lhs;
  }

  theory_formula_t parser_t::parse_implication(const feature_names_t& feature_names)
  {
    auto lhs = parse_disjunction(feature_names);
    auto token = peek_token();
    if (token.type() == token_type_t::KEYWORD && token.value() == kw::IMPLIES_OP)
    {
      next_token();
      auto rhs = parse_implication(feature_names);
      return theory_formula_t::implication(lhs, rhs);
    }
    return lhs;
  }

  theory_formula_t parser_t::parse_disjunction(const feature_names_t& feature_names)
  {
    std::vector<theory_formula_t> operands { parse_conjunction(feature_names) };
    while (true)
    {
      auto token = peek_token();
      if (token.type() == token_type_t::KEYWORD && token.value() == kw::OR_OP)
      {
        next_token();
        operands.push_back(parse_conjunction(feature_names));
        continue;
      }
      break;
    }
    if (operands.size() == 1)
    {
      return operands.front();
    }
    return theory_formula_t::disjunction(operands);
  }

  theory_formula_t parser_t::parse_conjunction(const feature_names_t& feature_names)
  {
    std::vector<theory_formula_t> operands { parse_unary(feature_names) };
    while (true)
    {
      auto token = peek_token();
      if (token.type() == token_type_t::KEYWORD && token.value() == kw::AND_OP)
      {
        next_token();
        operands.push_back(parse_unary(feature_names));
        continue;
      }
      break;
    }
    if (operands.size() == 1)
    {
      return operands.front();
    }
    return theory_formula_t::conjunction(operands);
  }

  theory_formula_t parser_t::parse_unary(const feature_names_t& feature_names)
  {
    auto token = peek_token();
    if (token.type() == token_type_t::KEYWORD && token.value() == kw::NOT_OP)
    {
      next_token();
      return theory_formula_t::negation(parse_unary(feature_names));
    }
    return parse_atom(feature_names);
  }

  theory_formula_t parser_t::parse_atom(const feature_names_t& feature_names)
  {
    auto token = next_token();
    if (token.type() == token_type_t::NAME)
    {
      const auto compiled_feature_index = static_cast<unsigned_integer>(
          feature_index(token.value(), feature_names).value());
      return theory_formula_t::feature(token.value(), compiled_feature_index);
    }
    if (token.type() == token_type_t::KEYWORD && toupper(token.value()) == kw::TRUE_LITERAL)
    {
      return theory_formula_t::true_literal();
    }
    if (token.type() == token_type_t::KEYWORD && toupper(token.value()) == kw::FALSE_LITERAL)
    {
      return theory_formula_t::false_literal();
    }
    if (token.type() == token_type_t::KEYWORD && token.value() == kw::LPAREN)
    {
      auto nested = parse_formula(feature_names);
      auto closing = next_token();
      if (closing.type() != token_type_t::KEYWORD || closing.value() != kw::RPAREN)
      {
        error(error_msgs::EXPECTED_BUT_GOT,
              std::string(kw::RPAREN),
              closing.value(),
              closing.position_line(),
              closing.position_column());
      }
      return nested;
    }
    if (token.type() == token_type_t::KEYWORD && toupper(token.value()) == kw::COUNT)
    {
      return parse_count_expression(feature_names);
    }
    error(error_msgs::UNEXPECTED,
          "formula token",
          token.value(),
          token.position_line(),
          token.position_column());
  }

  feature_names_t parser_t::parse_count_feature_list(const feature_names_t& feature_names)
  {
    feature_names_t listed_features {};
    std::unordered_set<std::string> seen_features {};
    while (true)
    {
      auto token = next_token();
      if (token.type() != token_type_t::NAME)
      {
        error(error_msgs::EXPECTED_BUT_GOT,
              "feature name",
              token.value(),
              token.position_line(),
              token.position_column());
      }
      feature_index(token.value(), feature_names);
      if (seen_features.contains(token.value()))
      {
        error(error_msgs::DUPLICATE_FOUND,
              "COUNT feature",
              token.value(),
              token.position_line(),
              token.position_column());
      }
      listed_features.push_back(token.value());
      seen_features.insert(token.value());

      auto separator = peek_token();
      if (separator.type() == token_type_t::KEYWORD && separator.value() == kw::COMMA)
      {
        next_token();
        continue;
      }
      break;
    }
    return listed_features;
  }

  theory_formula_t parser_t::parse_count_expression(const feature_names_t& feature_names)
  {
    auto token = next_token();
    if (token.type() != token_type_t::KEYWORD || token.value() != kw::LPAREN)
    {
      error(error_msgs::EXPECTED_BUT_GOT,
            std::string(kw::LPAREN),
            token.value(),
            token.position_line(),
            token.position_column());
    }

    const auto listed_features = parse_count_feature_list(feature_names);
    std::vector<unsigned_integer> compiled_feature_indices {};
    compiled_feature_indices.reserve(listed_features.size());
    for (const auto& feature_name : listed_features)
    {
      compiled_feature_indices.push_back(static_cast<unsigned_integer>(
          feature_index(feature_name, feature_names).value()));
    }

    token = next_token();
    if (token.type() != token_type_t::KEYWORD || token.value() != kw::RPAREN)
    {
      error(error_msgs::EXPECTED_BUT_GOT,
            std::string(kw::RPAREN),
            token.value(),
            token.position_line(),
            token.position_column());
    }

    token = next_token();
    if (token.type() != token_type_t::KEYWORD)
    {
      error(error_msgs::EXPECTED_BUT_GOT,
            "COUNT comparator",
            token.value(),
            token.position_line(),
            token.position_column());
    }

    const auto upper = toupper(token.value());
    if (upper == kw::IN)
    {
      auto open = next_token();
      if (open.type() != token_type_t::KEYWORD || open.value() != kw::LBRACKET)
      {
        error(error_msgs::EXPECTED_BUT_GOT,
              std::string(kw::LBRACKET),
              open.value(),
              open.position_line(),
              open.position_column());
      }
      auto lower = next_token();
      auto comma = next_token();
      auto upper_value = next_token();
      auto close = next_token();
      if (lower.type() != token_type_t::VALUE || comma.type() != token_type_t::KEYWORD
          || comma.value() != kw::COMMA || upper_value.type() != token_type_t::VALUE
          || close.type() != token_type_t::KEYWORD || close.value() != kw::RBRACKET)
      {
        error(error_msgs::EXPECTED_BUT_GOT,
              "COUNT interval",
              lower.value() + " " + comma.value() + " " + upper_value.value()
                  + " " + close.value());
      }
      const auto lower_bound = std::stoull(lower.value());
      const auto upper_bound = std::stoull(upper_value.value());
      if (lower_bound > upper_bound)
      {
        error("Invalid COUNT interval [{},{}]: lower bound must be <= upper bound.",
              lower_bound,
              upper_bound);
      }
      return theory_formula_t::count_interval_formula(
          listed_features, compiled_feature_indices, lower_bound, upper_bound);
    }

    theory_count_compare_op_t op {};
    if (token.value() == kw::EQ_OP)
    {
      op = theory_count_compare_op_t::eq;
    }
    else if (token.value() == kw::NE_OP)
    {
      op = theory_count_compare_op_t::ne;
    }
    else if (token.value() == kw::LT_OP)
    {
      op = theory_count_compare_op_t::lt;
    }
    else if (token.value() == kw::LE_OP)
    {
      op = theory_count_compare_op_t::le;
    }
    else if (token.value() == kw::GT_OP)
    {
      op = theory_count_compare_op_t::gt;
    }
    else if (token.value() == kw::GE_OP)
    {
      op = theory_count_compare_op_t::ge;
    }
    else
    {
      error(error_msgs::UNEXPECTED,
            "COUNT comparator",
            token.value(),
            token.position_line(),
            token.position_column());
    }

    auto value = next_token();
    if (value.type() != token_type_t::VALUE)
    {
      error(error_msgs::EXPECTED_BUT_GOT,
            "numeric COUNT bound",
            value.value(),
            value.position_line(),
            value.position_column());
    }
    return theory_formula_t::count_compare_formula(listed_features,
                                                   compiled_feature_indices,
                                                   op,
                                                   std::stoull(value.value()));
  }

  theory_feature_category_t parser_t::parse_theory_category(const token_t& token) const
  {
    const auto upper = toupper(token.value());
    if (upper == "IMPOSSIBLE")
    {
      return theory_feature_category_t::impossible;
    }
    if (upper == "CORE")
    {
      return theory_feature_category_t::core;
    }
    if (upper == "FUNDAMENTAL")
    {
      return theory_feature_category_t::fundamental;
    }
    if (upper == "NOT")
    {
      return theory_feature_category_t::not_feature;
    }
    if (upper == "AND")
    {
      return theory_feature_category_t::and_feature;
    }
    if (upper == "OR")
    {
      return theory_feature_category_t::or_feature;
    }
    if (upper == "COMPLEX")
    {
      return theory_feature_category_t::complex;
    }
    error(error_msgs::UNEXPECTED,
          "theory category",
          token.value(),
          token.position_line(),
          token.position_column());
  }

  spl_specification_t::systems_t parser_t::parse_available_systems()
  {
    spl_specification_t::systems_t systems;
    while (true)
    {
      auto token = peek_token();
      if (token.type() == token_type_t::EMPTY)
      {
        break;
      }
      if (token.type() == token_type_t::KEYWORD)
      {
        const auto upper = toupper(token.value());
        if (upper == kw::END)
        {
          next_token();
          break;
        }
        if (is_section_keyword(token))
        {
          break;
        }
      }
      token = next_token();
      if (token.type() != token_type_t::VALUE)
      {
        error(error_msgs::EXPECTED_BUT_GOT, "system id", token.value());
      }
      systems.push_back(std::stoull(token.value()));
    }
    return systems;
  }

  spl_specification_t::systems_t parser_t::parse_missing_systems()
  {
    spl_specification_t::systems_t systems;
    while (true)
    {
      auto token = peek_token();
      if (token.type() == token_type_t::EMPTY)
      {
        break;
      }
      if (token.type() == token_type_t::KEYWORD)
      {
        const auto upper = toupper(token.value());
        if (upper == kw::END)
        {
          next_token();
          break;
        }
        if (is_section_keyword(token))
        {
          break;
        }
      }
      token = next_token();
      if (token.type() != token_type_t::VALUE)
      {
        error(error_msgs::EXPECTED_BUT_GOT, "system id", token.value());
      }
      systems.push_back(std::stoull(token.value()));
    }
    return systems;
  }

  printer_t::printer_t(std::ostream& os): m_os { os } { }

  void printer_t::print_specification(const spl_specification_t& spec)
  {
    print_spl_name(spec);
    print_features(spec);
    print_constraints(spec);
    print_systems(spec);
    print_category_filter(spec);
  }

  void printer_t::print_spl_name(const spl_specification_t& spec)
  {
    if (spec.m_spl_specified)
    {
      m_os << kw::SPL << csv_separator << spec.m_spl_name << "\n" << std::endl;
    }
  }

  void printer_t::print_features(const spl_specification_t& spec)
  {
    if (spec.m_feature_names_specified)
    {
      m_os << kw::FEATURES << csv_separator << spec.m_F << std::endl;
      for (const auto& feature : spec.m_feature_names)
      {
        m_os << feature << std::endl;
      }
      m_os << std::endl;
    }
  }

  void printer_t::print_constraints(const spl_specification_t& spec)
  {
    if (spec.m_constraints_specified)
    {
      m_os << kw::CONSTRAINTS << std::endl;
      for (const auto& constraint : spec.m_constraints)
      {
        m_os << kw::ASSERT << csv_separator << to_string(constraint) << std::endl;
      }
      m_os << kw::END << std::endl;
      m_os << std::endl;
    }
  }

  void printer_t::print_category_filter(const spl_specification_t& spec)
  {
    if (spec.m_category_filter_specified && spec.m_category_filter.has_value())
    {
      m_os << kw::CATEGORY_FILTER;
      for (const auto category : spec.m_category_filter.value())
      {
        m_os << csv_separator << ::feature_isolation::to_string(category);
      }
      m_os << std::endl;
      m_os << std::endl;
    }
  }

  void printer_t::print_systems(const spl_specification_t& spec)
  {
    if (spec.m_available_systems_specified)
    {
      m_os << kw::AVAILABLE_SYSTEMS;
      for (const auto& system : spec.m_available_systems)
      {
        m_os << csv_separator << system;
      }
      m_os << csv_separator << kw::END << std::endl << std::endl;
    }

    if (spec.m_missing_systems_specified)
    {
      m_os << kw::MISSING_SYSTEMS;
      for (const auto& system : spec.m_missing_systems)
      {
        m_os << csv_separator << system;
      }
      m_os << csv_separator << kw::END << std::endl << std::endl;
    }
  }

  void printer_t::print_theory_dsl_coverage(const spl_specification_t& spec)
  {
    const auto coverage = theory_dsl_coverage(spec);

    m_os << "SUPPORTED" << std::endl;
    for (const auto& capability : coverage.supported_capabilities)
    {
      m_os << "- " << capability << std::endl;
    }

    m_os << std::endl << "NOT DIRECTLY COVERED" << std::endl;
    for (const auto& capability : coverage.unsupported_capabilities)
    {
      m_os << "- " << capability << std::endl;
    }
    m_os << std::endl;
  }

  spl_processor_t::spl_processor_t(const std::string& filename)
      : m_filename(filename)
      , m_spec(process())
  { }

  spl_specification_t spl_processor_t::process()
  {
    std::ifstream input_file(m_filename);
    if (!input_file.is_open())
    {
      error(error_msgs::FAILED_TO, "open", "input", m_filename);
    }

    parser_t parser(input_file, m_filename);

    return parser.m_spec;
  }

  void spl_processor_t::print(const std::string& output_filename) const
  {
    std::ofstream output_file(output_filename);
    if (!output_file.is_open())
    {
      error(error_msgs::FAILED_TO, "open", "output", output_filename);
    }

    printer_t printer(output_file);
    printer.print_specification(m_spec);
  }

  const spl_specification_t& spl_processor_t::get_specification() const
  {
    return m_spec;
  }

} // namespace feature_isolation
