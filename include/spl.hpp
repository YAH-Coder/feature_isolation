/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#ifndef SPL_HPP
#define SPL_HPP

#include "feature_analysis.hpp"
#include <exception>
#include <stdexcept>

#define FMT_HEADER_ONLY

#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <z3++.h>

namespace feature_isolation {
  using feature_names_t = std::vector<std::string>;
  using dependencies_t = std::unordered_map<signed int, std::set<unsigned_integer>>;

  extern char csv_separator;

  namespace error_msgs {
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

  class spl_specification_t final {
  public:
    using systems_t = std::vector<unsigned_integer>;
    const std::string m_filename;
    const std::string m_spl_name;
    const unsigned_integer m_max_rank;
    const unsigned_integer m_F;
    const unsigned_integer m_max_available_systems;
    const feature_names_t m_feature_names;
    const unsigned_integer m_categories;
    const dependencies_t m_requires_all, m_requires_any, m_requires_one, m_excludes_all;
    const systems_t m_available_systems;
    const systems_t m_missing_systems;
    const bool m_spl_specified, m_feature_names_specified, m_categories_specified,
        m_explicit_dependencies_specified, m_missing_systems_specified,
        m_available_systems_specified, m_max_rank_specified;

    spl_specification_t( const std::string& filename,
                         const std::optional<std::string>& spl_name,
                         std::optional<unsigned_integer> max_rank,
                         unsigned_integer F,
                         unsigned_integer max_available_systems,
                         const feature_names_t& feature_names,
                         const dependencies_t& requires_all,
                         const dependencies_t& requires_any,
                         const dependencies_t& requires_one,
                         const dependencies_t& excludes_all,
                         const systems_t& available_systems,
                         const systems_t& missing_systems,
                         bool spl_specified,
                         bool feature_names_specified,
                         bool categories_specified,
                         bool explicit_dependencies_specified,
                         bool missing_systems_specified,
                         bool available_systems_specified,
                         bool max_rank_specified,
                         unsigned_integer categories = feature_isolation::F );

  private:
    bool validate_systems() const;
    bool check_constraints( const std::string& constraint_type,
                            const dependencies_t& constraints_map,
                            unsigned_integer system_id,
                            unsigned_integer system,
                            z3::context& ctx ) const;
  };

  enum class token_type_t {
    EMPTY,
    UNKNOWN,
    COMMENT,
    KEYWORD,
    VALUE,
    NAME
  };

  namespace kw {
    constexpr std::string_view COMMENT { "//" }, SPL { "SPL" }, MAX_RANK { "MAX_RANK" }, FEATURES { "FEATURES" }, ROOT { "ROOT" },
        CATEGORIES { "CATEGORIES" }, F { "F" }, N { "N" }, O { "O" }, A { "A" }, AN { "AN" }, ON { "ON" },
        ANF { "ANF" }, ONF { "ONF" }, END { "END" }, EXPLICIT_DEPENDENCIES { "EXPLICIT_DEPENDENCIES" },
        REQUIRES_ALL { "REQUIRES_ALL" }, REQUIRES_ANY { "REQUIRES_ANY" }, REQUIRES_ONE { "REQUIRES_ONE" },
        EXCLUDES_ALL { "EXCLUDES_ALL" }, MISSING_SYSTEMS { "MISSING_SYSTEMS" },
        AVAILABLE_SYSTEMS { "AVAILABLE_SYSTEMS" };
    extern const std::unordered_map<std::string_view, token_type_t> keywords;
  } // namespace kw

  extern const std::unordered_map<std::string, unsigned_integer> category_map;

  [[nodiscard]] std::string toupper( std::string s );

  template <typename... Args>
  [[noreturn]] void error( const std::string_view& msg, Args&&... args );

  using position_t = std::pair<std::size_t, std::size_t>;

  class token_t final {
  public:
    token_t( const token_type_t& tt = token_type_t::EMPTY, const std::string& v = "", const position_t& b = { 0, 0 }, const position_t& e = { 0, 0 } );

    [[nodiscard]] const token_type_t& type() const;
    [[nodiscard]] const std::string& value() const;
    [[nodiscard]] const position_t& begin() const;
    [[nodiscard]] const position_t& end() const;
    [[nodiscard]] const std::string position_line() const;
    [[nodiscard]] const std::string position_column() const;

  private:
    token_type_t m_token_type;
    std::string m_value;
    position_t m_begin, m_end;
  };

  class tokenizer_t final {
  public:
    tokenizer_t( std::istream& is );
    token_t next();
    [[nodiscard]] std::optional<token_t> check_keyword( const std::string& token );

  private:
    std::istream& m_is;
    position_t m_position;

    bool is_token_char(char c) const;
    bool is_whitespace(char c) const;
    void consume_char(char& c);
    void skip_line();
    token_t parse_token();
  };

  class parser_t final {
  public:
    parser_t( std::istream& is, std::string filename );
    tokenizer_t m_tokenizer;
    spl_specification_t m_spec;

  private:
    std::optional<std::string> parse_spl();
    std::optional<unsigned_integer> parse_max_rank();
    std::pair<unsigned_integer, feature_names_t> parse_features();
    unsigned_integer parse_categories();
    struct dependency_maps_t {
      dependencies_t requires_all;
      dependencies_t requires_any;
      dependencies_t requires_one;
      dependencies_t excludes_all;
    };
    dependency_maps_t parse_explicit_dependencies( const feature_names_t& feature_names );
    spl_specification_t::systems_t parse_available_systems();
    spl_specification_t::systems_t parse_missing_systems();
    std::optional<signed int> feature_index( const std::string& feature, const feature_names_t& feature_names ) const;
    spl_specification_t::systems_t effective_available_systems( unsigned_integer max_available_systems,
                                                                const spl_specification_t::systems_t& available_systems,
                                                                const spl_specification_t::systems_t& missing_systems,
                                                                bool available_systems_specified,
                                                                bool missing_systems_specified ) const;

    spl_specification_t parse_specification( const std::string& filename );
  };

  class printer_t final {
  public:
    printer_t( std::ostream& os );
    void print_specification( const spl_specification_t& spec );
    void print_spl_name( const spl_specification_t& spec );
    void print_max_rank( const spl_specification_t& spec );
    void print_features( const spl_specification_t& spec );
    void print_categories( const spl_specification_t& spec );
    void print_dependencies( const spl_specification_t& spec );
    void print_systems( const spl_specification_t& spec );

  private:
    std::ostream& m_os;
  };

  class spl_processor_t final {
  private:
    std::string m_filename;
    spl_specification_t m_spec;

  public:
    spl_processor_t( const std::string& filename );
    spl_specification_t process();
    void print( const std::string& output_filename ) const;
    const spl_specification_t& get_specification() const;
  };

  template <typename... Args>
  [[noreturn]] void error( const std::string_view& msg, Args&&... args );

  std::string toupper( std::string s );

  z3::expr exactly( unsigned n, z3::expr_vector const& xs );

} // namespace feature_isolation

#endif // SPL_HPP
