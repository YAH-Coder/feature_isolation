/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
/**
 * @file spl.hpp
 * @brief SPL DSL parsing, serialization, validation, and system-space derivation.
 */
#ifndef SPL_HPP
#define SPL_HPP

#include "feature_analysis.hpp"
#include <exception>
#include <stdexcept>

#include <fstream>
#include <memory>
#include <iostream>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace feature_isolation
{
  /** @brief Ordered list of declared fundamental feature names. */
  using feature_names_t = std::vector<std::string>;

  /** @brief Global separator used by CSV-style printers. */
  extern char csv_separator;


  /** @brief Discriminator for the concrete formula variant stored in @ref theory_formula_t. */
  enum class theory_formula_kind_t
  {
    feature_ref,
    true_literal,
    false_literal,
    negation,
    conjunction,
    disjunction,
    implication,
    equivalence,
    count_compare,
    count_interval
  };

  /** @brief Comparison operators supported by COUNT expressions. */
  enum class theory_count_compare_op_t
  {
    eq,
    ne,
    lt,
    le,
    gt,
    ge
  };

  /** @brief Payload for a COUNT(...) comparison formula. */
  struct theory_count_compare_t
  {
      feature_names_t                 features {};
      std::vector<unsigned_integer>   compiled_feature_indices {};
      theory_count_compare_op_t       op { theory_count_compare_op_t::eq };
      unsigned_integer                value { zero };
  };

  /** @brief Payload for a COUNT(...) interval formula. */
  struct theory_count_interval_t
  {
      feature_names_t               features {};
      std::vector<unsigned_integer> compiled_feature_indices {};
      unsigned_integer              lower { zero };
      unsigned_integer              upper { zero };
  };

  /** @brief Immutable formula node type used by parsed DSL constraints. */
  class theory_formula_t;
  /** @brief Shared pointer to an immutable formula node. */
  using theory_formula_ptr_t = std::shared_ptr<const theory_formula_t>;

  /** @brief Tagged immutable formula node representing a parsed DSL expression. */
  class theory_formula_t final
  {
    public:
      struct feature_ref_t final
      {
          std::string feature_name {};
          std::optional<unsigned_integer> compiled_feature_index {};
      };

      struct true_literal_t final
      { };

      struct false_literal_t final
      { };

      struct negation_t final
      {
          theory_formula_ptr_t operand {};
      };

      struct conjunction_t final
      {
          std::vector<theory_formula_ptr_t> operands {};
      };

      struct disjunction_t final
      {
          std::vector<theory_formula_ptr_t> operands {};
      };

      struct implication_t final
      {
          theory_formula_ptr_t lhs {};
          theory_formula_ptr_t rhs {};
      };

      struct equivalence_t final
      {
          theory_formula_ptr_t lhs {};
          theory_formula_ptr_t rhs {};
      };

      /** @brief Variant storing the concrete payload for the formula node. */
      using payload_t = std::variant<feature_ref_t,
                                     true_literal_t,
                                     false_literal_t,
                                     negation_t,
                                     conjunction_t,
                                     disjunction_t,
                                     implication_t,
                                     equivalence_t,
                                     theory_count_compare_t,
                                     theory_count_interval_t>;

      /** @brief Construct a named feature-reference formula. */
      static theory_formula_t feature(const std::string&               name,
                                      std::optional<unsigned_integer> compiled_feature_index = std::nullopt);
      /** @brief Construct the TRUE literal formula. */
      static theory_formula_t true_literal();
      /** @brief Construct the FALSE literal formula. */
      static theory_formula_t false_literal();
      /** @brief Construct a negation formula. */
      static theory_formula_t negation(const theory_formula_t& operand);
      /** @brief Construct a conjunction formula. */
      static theory_formula_t conjunction(const std::vector<theory_formula_t>& operands);
      /** @brief Construct a disjunction formula. */
      static theory_formula_t disjunction(const std::vector<theory_formula_t>& operands);
      /** @brief Construct an implication formula. */
      static theory_formula_t implication(const theory_formula_t& lhs,
                                          const theory_formula_t& rhs);
      /** @brief Construct an equivalence formula. */
      static theory_formula_t equivalence(const theory_formula_t& lhs,
                                          const theory_formula_t& rhs);
      /** @brief Construct a COUNT comparison formula. */
      static theory_formula_t count_compare_formula(
          const feature_names_t&               features,
          const std::vector<unsigned_integer>& compiled_feature_indices,
          theory_count_compare_op_t            op,
          unsigned_integer                     value);
      /** @brief Construct a COUNT interval formula. */
      static theory_formula_t count_interval_formula(
          const feature_names_t&               features,
          const std::vector<unsigned_integer>& compiled_feature_indices,
          unsigned_integer                     lower,
          unsigned_integer                     upper);

      /** @brief Return the stored payload variant. */
      [[nodiscard]] const payload_t& payload() const;
      /** @brief Return the concrete kind of the stored payload. */
      [[nodiscard]] theory_formula_kind_t kind() const;

    private:
      explicit theory_formula_t(payload_t payload);
      payload_t m_payload;
  };

  /** @brief Sequence of parsed DSL constraints. */
  using theory_constraints_t = std::vector<theory_formula_t>;
  /** @brief Sequence of category filters read from the DSL. */
  using theory_category_filter_t = std::vector<theory_feature_category_t>;

  [[nodiscard]] std::string to_string(theory_count_compare_op_t op);
  [[nodiscard]] std::string to_string(const theory_formula_t& formula);
  [[nodiscard]] std::string to_string(const theory_category_filter_t& filter);

  /** @brief Shared parser and processor error-message fragments. */
  namespace error_msgs
  {
    extern const std::string FEATURE_NOT_FOUND;
    extern const std::string FEATUREINDEX_NOT_FOUND;
    extern const std::string SYSTEM_CONSTRAINT;
    extern const std::string UNEXPECTED;
    extern const std::string SPECIFIED_MORE_THAN_ONCE;
    extern const std::string EXPECTED_BUT_GOT;
    extern const std::string ERROR_READING;
    extern const std::string DUPLICATE_FOUND;
    extern const std::string FAILED_TO;
  } // namespace error_msgs

  /** @brief Immutable in-memory representation of a parsed SPL specification. */
  class spl_specification_t final
  {
    public:
      /** @brief Ordered list of one-based system identifiers. */
      using systems_t = std::vector<unsigned_integer>;
      const std::string      m_filename;
      const std::string      m_spl_name;
      const unsigned_integer m_F;
      const unsigned_integer m_max_available_systems;
      const feature_names_t  m_feature_names;
      const theory_constraints_t m_constraints;
      const std::optional<theory_category_filter_t> m_category_filter;
      const systems_t m_available_systems;
      const systems_t m_missing_systems;
      const system_space_t m_system_space;
      const bool      m_spl_specified, m_feature_names_specified,
          m_constraints_specified, m_category_filter_specified,
          m_missing_systems_specified, m_available_systems_specified;

      /** @brief Construct the immutable parsed specification object. */
      spl_specification_t(const std::string&                filename,
                          const std::optional<std::string>& spl_name,
                          unsigned_integer                  F,
                          unsigned_integer                  max_available_systems,
                          const feature_names_t&            feature_names,
                          const theory_constraints_t&       constraints,
                          const std::optional<theory_category_filter_t>& category_filter,
                          const systems_t&                  available_systems,
                          const systems_t&                  missing_systems,
                          bool                              spl_specified,
                          bool                              feature_names_specified,
                          bool                              constraints_specified,
                          bool                              category_filter_specified,
                          bool                              missing_systems_specified,
                          bool                              available_systems_specified);

      /** @brief Return the derived system-space partition. */
      [[nodiscard]] const system_space_t& system_space() const;
    private:
      bool validate_systems() const;
  };

  /** @brief Token classes emitted by the tokenizer. */
  enum class token_type_t
  {
    EMPTY,
    UNKNOWN,
    COMMENT,
    KEYWORD,
    VALUE,
    NAME
  };

  /** @brief DSL keyword and operator string constants. */
  namespace kw
  {
    constexpr std::string_view COMMENT { "//" }, SPL { "SPL" },
        FEATURES { "FEATURES" }, END { "END" },
        MISSING_SYSTEMS { "MISSING_SYSTEMS" },
        AVAILABLE_SYSTEMS { "AVAILABLE_SYSTEMS" },
        CONSTRAINTS { "CONSTRAINTS" }, ASSERT { "ASSERT" },
        CATEGORY_FILTER { "CATEGORY_FILTER" }, TRUE_LITERAL { "TRUE" },
        FALSE_LITERAL { "FALSE" }, COUNT { "COUNT" }, IN { "IN" },
        NOT_OP { "!" }, AND_OP { "&&" }, OR_OP { "||" }, IMPLIES_OP { "->" },
        IFF_OP { "<->" }, LPAREN { "(" }, RPAREN { ")" }, LBRACKET { "[" },
        RBRACKET { "]" }, COMMA { "," }, EQ_OP { "=" }, NE_OP { "!=" },
        LT_OP { "<" }, LE_OP { "<=" }, GT_OP { ">" }, GE_OP { ">=" };
    extern const std::unordered_map<std::string_view, token_type_t> keywords;
  } // namespace kw

  /** @brief Mapping from category names to internal category codes. */
  extern const std::unordered_map<std::string, unsigned_integer> category_map;

  /** @brief Return an upper-case copy of a string. */
  [[nodiscard]] std::string toupper(std::string s);

  template<typename... Args>
  [[noreturn]] void error(const std::string_view& msg, Args&&... args);

  /** @brief Line/column position pair used by tokens and diagnostics. */
  using position_t = std::pair<std::size_t, std::size_t>;

  /** @brief Token emitted by the tokenizer. */
  class token_t final
  {
    public:
      /** @brief Construct a token with type, value, and source positions. */
      token_t(const token_type_t& tt = token_type_t::EMPTY,
              const std::string& v   = "",
              const position_t& b    = { 0, 0 },
              const position_t& e    = { 0, 0 });

      /** @brief Return the token type. */
      [[nodiscard]] const token_type_t& type() const;
      /** @brief Return the token lexeme. */
      [[nodiscard]] const std::string&  value() const;
      [[nodiscard]] const position_t&   begin() const;
      [[nodiscard]] const position_t&   end() const;
      [[nodiscard]] const std::string   position_line() const;
      [[nodiscard]] const std::string   position_column() const;
    private:
      token_type_t m_token_type;
      std::string  m_value;
      position_t   m_begin, m_end;
  };

  /** @brief Line-oriented tokenizer for the SPL DSL. */
  class tokenizer_t final
  {
    public:
      /** @brief Construct a tokenizer over an input stream. */
      tokenizer_t(std::istream& is);
      /** @brief Read the next token from the stream. */
      token_t next();
      [[nodiscard]] std::optional<token_t>
          check_keyword(const std::string& token);
    private:
      std::istream& m_is;
      position_t    m_position;

      bool    is_token_char(char c) const;
      bool    is_whitespace(char c) const;
      void    consume_char(char& c);
      void    skip_line();
      token_t parse_token();
  };

  /** @brief Recursive-descent parser for the SPL DSL. */
  class parser_t final
  {
    private:
      std::optional<token_t> m_buffered_token {};

    public:
      /** @brief Construct a parser over an input stream and filename context. */
      parser_t(std::istream& is, std::string filename);
      tokenizer_t         m_tokenizer;
      spl_specification_t m_spec;
      /** @brief Parse the SPL section. */
      std::optional<std::string>                   parse_spl();
      /** @brief Parse the FEATURES section. */
      std::pair<unsigned_integer, feature_names_t> parse_features();
      /** @brief Parse the CONSTRAINTS section. */
      theory_constraints_t                         parse_constraints(const feature_names_t& feature_names);
      /** @brief Parse the CATEGORY_FILTER section. */
      std::optional<theory_category_filter_t>      parse_category_filter();
      /** @brief Parse the AVAILABLE_SYSTEMS section. */
      spl_specification_t::systems_t parse_available_systems();
      /** @brief Parse the MISSING_SYSTEMS section. */
      spl_specification_t::systems_t parse_missing_systems();
      token_t next_token();
      token_t peek_token();
      bool is_section_keyword(const token_t& token) const;
      /** @brief Parse a formula with full precedence handling. */
      theory_formula_t parse_formula(const feature_names_t& feature_names);
      theory_formula_t parse_equivalence(const feature_names_t& feature_names);
      theory_formula_t parse_implication(const feature_names_t& feature_names);
      theory_formula_t parse_disjunction(const feature_names_t& feature_names);
      theory_formula_t parse_conjunction(const feature_names_t& feature_names);
      theory_formula_t parse_unary(const feature_names_t& feature_names);
      theory_formula_t parse_atom(const feature_names_t& feature_names);
      /** @brief Parse the repository COUNT(...) expression. */
      theory_formula_t parse_count_expression(const feature_names_t& feature_names);
      feature_names_t parse_count_feature_list(const feature_names_t& feature_names);
      theory_feature_category_t parse_theory_category(const token_t& token) const;
      std::optional<signed int>
                                     feature_index(const std::string&     feature,
                                                   const feature_names_t& feature_names) const;
      /** @brief Parse a complete specification from the current token stream. */
      spl_specification_t parse_specification(const std::string& filename);
  };

  /** @brief Summary of which documented DSL capabilities are supported by the current parser/spec. */
  struct theory_dsl_coverage_t
  {
      std::vector<std::string> supported_capabilities;
      std::vector<std::string> unsupported_capabilities;
  };

  [[nodiscard]] theory_dsl_coverage_t
      theory_dsl_coverage(const spl_specification_t& spec);

  /** @brief Human-readable serializer for parsed SPL specifications. */
  class printer_t final
  {
    public:
      printer_t(std::ostream& os);
      void print_specification(const spl_specification_t& spec);
      void print_spl_name(const spl_specification_t& spec);
      void print_features(const spl_specification_t& spec);
      void print_constraints(const spl_specification_t& spec);
      void print_category_filter(const spl_specification_t& spec);
      void print_systems(const spl_specification_t& spec);
      void print_theory_dsl_coverage(const spl_specification_t& spec);
    private:
      std::ostream& m_os;
  };

  /** @brief Convenience façade that parses and optionally prints an SPL specification. */
  class spl_processor_t final
  {
    private:
      std::string         m_filename;
      spl_specification_t m_spec;
    public:
      /** @brief Construct the processor for an input file. */
      spl_processor_t(const std::string& filename);
      /** @brief Parse and return the specification. */
      spl_specification_t process();
      void                print(const std::string& output_filename) const;
      /** @brief Return the cached parsed specification. */
      const spl_specification_t& get_specification() const;
  };

  /** @brief Report whether a one-based system id satisfies the parsed constraints. */
  [[nodiscard]] bool system_satisfies_constraints(
      const spl_specification_t& spec, unsigned_integer system_id);
  /** @brief Enumerate all valid systems for the specification. */
  [[nodiscard]] system_ids_t valid_system_ids(const spl_specification_t& spec);
  /** @brief Enumerate valid systems that are available. */
  [[nodiscard]] system_ids_t available_valid_system_ids(
      const spl_specification_t& spec);
  /** @brief Enumerate valid systems that are missing. */
  [[nodiscard]] system_ids_t missing_valid_system_ids(
      const spl_specification_t& spec);
  /** @brief Derive the valid/available/missing theory system space for the specification. */
  [[nodiscard]] system_space_t theory_system_space(const spl_specification_t& spec);

} // namespace feature_isolation

#endif // SPL_HPP
