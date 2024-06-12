#ifndef SPL_HPP
#define SPL_HPP

#include "feature_analysis.hpp"

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
    const std::string FEATURE_NAME_NOT_FOUND = "Feature name not found: {}",
                      ERROR_ROOT_CONSTRAINT = "System {} violates {} {} constraint for dependent feature {}",
                      ERROR_SYSTEM_CONSTRAINT = "System {} violates {} constraint for feature {} and dependent feature {}",
                      UNEXPECTED_KEYWORD = "Unexpected keyword: {} at {}",
                      UNEXPECTED_TOKEN = "Unexpected token: {} at {}",
                      FEATURES_NOT_SPECIFIED = "Features not specified in file {}",
                      SPL_SPECIFIED_MORE_THAN_ONCE = "SPL specified more than once in file {}",
                      EXPECTED_SPL_NAME = "Expected SPL name, but got: {}",
                      MAX_RANK_SPECIFIED_MORE_THAN_ONCE = "MAX_RANK specified more than once in file {}",
                      EXPECTED_MAX_RANK_VALUE = "Expected MAX_RANK value, but got: {}",
                      FEATURES_SPECIFIED_MORE_THAN_ONCE = "Features specified more than once in file {}",
                      ERROR_READING_NUM_FEATURES = "Error occurred when reading number of independent features in file {}",
                      ERROR_READING_FEATURE_NAME = "Error occurred when reading name of independent feature in file {}",
                      DUPLICATE_FEATURE_FOUND = "Duplicate feature found in file {}: {} at {}",
                      EXPLICIT_DEPENDENCIES_SPECIFIED_MORE_THAN_ONCE = "Explicit dependencies specified more than once in file {}",
                      EXPECTED_FEATURE_NAME_OR_ROOT = "Expected a feature name or ROOT but received: {} at {}",
                      FEATURE_INDEX_NOT_FOUND = "Feature index not found for: {}",
                      EXPECTED_DEPENDENCY_KEYWORD = "Expected a dependency keyword but received: {} at {}",
                      UNEXPECTED_CATEGORY = "Unexpected category: {} at {}",
                      DUPLICATE_CATEGORY = "Duplicate category: {} at {}",
                      CATEGORIES_SPECIFIED_MORE_THAN_ONCE = "Feature categories specified more than once in file {}",
                      AVAILABLE_SYSTEMS_SPECIFIED_MORE_THAN_ONCE = "Available systems specified more than once in file {}",
                      ONLY_ONE_OF_AVAILABLE_OR_MISSING_SYSTEMS = "Only one of AVAILABLE_SYSTEMS or MISSING_SYSTEMS can be specified in file {}",
                      SYSTEM_EXCEEDS_AVAILABLE = "System {} exceeds the number of available systems: {}",
                      UNEXPECTED_TOKEN_IN_AVAILABLE_SYSTEMS = "Unexpected token in AVAILABLE_SYSTEMS section: {}",
                      MISSING_SYSTEMS_SPECIFIED_MORE_THAN_ONCE = "Missing systems specified more than once in file {}",
                      UNEXPECTED_TOKEN_IN_MISSING_SYSTEMS = "Unexpected token in MISSING_SYSTEMS section: {}",
                      FAILED_TO_OPEN_INPUT_FILE = "Failed to open input file: {}",
                      FAILED_TO_PARSE_SPECIFICATION = "Failed to parse SPL specification in file: {}",
                      INVALID_SYSTEM_CONFIGURATION = "Invalid system configuration in file {}",
                      FAILED_TO_OPEN_OUTPUT_FILE = "Failed to open output file: {}";
  } // namespace error_msgs

  class spl_specification_t final {
    using systems_t = std::vector<unsigned_integer>;

  private:
    std::string m_filename {};
    std::string m_spl_name;
    unsigned_integer m_max_rank;
    unsigned_integer m_F;
    unsigned_integer m_max_available_systems;
    feature_names_t m_feature_names;
    unsigned_integer m_categories { F };
    dependencies_t m_requires_all, m_requires_any, m_requires_one, m_excludes_all;
    systems_t m_available_systems, m_missing_systems;

    [[nodiscard]] std::optional<signed int> feature_index( const std::string& feature ) const;
    [[nodiscard]] systems_t effective_available_systems() const;
    bool validate_systems();
    bool check_constraints( const std::string& constraint_type, const dependencies_t& constraints_map, unsigned_integer system_id, unsigned_integer system, z3::context& ctx );

    bool m_spl_specified { false }, m_feature_names_specified { false }, m_categories_specified { false },
        m_explicit_dependencies_specified { false }, m_missing_systems_specified { false },
        m_available_systems_specified { false }, m_max_rank_specified { false };

    friend class Parser;
    friend class Printer;
    friend class spl_processor;
  };

  enum class TokenType {
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
    static const std::unordered_map<std::string_view, TokenType> keywords = {
        { COMMENT, TokenType::COMMENT },
        { SPL, TokenType::KEYWORD },
        { FEATURES, TokenType::KEYWORD },
        { ROOT, TokenType::NAME },
        { CATEGORIES, TokenType::KEYWORD },
        { F, TokenType::KEYWORD },
        { A, TokenType::KEYWORD },
        { O, TokenType::KEYWORD },
        { N, TokenType::KEYWORD },
        { AN, TokenType::KEYWORD },
        { ON, TokenType::KEYWORD },
        { ANF, TokenType::KEYWORD },
        { ONF, TokenType::KEYWORD },
        { END, TokenType::KEYWORD },
        { EXPLICIT_DEPENDENCIES, TokenType::KEYWORD },
        { REQUIRES_ALL, TokenType::KEYWORD },
        { REQUIRES_ANY, TokenType::KEYWORD },
        { REQUIRES_ONE, TokenType::KEYWORD },
        { EXCLUDES_ALL, TokenType::KEYWORD },
        { MISSING_SYSTEMS, TokenType::KEYWORD },
        { AVAILABLE_SYSTEMS, TokenType::KEYWORD },
        { MAX_RANK, TokenType::KEYWORD } };
  } // namespace kw

  static const std::unordered_map<std::string, unsigned_integer> category_map = {
      { std::string( kw::F ), F }, { std::string( kw::A ), A }, { std::string( kw::O ), O }, { std::string( kw::N ), N }, { std::string( kw::AN ), AN }, { std::string( kw::ON ), ON }, { std::string( kw::ANF ), ANF }, { std::string( kw::ONF ), ONF } };

  [[nodiscard]] std::string toupper( std::string s );
  bool error( const std::string_view& msg, std::ostream& output = std::cerr );

  using Position = std::pair<std::size_t, std::size_t>;

  class Token final {
  public:
    Token( const TokenType& tt = TokenType::EMPTY, const std::string& v = "", const Position& b = { 0, 0 },
           const Position& e = { 0, 0 } )
        : m_token_type { tt }, m_value { v }, m_begin { b }, m_end { e } {};

    [[nodiscard]] const TokenType& type() const;
    [[nodiscard]] const std::string value() const;
    [[nodiscard]] const Position& begin() const;
    [[nodiscard]] const Position& end() const;
    [[nodiscard]] const std::string position_string() const;

  private:
    TokenType m_token_type;
    std::string m_value;
    Position m_begin, m_end;
  };

  class Tokenizer final {
  public:
    Tokenizer( std::istream& is )
        : m_is { is }, m_position { 1, 0 } {}
    Token next();
    [[nodiscard]] std::optional<Token> check_keyword( const std::string& token );

  private:
    std::istream& m_is;
    Position m_position;
  };

  class Parser final {
  public:
    Parser( std::istream& is )
        : m_tokenizer { is } {}
    bool parse_specification( spl_specification_t& spec );
    bool parse_spl( spl_specification_t& spec );
    bool parse_max_rank( spl_specification_t& spec );
    bool parse_features( spl_specification_t& spec );
    bool parse_categories( spl_specification_t& spec );
    bool parse_explicit_dependencies( spl_specification_t& spec );
    bool parse_available_systems( spl_specification_t& spec );
    bool parse_missing_systems( spl_specification_t& spec );

  private:
    Tokenizer m_tokenizer;
  };

  class Printer final {
  public:
    Printer( std::ostream& os )
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

  class spl_processor final {
  public:
    spl_processor( const std::string& filename );
    bool process();
    void print( const std::string& output_filename ) const;
    const spl_specification_t& get_specification() const;

  private:
    std::string m_filename;
    spl_specification_t m_spec;
  };

  bool error( const std::string_view& msg, std::ostream& os, const std::string& arg1 = "", const std::string& arg2 = "", const std::string& arg3 = "", const std::string& arg4 = "" ) {
    os << fmt::format( fmt::runtime( msg ), arg1, arg2, arg3, arg4 ) << std::endl;
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

  std::optional<signed int> spl_specification_t::feature_index( const std::string& feature_name ) const {
    if ( toupper( feature_name ) == kw::ROOT ) {
      return -1;
    }

    auto it = std::find( m_feature_names.begin(), m_feature_names.end(), feature_name );
    if ( it == m_feature_names.end() ) {
      error( error_msgs::FEATURE_NAME_NOT_FOUND, std::cerr, feature_name );
    }
    return std::distance( m_feature_names.begin(), it );
  }

  spl_specification_t::systems_t spl_specification_t::effective_available_systems() const {
    if ( m_available_systems_specified ) {
      return m_available_systems;
    } else if ( m_missing_systems_specified ) {
      systems_t eff_available_systems;
      for ( unsigned_integer i = 0; i < m_max_available_systems; ++i ) {
        if ( std::find( m_missing_systems.begin(), m_missing_systems.end(), i ) == m_missing_systems.end() ) {
          eff_available_systems.push_back( i );
        }
      }
      return eff_available_systems;
    } else {
      systems_t eff_available_systems;
      for ( unsigned_integer i = 0; i < m_max_available_systems; ++i ) {
        eff_available_systems.push_back( i );
      }
      return eff_available_systems;
    }
  }

  bool spl_specification_t::check_constraints( const std::string& constraint_type, const dependencies_t& constraints_map, unsigned_integer system_id, unsigned_integer system, z3::context& ctx ) {
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
          return error( error_msgs::ERROR_ROOT_CONSTRAINT, std::cerr, std::to_string( system_id ), std::string( kw::ROOT ), constraint_type, m_feature_names[*deps.begin()] );
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
              return error( error_msgs::ERROR_SYSTEM_CONSTRAINT, std::cerr, std::string( constraint_type ), std::to_string( system_id ), m_feature_names[feature], m_feature_names[dep] );
            }
          }
        }
      }
    }
    return true;
  }

  bool spl_specification_t::validate_systems() {
    systems_t eff_available_systems = effective_available_systems();
    z3::context ctx;

    for ( const auto& system_id : eff_available_systems ) {
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

    m_available_systems = eff_available_systems;
    return true;
  }

  [[nodiscard]] const TokenType& Token::type() const {
    return m_token_type;
  }

  [[nodiscard]] const std::string Token::value() const {
    if ( m_token_type == TokenType::EMPTY ) {
      throw std::runtime_error( "Token::value() MUST not be called if m_token_type == TokenType::EMPTY." );
    }
    return m_value;
  }

  [[nodiscard]] const Position& Token::begin() const {
    if ( m_token_type == TokenType::EMPTY ) {
      throw std::runtime_error( "Token::begin() MUST not be called if m_token_type == TokenType::EMPTY." );
    }
    return m_begin;
  }

  [[nodiscard]] const Position& Token::end() const {
    if ( m_token_type == TokenType::EMPTY ) {
      throw std::runtime_error( "Token::end() MUST not be called if m_token_type == TokenType::EMPTY." );
    }
    return m_end;
  }

  [[nodiscard]] const std::string Token::position_string() const {
    return ( "line: " + std::to_string( m_begin.first ) + " ,column: " + std::to_string( m_begin.second ) );
  }

  Token Tokenizer::next() {
    std::string token;
    char c;
    Position token_begin = m_position;

    while ( m_is.get( c ) ) {
      if ( c == '\n' ) {
        m_position.first++;
        m_position.second = 0;
      } else {
        m_position.second++;
      }

      if ( std::isalnum( c ) || c == '_' ) {
        if ( token.empty() ) {
          token_begin = m_position;
          token_begin.second -= 1;
        }
        token += c;
      } else {
        if ( !token.empty() ) {
          m_is.unget();
          m_position.second--;

          Position token_end = m_position;

          if ( std::all_of( token.begin(), token.end(), ::isdigit ) ) {
            Token result( TokenType::VALUE, token, token_begin, token_end );
            return result;
          }

          auto keyword_check = check_keyword( toupper( token ) );
          if ( keyword_check.has_value() ) {
            Token result = keyword_check.value();
            return result;
          }

          Token result( TokenType::NAME, token, token_begin, token_end );
          return result;
        }

        if ( std::isspace( c ) ) {
          continue;
        }

        if ( c == '/' && m_is.peek() == '/' ) {
          while ( m_is.get( c ) && c != '\n' ) {
          }
          if ( c == '\n' ) {
            m_position.first++;
            m_position.second = 0;
          }
          continue;
        }

        token.clear();
      }
    }

    if ( !token.empty() ) {
      Position token_end = m_position;

      if ( std::all_of( token.begin(), token.end(), ::isdigit ) ) {
        Token result( TokenType::VALUE, token, token_begin, token_end );
        return result;
      }

      auto keyword_check = check_keyword( toupper( token ) );
      if ( keyword_check.has_value() ) {
        Token result = keyword_check.value();
        return result;
      }

      Token result( TokenType::NAME, token, token_begin, token_end );
      return result;
    }

    return Token( TokenType::EMPTY, "", m_position, m_position );
  }

  std::optional<Token> Tokenizer::check_keyword( const std::string& token ) {
    using namespace kw;
    std::string upper_token = toupper( token );

    auto it = keywords.find( upper_token );
    if ( it != keywords.end() ) {
      return Token( it->second, token, m_position, m_position );
    }
    return std::nullopt;
  }

  bool Parser::parse_specification( spl_specification_t& spec ) {
    bool result = true;
    while ( true ) {
      auto token = m_tokenizer.next();
      if ( token.type() == TokenType::EMPTY ) {
        break;
      }
      if ( token.type() == TokenType::COMMENT ) {
        continue;
      }
      if ( token.type() == TokenType::KEYWORD ) {
        if ( token.value() == kw::SPL ) {
          result &= parse_spl( spec );
        } else if ( token.value() == kw::MAX_RANK ) {
          result &= parse_max_rank( spec );
        } else if ( token.value() == kw::FEATURES ) {
          result &= parse_features( spec );
        } else if ( token.value() == kw::CATEGORIES ) {
          result &= parse_categories( spec );
        } else if ( token.value() == kw::EXPLICIT_DEPENDENCIES ) {
          result &= parse_explicit_dependencies( spec );
        } else if ( token.value() == kw::AVAILABLE_SYSTEMS ) {
          result &= parse_available_systems( spec );
        } else if ( token.value() == kw::MISSING_SYSTEMS ) {
          result &= parse_missing_systems( spec );
        } else {
          return error( error_msgs::UNEXPECTED_KEYWORD, std::cerr, token.value(), token.position_string() );
        }
      } else {
        return error( error_msgs::UNEXPECTED_TOKEN, std::cerr, token.value(), token.position_string() );
      }
    }

    if ( !spec.m_spl_specified ) {
      spec.m_spl_name = "NOT SPECIFIED";
    }
    if ( !spec.m_max_rank_specified ) {
      spec.m_max_rank = spec.m_F;
    }
    if ( !spec.m_feature_names_specified ) {
      return error( error_msgs::FEATURES_NOT_SPECIFIED, std::cerr, spec.m_filename );
    }

    return result;
  }

  bool Parser::parse_spl( spl_specification_t& spec ) {
    if ( spec.m_spl_specified ) {
      return error( error_msgs::SPL_SPECIFIED_MORE_THAN_ONCE, std::cerr, spec.m_filename );
    }
    auto token = m_tokenizer.next();
    if ( token.type() != TokenType::NAME ) {
      return error( error_msgs::EXPECTED_SPL_NAME, std::cerr, token.value() );
    }
    spec.m_spl_name = token.value();
    spec.m_spl_specified = true;
    return true;
  }

  bool Parser::parse_max_rank( spl_specification_t& spec ) {
    if ( spec.m_max_rank_specified ) {
      return error( error_msgs::MAX_RANK_SPECIFIED_MORE_THAN_ONCE, std::cerr, spec.m_filename );
    }
    auto token = m_tokenizer.next();
    if ( token.type() != TokenType::VALUE ) {
      return error( error_msgs::EXPECTED_MAX_RANK_VALUE, std::cerr, token.value() );
    }
    spec.m_max_rank = std::stoull( token.value() );
    spec.m_max_rank_specified = true;
    return true;
  }

  bool Parser::parse_features( spl_specification_t& spec ) {
    if ( spec.m_feature_names_specified ) {
      return error( error_msgs::FEATURES_SPECIFIED_MORE_THAN_ONCE, std::cerr, spec.m_filename );
    }
    auto token = m_tokenizer.next();
    if ( token.type() != TokenType::VALUE ) {
      return error( error_msgs::ERROR_READING_NUM_FEATURES, std::cerr, spec.m_filename );
    }
    unsigned_integer f { std::stoull( token.value() ) };
    spec.m_F = f;
    spec.m_max_available_systems = ( 1 << f );
    spec.m_feature_names = feature_names_t( f );
    std::unordered_set<std::string> feature_set;

    bool parsing_feature_names = true;

    for ( auto& feature : spec.m_feature_names ) {
      token = m_tokenizer.next();
      if ( parsing_feature_names && token.type() == TokenType::KEYWORD && token.value() == "END" ) {
        parsing_feature_names = false;
      }
      if ( !parsing_feature_names && token.type() != TokenType::NAME ) {
        return error( error_msgs::ERROR_READING_FEATURE_NAME, std::cerr, spec.m_filename );
      }
      if ( feature_set.find( token.value() ) != feature_set.end() ) {
        return error( error_msgs::DUPLICATE_FEATURE_FOUND, std::cerr, spec.m_filename, token.value(), token.position_string() );
      }
      feature_set.insert( token.value() );
      feature = token.value();
    }
    spec.m_feature_names_specified = true;
    return true;
  }

  bool Parser::parse_explicit_dependencies( spl_specification_t& spec ) {
    if ( spec.m_explicit_dependencies_specified ) {
      return error( error_msgs::EXPLICIT_DEPENDENCIES_SPECIFIED_MORE_THAN_ONCE, std::cerr, spec.m_filename );
    }

    Token token;
    while ( true ) {
      token = m_tokenizer.next();
      if ( token.type() == TokenType::KEYWORD && toupper( token.value() ) == kw::END ) {
        break;
      }

      if ( token.type() != TokenType::NAME && toupper( token.value() ) != kw::ROOT ) {
        return error( error_msgs::EXPECTED_FEATURE_NAME_OR_ROOT, std::cerr, token.value(), token.position_string() );
      }

      std::string feature_name = token.value();
      std::string feature_name_upper = toupper( feature_name );
      std::optional<signed int> feature_index_opt;
      if ( feature_name_upper != kw::ROOT ) {
        feature_index_opt = spec.feature_index( feature_name );
        if ( !feature_index_opt.has_value() ) {
          return error( error_msgs::FEATURE_INDEX_NOT_FOUND, std::cerr, feature_name );
        }
      } else {
        feature_index_opt = -1; // Special index for ROOT
      }

      token = m_tokenizer.next();
      if ( token.type() != TokenType::KEYWORD ) {
        return error( error_msgs::EXPECTED_DEPENDENCY_KEYWORD, std::cerr, token.value(), token.position_string() );
      }

      std::string dependency_type = toupper( token.value() );
      std::unordered_set<unsigned_integer> dependent_features;

      while ( true ) {
        token = m_tokenizer.next();

        if ( token.type() == TokenType::KEYWORD && toupper( token.value() ) == kw::END ) {
          break;
        }

        if ( token.type() == TokenType::NAME ) {
          auto index_opt = spec.feature_index( token.value() );
          if ( !index_opt.has_value() ) {
            return error( error_msgs::FEATURE_INDEX_NOT_FOUND, std::cerr, token.value(), token.position_string() );
          }
          dependent_features.insert( static_cast<unsigned_integer>( index_opt.value() ) );
        } else if ( token.type() == TokenType::COMMENT ) {
          while ( m_tokenizer.next().type() != TokenType::KEYWORD ) {
            // Skip until the next keyword or end of dependencies section
          }
          break;
        } else {
          return error( error_msgs::UNEXPECTED_TOKEN, std::cerr, token.value(), token.position_string() );
        }
      }

      if ( dependency_type == kw::REQUIRES_ALL ) {
        spec.m_requires_all[feature_index_opt.value()].insert( dependent_features.begin(), dependent_features.end() );
      } else if ( dependency_type == kw::REQUIRES_ANY ) {
        spec.m_requires_any[feature_index_opt.value()].insert( dependent_features.begin(), dependent_features.end() );
      } else if ( dependency_type == kw::REQUIRES_ONE ) {
        spec.m_requires_one[feature_index_opt.value()].insert( dependent_features.begin(), dependent_features.end() );
      } else if ( dependency_type == kw::EXCLUDES_ALL ) {
        spec.m_excludes_all[feature_index_opt.value()].insert( dependent_features.begin(), dependent_features.end() );
      }
    }

    spec.m_explicit_dependencies_specified = true;
    return true;
  }

  bool Parser::parse_categories( spl_specification_t& spec ) {
    if ( spec.m_categories_specified ) {
      return error( error_msgs::CATEGORIES_SPECIFIED_MORE_THAN_ONCE, std::cerr, spec.m_filename );
    }

    std::unordered_set<std::string> category_set;
    Token token;
    do {
      token = m_tokenizer.next();
      if ( token.type() != TokenType::KEYWORD ) {
        return error( error_msgs::UNEXPECTED_CATEGORY, std::cerr, token.value(), token.position_string() );
      }

      std::string token_value = toupper( token.value() );
      if ( token_value == kw::END ) {
        break;
      }

      auto it = category_map.find( token_value );
      if ( it != category_map.end() ) {
        if ( category_set.find( token_value ) != category_set.end() ) {
          return error( error_msgs::DUPLICATE_CATEGORY, std::cerr, token.value(), token.position_string() );
        }
        category_set.insert( token_value );
        spec.m_categories |= it->second;
      } else {
        return error( error_msgs::UNEXPECTED_CATEGORY, std::cerr, token.value(), token.position_string() );
      }
    } while ( token.value() != kw::END );

    spec.m_categories_specified = true;
    return true;
  }

  bool Parser::parse_available_systems( spl_specification_t& spec ) {
    if ( spec.m_available_systems_specified ) {
      return error( error_msgs::AVAILABLE_SYSTEMS_SPECIFIED_MORE_THAN_ONCE, std::cerr, spec.m_filename );
    }
    if ( spec.m_missing_systems_specified ) {
      return error( error_msgs::ONLY_ONE_OF_AVAILABLE_OR_MISSING_SYSTEMS, std::cerr, spec.m_filename );
    }
    Token token = m_tokenizer.next();
    while ( token.type() != TokenType::EMPTY ) {
      if ( token.type() == TokenType::VALUE ) {
        if ( std::stoull( token.value() ) > spec.m_max_available_systems ) {
          return error( error_msgs::SYSTEM_EXCEEDS_AVAILABLE, std::cerr, token.value(), std::to_string( spec.m_max_available_systems ) );
        }
        spec.m_available_systems.push_back( std::stoull( token.value() ) );
      } else if ( token.type() == TokenType::KEYWORD && toupper( token.value() ) == kw::END ) {
        break;
      } else {
        return error( error_msgs::UNEXPECTED_TOKEN_IN_AVAILABLE_SYSTEMS, std::cerr, token.value() );
      }
      token = m_tokenizer.next();
    }
    spec.m_available_systems_specified = true;
    return true;
  }

  bool Parser::parse_missing_systems( spl_specification_t& spec ) {
    if ( spec.m_missing_systems_specified ) {
      return error( error_msgs::MISSING_SYSTEMS_SPECIFIED_MORE_THAN_ONCE, std::cerr, spec.m_filename );
    }
    if ( spec.m_available_systems_specified ) {
      return error( error_msgs::ONLY_ONE_OF_AVAILABLE_OR_MISSING_SYSTEMS, std::cerr, spec.m_filename );
    }
    Token token = m_tokenizer.next();
    while ( token.type() != TokenType::EMPTY ) {
      if ( token.type() == TokenType::VALUE ) {
        if ( std::stoull( token.value() ) > spec.m_max_available_systems ) {
          return error( error_msgs::SYSTEM_EXCEEDS_AVAILABLE, std::cerr, token.value(), std::to_string( spec.m_max_available_systems ) );
        }
        spec.m_missing_systems.push_back( std::stoull( token.value() ) );
      } else if ( token.type() == TokenType::KEYWORD && toupper( token.value() ) == kw::END ) {
        break;
      } else {
        return error( error_msgs::UNEXPECTED_TOKEN_IN_MISSING_SYSTEMS, std::cerr, token.value() );
      }
      token = m_tokenizer.next();
    }
    spec.m_missing_systems_specified = true;
    return true;
  }

  void Printer::print_specification( const spl_specification_t& spec ) {
    print_spl_name( spec );
    print_max_rank( spec );
    print_features( spec );
    print_categories( spec );
    print_dependencies( spec );
    print_systems( spec );
  }

  void Printer::print_spl_name( const spl_specification_t& spec ) {
    if ( spec.m_spl_specified ) {
      m_os << kw::SPL << csv_separator << spec.m_spl_name << "\n"
           << std::endl;
    }
  }

  void Printer::print_max_rank( const spl_specification_t& spec ) {
    if ( spec.m_max_rank_specified ) {
      m_os << kw::MAX_RANK << csv_separator << spec.m_max_rank << "\n"
           << std::endl;
    }
  }

  void Printer::print_features( const spl_specification_t& spec ) {
    if ( spec.m_feature_names_specified ) {
      m_os << kw::FEATURES << csv_separator << spec.m_F << std::endl;
      for ( const auto& feature : spec.m_feature_names ) {
        m_os << feature << std::endl;
      }
      m_os << std::endl;
    }
  }

  void Printer::print_categories( const spl_specification_t& spec ) {
    if ( spec.m_categories_specified ) {
      m_os << kw::CATEGORIES << std::endl;
      for ( const auto& [name, bit] : category_map ) {
        if ( spec.m_categories & bit ) {
          m_os << name << std::endl;
        }
      }
      m_os << kw::END << std::endl;
    }
  }

  void Printer::print_dependencies( const spl_specification_t& spec ) {
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
          m_os << csv_separator
               << kw::END << std::endl;
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

  void Printer::print_systems( const spl_specification_t& spec ) {
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

  spl_processor::spl_processor( const std::string& filename )
      : m_filename( filename ) {};

  bool spl_processor::process() {
    std::ifstream input_file( m_filename );
    if ( !input_file.is_open() ) {
      error( error_msgs::FAILED_TO_OPEN_INPUT_FILE, std::cerr, m_filename );
      return false;
    }

    feature_isolation::Parser parser( input_file );
    if ( !parser.parse_specification( m_spec ) ) {
      error( error_msgs::FAILED_TO_PARSE_SPECIFICATION, std::cerr, m_filename );
      return false;
    }
    if ( !m_spec.validate_systems() ) {
      return error( error_msgs::INVALID_SYSTEM_CONFIGURATION, std::cerr, m_spec.m_filename );
    }

    return true;
  }

  void spl_processor::print( const std::string& output_filename ) const {
    std::ofstream output_file( output_filename );
    if ( !output_file.is_open() ) {
      error( error_msgs::FAILED_TO_OPEN_OUTPUT_FILE, std::cerr, output_filename );
      return;
    }

    feature_isolation::Printer printer( output_file );
    printer.print_specification( m_spec );
  }

  const spl_specification_t& spl_processor::get_specification() const {
    return m_spec;
  }

} // namespace feature_isolation

#endif // SPL_HPP
