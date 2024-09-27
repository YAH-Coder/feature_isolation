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

  char csv_separator { '\t' };

  namespace error_msgs {
    const std::string FEATURE_NOT_FOUND = "Feature {} not found at line {}, column {}";
    const std::string FEATUREINDEX_NOT_FOUND = "Feature {} is not a specified feature name.";
    const std::string SYSTEM_CONSTRAINT = "System {} violates {} constraint for feature {} and dependent feature {}";
    const std::string UNEXPECTED = "Unexpected {}: {} at line {}, column {}";
    const std::string SPECIFIED_MORE_THAN_ONCE = "{} specified more than once in file {}";
    const std::string EXPECTED_BUT_GOT = "Expected {} but got: {}";
    const std::string ERROR_READING = "Error occurred when reading {} in file {}";
    const std::string DUPLICATE_FOUND = "Duplicate {} found in file {}: {} at line {}, column {}";
    const std::string FAILED_TO = "Failed to {} {}: {}";
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
    static const std::unordered_map<std::string_view, token_type_t> keywords = {
        { COMMENT, token_type_t::COMMENT },
        { SPL, token_type_t::KEYWORD },
        { FEATURES, token_type_t::KEYWORD },
        { ROOT, token_type_t::NAME },
        { CATEGORIES, token_type_t::KEYWORD },
        { F, token_type_t::KEYWORD },
        { A, token_type_t::KEYWORD },
        { O, token_type_t::KEYWORD },
        { N, token_type_t::KEYWORD },
        { AN, token_type_t::KEYWORD },
        { ON, token_type_t::KEYWORD },
        { ANF, token_type_t::KEYWORD },
        { ONF, token_type_t::KEYWORD },
        { END, token_type_t::KEYWORD },
        { EXPLICIT_DEPENDENCIES, token_type_t::KEYWORD },
        { REQUIRES_ALL, token_type_t::KEYWORD },
        { REQUIRES_ANY, token_type_t::KEYWORD },
        { REQUIRES_ONE, token_type_t::KEYWORD },
        { EXCLUDES_ALL, token_type_t::KEYWORD },
        { MISSING_SYSTEMS, token_type_t::KEYWORD },
        { AVAILABLE_SYSTEMS, token_type_t::KEYWORD },
        { MAX_RANK, token_type_t::KEYWORD } };
  } // namespace kw

  static const std::unordered_map<std::string, unsigned_integer> category_map = {
      { std::string( kw::F ), F }, { std::string( kw::A ), A }, { std::string( kw::O ), O }, { std::string( kw::N ), N }, { std::string( kw::AN ), AN }, { std::string( kw::ON ), ON }, { std::string( kw::ANF ), ANF }, { std::string( kw::ONF ), ONF } };

  [[nodiscard]] std::string toupper( std::string s );

  template <typename... Args>
  bool error( const std::string_view& msg, std::ostream& output = std::cerr, Args&&... args );

  using position_t = std::pair<std::size_t, std::size_t>;

  class token_t final {
  public:
    token_t( const token_type_t& tt = token_type_t::EMPTY, const std::string& v = "", const position_t& b = { 0, 0 }, const position_t& e = { 0, 0 } )
        : m_token_type { tt }, m_value { v }, m_begin { b }, m_end { e } {};

    [[nodiscard]] const token_type_t& type() const;
    [[nodiscard]] const std::string value() const;
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
    tokenizer_t( std::istream& is )
        : m_is { is }, m_position { 1, 0 } {}
    token_t next();
    [[nodiscard]] std::optional<token_t> check_keyword( const std::string& token );

  private:
    std::istream& m_is;
    position_t m_position;
  };

  class parser_t final {
  public:
    parser_t( std::istream& is, std::string filename )
        : m_tokenizer { is }, m_spec { parse_specification( filename ) } {}
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
    printer_t( std::ostream& os )
        : m_os { os } {}
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
  bool error( const std::string_view& msg, std::ostream& output, Args&&... args ) {
    output << fmt::format( fmt::runtime( msg ), std::forward<Args>( args )... ) << std::endl;
    return false;
  }

  std::string toupper( std::string s ) {
    std::transform( s.begin(), s.end(), s.begin(), ::toupper );
    return s;
  }

  z3::expr exactly( unsigned n, z3::expr_vector const& xs ) {
    z3::context& ctx = xs[0].ctx();
    z3::expr_vector k( ctx );
    for ( unsigned i = 0; i < xs.size(); i++ ) {
      k.push_back( z3::ite( xs[i], z3::expr( ctx, Z3_mk_true( ctx ) ), z3::expr( ctx, Z3_mk_false( ctx ) ) ) );
    }
    return z3::sum( k ) == ctx.int_val( n );
  }
  spl_specification_t::spl_specification_t( const std::string& filename,
                                            const std::optional<std::string>& spl_name,
                                            std::optional<unsigned_integer> max_rank,
                                            unsigned_integer F,
                                            unsigned_integer max_available_systems,
                                            const feature_names_t& feature_names,
                                            const dependencies_t& requires_all,
                                            const dependencies_t& requires_any,
                                            const dependencies_t& requires_one,
                                            const dependencies_t& excludes_all,
                                            const spl_specification_t::systems_t& available_systems,
                                            const spl_specification_t::systems_t& missing_systems,
                                            bool spl_specified,
                                            bool feature_names_specified,
                                            bool categories_specified,
                                            bool explicit_dependencies_specified,
                                            bool missing_systems_specified,
                                            bool available_systems_specified,
                                            bool max_rank_specified,
                                            unsigned_integer categories )
      : m_filename( filename ),
        m_spl_name( spl_name.value_or( "NOT SPECIFIED" ) ),
        m_max_rank( max_rank.value_or( F ) ), // If max_rank is not specified, use F
        m_F( F ),
        m_max_available_systems( max_available_systems ),
        m_feature_names( feature_names ),
        m_categories( categories ),
        m_requires_all( requires_all ),
        m_requires_any( requires_any ),
        m_requires_one( requires_one ),
        m_excludes_all( excludes_all ),
        m_available_systems( available_systems ),
        m_missing_systems( missing_systems ),
        m_spl_specified( spl_specified ),
        m_feature_names_specified( feature_names_specified ),
        m_categories_specified( categories_specified ),
        m_explicit_dependencies_specified( explicit_dependencies_specified ),
        m_missing_systems_specified( missing_systems_specified ),
        m_available_systems_specified( available_systems_specified ),
        m_max_rank_specified( max_rank_specified ) {

    // Validate systems during object creation
    if ( !validate_systems() ) {
      throw std::runtime_error( "Failed to validate systems during SPL specification construction." );
    }
  }

  bool spl_specification_t::check_constraints( const std::string& constraint_type, const dependencies_t& constraints_map, unsigned_integer system_id, unsigned_integer system, z3::context& ctx ) const {
    for ( const auto& [feature, deps] : constraints_map ) {
      z3::solver solver( ctx );

      if ( feature == -1 ) {
        z3::expr_vector dep_exprs( ctx );
        for ( const auto& dep : deps ) {
          z3::expr dep_expr = ctx.bool_val( system & ( 1 << dep ) );
          dep_exprs.push_back( dep_expr );
        }

        if ( constraint_type == kw::REQUIRES_ONE ) {
          solver.add( exactly( 1, dep_exprs ) );
        } else {
          for ( const auto& dep_expr : dep_exprs ) {
            solver.add( dep_expr );
          }
        }

        if ( solver.check() != z3::sat ) {
          return error( error_msgs::SYSTEM_CONSTRAINT, std::cerr, std::to_string( system_id ), constraint_type, std::string( kw::ROOT ), m_feature_names[*deps.begin()] );
        }
      } else {
        z3::expr feature_expr = ctx.bool_val( system & ( 1 << feature ) );
        z3::expr_vector dep_exprs( ctx );
        for ( const auto& dep : deps ) {
          z3::expr dep_expr = ctx.bool_val( system & ( 1 << dep ) );
          dep_exprs.push_back( dep_expr );
        }

        if ( constraint_type == kw::REQUIRES_ALL ) {
          for ( const auto& dep_expr : dep_exprs ) {
            solver.add( implies( feature_expr, dep_expr ) );
          }
        } else if ( constraint_type == kw::REQUIRES_ANY ) {
          solver.add( implies( feature_expr, z3::mk_or( dep_exprs ) ) );
        } else if ( constraint_type == kw::REQUIRES_ONE ) {
          solver.add( implies( feature_expr, exactly( 1, dep_exprs ) ) );
        } else if ( constraint_type == kw::EXCLUDES_ALL ) {
          for ( const auto& dep_expr : dep_exprs ) {
            solver.add( implies( feature_expr, !dep_expr ) );
          }
        }

        if ( solver.check() != z3::sat ) {
          for ( const auto& dep : deps ) {
            z3::solver detail_solver( ctx );
            z3::expr dep_expr = ctx.bool_val( system & ( 1 << dep ) );
            if ( constraint_type == kw::REQUIRES_ALL ) {
              detail_solver.add( implies( feature_expr, dep_expr ) );
            } else if ( constraint_type == kw::REQUIRES_ANY ) {
              detail_solver.add( implies( feature_expr, dep_expr ) );
            } else if ( constraint_type == kw::REQUIRES_ONE ) {
              detail_solver.add( implies( feature_expr, exactly( 1, dep_exprs ) ) );
            } else if ( constraint_type == kw::EXCLUDES_ALL ) {
              detail_solver.add( implies( feature_expr, !dep_expr ) );
            }
            if ( detail_solver.check() != z3::sat ) {
              return error( error_msgs::SYSTEM_CONSTRAINT, std::cerr, std::to_string( system_id ), constraint_type, std::string( kw::ROOT ), m_feature_names[*deps.begin()] );
            }
          }
        }
      }
    }
    return true;
  }

  bool spl_specification_t::validate_systems() const {
    z3::context ctx;

    for ( const auto& system_id : m_available_systems ) {
      unsigned_integer system = system_id - 1;

      if ( !check_constraints( std::string( kw::REQUIRES_ALL ), m_requires_all, system_id, system, ctx ) )
        return false;
      if ( !check_constraints( std::string( kw::REQUIRES_ANY ), m_requires_any, system_id, system, ctx ) )
        return false;
      if ( !check_constraints( std::string( kw::REQUIRES_ONE ), m_requires_one, system_id, system, ctx ) )
        return false;
      if ( !check_constraints( std::string( kw::EXCLUDES_ALL ), m_excludes_all, system_id, system, ctx ) )
        return false;
    }
    return true;
  }

  [[nodiscard]] const token_type_t& token_t::type() const {
    return m_token_type;
  }

  [[nodiscard]] const std::string token_t::value() const {
    if ( m_token_type == token_type_t::EMPTY ) {
      throw std::runtime_error( "token_t::value() MUST not be called if m_token_type == token_type_t::EMPTY." );
    }
    return m_value;
  }

  [[nodiscard]] const position_t& token_t::begin() const {
    if ( m_token_type == token_type_t::EMPTY ) {
      throw std::runtime_error( "token_t::begin() MUST not be called if m_token_type == token_type_t::EMPTY." );
    }
    return m_begin;
  }

  [[nodiscard]] const position_t& token_t::end() const {
    if ( m_token_type == token_type_t::EMPTY ) {
      throw std::runtime_error( "token_t::end() MUST not be called if m_token_type == token_type_t::EMPTY." );
    }
    return m_end;
  }

  [[nodiscard]] const std::string token_t::position_line() const {
    return ( std::to_string( m_begin.first ) );
  }

  [[nodiscard]] const std::string token_t::position_column() const {
    return ( std::to_string( m_begin.second ) );
  }
  std::string tokenTypeToString( token_type_t type ) { // DEBUG
    switch ( type ) {
    case token_type_t::EMPTY:
      return "EMPTY";
    case token_type_t::UNKNOWN:
      return "UNKNOWN";
    case token_type_t::COMMENT:
      return "COMMENT";
    case token_type_t::KEYWORD:
      return "KEYWORD";
    case token_type_t::VALUE:
      return "VALUE";
    case token_type_t::NAME:
      return "NAME";
    default:
      return "UNKNOWN";
    }
  }
  void debug_token( const token_t& token ) { // DEBUG
    if ( token.type() != token_type_t::EMPTY ) {
      std::cerr << "Type: " << tokenTypeToString( token.type() )
                << ", Token: " << token.value()
                << ", Start: (" << token.begin().first << ", " << token.begin().second << ")"
                << ", End: (" << token.end().first << ", " << token.end().second << ")"
                << std::endl;
    } else {
      std::cerr << "Type: " << tokenTypeToString( token.type() )
                << std::endl;
    }
  }

  token_t tokenizer_t::next() {
    std::string token;
    char c;
    position_t token_begin;
    position_t token_end;
    position_t current_position;

    while ( m_is.get( c ) ) {
      current_position = m_position;

      if ( c == '\n' ) {
        m_position.first++;    // Increment line number
        m_position.second = 1; // Reset column number
      } else {
        m_position.second++; // Increment column number
      }

      if ( std::isalnum( c ) || c == '_' || c == '#' || c == '-' || c == '.' ) {
        if ( token.empty() ) {
          token_begin = current_position;
        }
        token += c;
        token_end = current_position;
      } else {
        if ( !token.empty() ) {
          if ( std::all_of( token.begin(), token.end(), ::isdigit ) ) {
            token_t result( token_type_t::VALUE, token, token_begin, token_end );
            debug_token( result );
            return result;
          }

          auto keyword_check = check_keyword( toupper( token ) );
          if ( keyword_check.has_value() ) {
            token_t result( keyword_check->type(), token, token_begin, token_end );
            debug_token( result );
            return result;
          }

          token_t result( token_type_t::NAME, token, token_begin, token_end );
          debug_token( result );
          return result;
        }

        if ( std::isspace( c ) ) {
          // Skip whitespace
          continue;
        } else if ( c == '/' && m_is.peek() == '/' ) {
          // Handle comment
          // Consume the second '/'
          m_is.get( c );
          m_position.second++; 

          // Read until end of line, updating positions
          while ( m_is.get( c ) ) {
            if ( c == '\n' ) {
              m_position.first++;
              m_position.second = 1;
              break;
            } else {
              m_position.second++;
            }
          }
          continue;
        } 
      }
    }

    // Handle any remaining token at EOF
    if ( !token.empty() ) {
      // token_end already set to last character position

      if ( std::all_of( token.begin(), token.end(), ::isdigit ) ) {
        token_t result( token_type_t::VALUE, token, token_begin, token_end );
        debug_token( result );
        return result;
      }

      auto keyword_check = check_keyword( toupper( token ) );
      if ( keyword_check.has_value() ) {
        token_t result( keyword_check->type(), token, token_begin, token_end );
        debug_token( result );
        return result;
      }

      token_t result( token_type_t::NAME, token, token_begin, token_end );
      debug_token( result );
      return result;
    }

    // Return empty token at EOF
    token_t result( token_type_t::EMPTY, "", m_position, m_position );
    debug_token( result );
    return result;
  }

  std::optional<token_t> tokenizer_t::check_keyword( const std::string& token ) {
    using namespace kw;
    std::string upper_token = toupper( token );

    auto it = keywords.find( upper_token );
    if ( it != keywords.end() ) {
      return token_t( it->second, token, m_position, m_position );
    }
    return std::nullopt;
  }

  spl_specification_t parser_t::parse_specification( const std::string& filename ) {
    std::optional<std::string> spl_name;
    std::optional<unsigned_integer> max_rank;
    unsigned_integer max_available_systems = 0;
    unsigned_integer F = 3;
    feature_names_t feature_names;
    unsigned_integer categories = feature_isolation::F; // Default value
    dependency_maps_t dependencies;                     // Temporary struct to hold dependency maps
    spl_specification_t::systems_t available_systems;
    spl_specification_t::systems_t missing_systems;
    bool spl_specified = false, feature_names_specified = false, categories_specified = false,
         explicit_dependencies_specified = false, missing_systems_specified = false,
         available_systems_specified = false, max_rank_specified = false;

    while ( true ) {
      auto token = m_tokenizer.next();
      if ( token.type() == token_type_t::EMPTY ) {
        break;
      }
      if ( token.type() == token_type_t::COMMENT ) {
        continue;
      }
      if ( token.type() == token_type_t::KEYWORD ) {
        if ( token.value() == kw::SPL ) {
          spl_name = parse_spl();
          spl_specified = true;
        } else if ( token.value() == kw::MAX_RANK ) {
          max_rank = parse_max_rank();
          max_rank_specified = true;
        } else if ( token.value() == kw::FEATURES ) {
          auto [parsed_F, parsed_features] = parse_features();
          F = parsed_F;
          feature_names = parsed_features;
          feature_names_specified = true;
        } else if ( token.value() == kw::CATEGORIES ) {
          categories = parse_categories();
          categories_specified = true;
        } else if ( token.value() == kw::EXPLICIT_DEPENDENCIES ) {
          dependencies = parse_explicit_dependencies( feature_names );
          explicit_dependencies_specified = true;
        } else if ( token.value() == kw::AVAILABLE_SYSTEMS ) {
          available_systems = parse_available_systems();
          available_systems_specified = true;
        } else if ( token.value() == kw::MISSING_SYSTEMS ) {
          missing_systems = parse_missing_systems();
          missing_systems_specified = true;
        } else {
          error( error_msgs::UNEXPECTED, std::cerr, "keyword", token.value(), token.position_line(), token.position_column() );
        }
      } else {
        error( error_msgs::UNEXPECTED, std::cerr, "", token.value(), token.position_line(), token.position_column() );
      }
    }

    max_available_systems = ( 1 << F ); // Calculate max_available_systems based on F

    spl_specification_t::systems_t calculated_available_systems = effective_available_systems(
        max_available_systems,
        available_systems,
        missing_systems,
        available_systems_specified,
        missing_systems_specified );

    return spl_specification_t(
        filename,
        spl_name,
        max_rank,
        F,
        max_available_systems,
        feature_names,
        dependencies.requires_all,
        dependencies.requires_any,
        dependencies.requires_one,
        dependencies.excludes_all,
        calculated_available_systems, // Pass calculated available systems
        missing_systems,
        spl_specified,
        feature_names_specified,
        categories_specified,
        explicit_dependencies_specified,
        missing_systems_specified,
        available_systems_specified,
        max_rank_specified,
        categories // Pass categories
    );
  }
  spl_specification_t::systems_t parser_t::effective_available_systems( unsigned_integer max_available_systems,
                                                                        const spl_specification_t::systems_t& available_systems,
                                                                        const spl_specification_t::systems_t& missing_systems,
                                                                        bool available_systems_specified,
                                                                        bool missing_systems_specified ) const {
    if ( available_systems_specified ) {
      return available_systems;
    } else if ( missing_systems_specified ) {
      spl_specification_t::systems_t eff_available_systems;
      for ( unsigned_integer i = 1; i <= max_available_systems; ++i ) {
        if ( std::find( missing_systems.begin(), missing_systems.end(), i ) == missing_systems.end() ) {
          eff_available_systems.push_back( i );
        }
      }
      return eff_available_systems;
    } else {
      spl_specification_t::systems_t eff_available_systems;
      for ( unsigned_integer i = 1; i <= max_available_systems; ++i ) {
        eff_available_systems.push_back( i );
      }
      return eff_available_systems;
    }
  }

  std::optional<signed int> parser_t::feature_index( const std::string& feature, const feature_names_t& feature_names ) const {
    if ( toupper( feature ) == kw::ROOT ) {
      return -1;
    }

    auto it = std::find( feature_names.begin(), feature_names.end(), feature );
    if ( it == feature_names.end() ) {
      error( error_msgs::FEATUREINDEX_NOT_FOUND, std::cerr, feature );
      return std::nullopt;
    }
    return std::distance( feature_names.begin(), it );
  }

  std::optional<std::string> parser_t::parse_spl() {
    auto token = m_tokenizer.next();
    return token.value();
  }

  std::optional<unsigned_integer> parser_t::parse_max_rank() {
    auto token = m_tokenizer.next();
    if ( token.type() != token_type_t::VALUE ) {
      error( error_msgs::EXPECTED_BUT_GOT, std::cerr, std::string( kw::MAX_RANK ), "value", token.value() );
      return std::nullopt;
    }
    return std::stoull( token.value() );
  }

  std::pair<unsigned_integer, feature_names_t> parser_t::parse_features() {
    auto token = m_tokenizer.next();
    unsigned_integer f { std::stoull( token.value() ) };
    feature_names_t feature_names( f );
    for ( auto& feature : feature_names ) {
      token = m_tokenizer.next();
      feature = token.value();
    }
    return { f, feature_names };
  }

  unsigned_integer parser_t::parse_categories() {
    std::unordered_set<std::string> category_set;
    unsigned_integer categories = 0; // Temporary variable to accumulate categories
    token_t token;

    do {
      token = m_tokenizer.next();
      if ( token.type() != token_type_t::KEYWORD ) {
        error( error_msgs::UNEXPECTED, std::cerr, "category", token.value(), token.position_line(), token.position_column() );
        continue;
      }

      std::string token_value = toupper( token.value() );
      if ( token_value == kw::END ) {
        break;
      }

      auto it = category_map.find( token_value );
      if ( it != category_map.end() ) {
        if ( category_set.find( token_value ) != category_set.end() ) {
          error( error_msgs::DUPLICATE_FOUND, std::cerr, "category", token.value(), token.position_line(), token.position_column() );
          continue;
        }
        category_set.insert( token_value );
        categories |= it->second; // Use temporary variable to accumulate categories
      } else {
        error( error_msgs::UNEXPECTED, std::cerr, "category", token.value(), token.position_line(), token.position_column() );
      }
    } while ( token.value() != kw::END );

    return categories; // Return the accumulated categories
  }

  parser_t::dependency_maps_t parser_t::parse_explicit_dependencies( const feature_names_t& feature_names ) {
    dependency_maps_t dependencies;

    while ( true ) {
      auto token = m_tokenizer.next();
      if ( token.type() == token_type_t::KEYWORD && toupper( token.value() ) == kw::END ) {
        break;
      }

      // Parse feature name
      std::string feature_name = token.value();
      auto feature_index_opt = feature_index( feature_name, feature_names );
      if ( !feature_index_opt.has_value() ) {
        error( error_msgs::FEATURE_NOT_FOUND, std::cerr, feature_name, token.position_line(), token.position_column() );
        continue;
      }

      signed int feature_index = feature_index_opt.value();

      // Parse dependency type (REQUIRES_ALL, REQUIRES_ANY, etc.)
      token = m_tokenizer.next();
      if ( token.type() != token_type_t::KEYWORD ) {
        error( error_msgs::EXPECTED_BUT_GOT, std::cerr, "a dependency keyword", token.value(), token.position_line(), token.position_column() );
        continue;
      }

      std::string dependency_type = toupper( token.value() );

      // Parse the dependencies
      std::set<unsigned_integer> dependent_features;
      while ( true ) {
        token = m_tokenizer.next();

        if ( token.type() == token_type_t::KEYWORD && toupper( token.value() ) == kw::END ) {
          break;
        }

        if ( token.type() == token_type_t::NAME || token.type() == token_type_t::VALUE ) {
          auto index_opt = this->feature_index( token.value(), feature_names );
          if ( !index_opt.has_value() ) {
            error( error_msgs::FEATURE_NOT_FOUND, std::cerr, token.value(), token.position_line(), token.position_column() );
            continue;
          }
          dependent_features.insert( static_cast<unsigned_integer>( index_opt.value() ) );
        } else {
          error( error_msgs::UNEXPECTED, std::cerr, "token", token.value(), token.position_line(), token.position_column() );
          continue;
        }
      }

      // Add dependencies to the appropriate map in the temporary struct
      if ( dependency_type == kw::REQUIRES_ALL ) {
        dependencies.requires_all[feature_index] = dependent_features;
      } else if ( dependency_type == kw::REQUIRES_ANY ) {
        dependencies.requires_any[feature_index] = dependent_features;
      } else if ( dependency_type == kw::REQUIRES_ONE ) {
        dependencies.requires_one[feature_index] = dependent_features;
      } else if ( dependency_type == kw::EXCLUDES_ALL ) {
        dependencies.excludes_all[feature_index] = dependent_features;
      } else {
        error( error_msgs::UNEXPECTED, std::cerr, "dependency type", dependency_type, token.position_line(), token.position_column() );
      }
    }

    return dependencies;
  }

  spl_specification_t::systems_t parser_t::parse_available_systems() {
    spl_specification_t::systems_t systems;
    while ( true ) {
      auto token = m_tokenizer.next();
      if ( token.type() == token_type_t::EMPTY ) {
        break;
      }
      std::cerr << token.value() << std::endl;
      if ( token.type() != token_type_t::VALUE ) {
        error( error_msgs::EXPECTED_BUT_GOT, std::cerr, "system id", token.value() );
        continue;
      }
      systems.push_back( std::stoull( token.value() ) );
    }
    return systems;
  }

  spl_specification_t::systems_t parser_t::parse_missing_systems() {
    spl_specification_t::systems_t systems;
    while ( true ) {
      auto token = m_tokenizer.next();
      if ( token.type() == token_type_t::EMPTY ) {
        break;
      }
      if ( token.type() != token_type_t::VALUE ) {
        error( error_msgs::EXPECTED_BUT_GOT, std::cerr, "system id", token.value() );
        continue;
      }
      systems.push_back( std::stoull( token.value() ) );
    }
    return systems;
  }

  void printer_t::print_specification( const spl_specification_t& spec ) {
    print_spl_name( spec );
    print_max_rank( spec );
    print_features( spec );
    print_categories( spec );
    print_dependencies( spec );
    print_systems( spec );
  }

  void printer_t::print_spl_name( const spl_specification_t& spec ) {
    if ( spec.m_spl_specified ) {
      m_os << kw::SPL << csv_separator << spec.m_spl_name << "\n"
           << std::endl;
    }
  }

  void printer_t::print_max_rank( const spl_specification_t& spec ) {
    if ( spec.m_max_rank_specified ) {
      m_os << kw::MAX_RANK << csv_separator << spec.m_max_rank << "\n"
           << std::endl;
    }
  }

  void printer_t::print_features( const spl_specification_t& spec ) {
    if ( spec.m_feature_names_specified ) {
      m_os << kw::FEATURES << csv_separator << spec.m_F << std::endl;
      for ( const auto& feature : spec.m_feature_names ) {
        m_os << feature << std::endl;
      }
      m_os << std::endl;
    }
  }

  void printer_t::print_categories( const spl_specification_t& spec ) {
    if ( spec.m_categories_specified ) {
      m_os << kw::CATEGORIES << std::endl;
      for ( const auto& [name, bit] : category_map ) {
        if ( spec.m_categories & bit ) {
          m_os << name << std::endl;
        }
      }
      m_os << kw::END << std::endl;
      m_os << std::endl;
    }
  }

  void printer_t::print_dependencies( const spl_specification_t& spec ) {
    if ( spec.m_explicit_dependencies_specified ) {
      m_os << kw::EXPLICIT_DEPENDENCIES << std::endl;

      auto print_dependency_group = [&]( const std::string& type, const dependencies_t& group ) {
        for ( const auto& [feature, dependencies] : group ) {
          if ( feature == -1 ) {
            m_os << kw::ROOT;
          } else {
            m_os << spec.m_feature_names[static_cast<unsigned_integer>( feature )];
          }
          m_os << csv_separator << type;
          for ( const auto& dependency : dependencies ) {
            m_os << csv_separator << spec.m_feature_names[dependency];
          }
          m_os << csv_separator << kw::END << std::endl;
        }
      };

      print_dependency_group( "REQUIRES_ALL", spec.m_requires_all );
      print_dependency_group( "REQUIRES_ANY", spec.m_requires_any );
      print_dependency_group( "REQUIRES_ONE", spec.m_requires_one );
      print_dependency_group( "EXCLUDES_ALL", spec.m_excludes_all );

      m_os << kw::END << "\n"
           << std::endl;
    }
  }

  void printer_t::print_systems( const spl_specification_t& spec ) {
    if ( spec.m_available_systems_specified ) {
      m_os << kw::AVAILABLE_SYSTEMS;
      for ( const auto& system : spec.m_available_systems ) {
        m_os << "\n"
             << system;
      }
      m_os << std::endl;
    } else if ( spec.m_missing_systems_specified ) {
      m_os << kw::MISSING_SYSTEMS;
      for ( const auto& system : spec.m_missing_systems ) {
        m_os << "\n"
             << system;
      }
      m_os << "\n"
           << std::endl;
    }
  }

  spl_processor_t::spl_processor_t( const std::string& filename )
      : m_filename( filename ), m_spec( process() ) {}

  spl_specification_t spl_processor_t::process() {
    std::ifstream input_file( m_filename );
    if ( !input_file.is_open() ) {
      error( error_msgs::FAILED_TO, std::cerr, "open", "input", m_filename );
      throw std::logic_error( "Failed to open input file" );
    }

    parser_t parser( input_file, m_filename );

    return parser.m_spec;
  }

  void spl_processor_t::print( const std::string& output_filename ) const {
    std::ofstream output_file( output_filename );
    if ( !output_file.is_open() ) {
      error( error_msgs::FAILED_TO, std::cerr, "open", "output", output_filename );
      return;
    }

    printer_t printer( output_file );
    printer.print_specification( m_spec );
  }

  const spl_specification_t& spl_processor_t::get_specification() const {
    return m_spec;
  }

} // namespace feature_isolation

#endif // SPL_HPP
