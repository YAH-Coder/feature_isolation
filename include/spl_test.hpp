#ifndef SPL_HPP
#define SPL_HPP

#include "feature_analysis.hpp"

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

  char csv_separator;

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

  static const std::unordered_map<std::string, unsigned_integer> category_map = {
      { "F", F }, { "A", A }, { "O", O }, { "N", N }, { "AN", AN }, { "ON", ON }, { "ANF", ANF }, { "ONF", ONF } };

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

  bool error( const std::string_view& msg, std::ostream& os ) {
    os << msg << std::endl;
    return false;
  }

  std::string toupper( std::string s ) {
    std::transform( s.begin(), s.end(), s.begin(), ::toupper );
    return s;
  }

  std::optional<signed int> spl_specification_t::feature_index( const std::string& feature_name ) const {
    if ( toupper( feature_name ) == kw::ROOT ) {
      return -1;
    }

    auto it = std::find( m_feature_names.begin(), m_feature_names.end(), feature_name );
    if ( it == m_feature_names.end() ) {
      error( "Feature name not found: " + feature_name );
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

  bool spl_specification_t::validate_systems() {
    systems_t eff_available_systems { effective_available_systems() };
    z3::context ctx;

    for ( const auto& system_id : eff_available_systems ) {
      unsigned_integer system = system_id - 1;

      for ( const auto& [feature, deps] : m_requires_all ) {
        z3::solver solver( ctx );
        if ( feature == -1 ) {
          for ( const auto& dep : deps ) {
            z3::expr dep_expr = ctx.bool_val( system & ( 1 << dep ) );
            solver.add( dep_expr );
            if ( solver.check() != z3::sat ) {
              return error( "System " + std::to_string( system_id ) + " violates " + static_cast<std::string>( kw::ROOT ) + " REQUIRES_ALL constraint for dependent feature " + m_feature_names[dep] );
            }
          }
        }
        if ( system & ( 1 << feature ) ) {
          for ( const auto& dep : deps ) {
            z3::expr dep_expr = ctx.bool_val( system & ( 1 << dep ) );
            solver.add( dep_expr );
            if ( solver.check() != z3::sat ) {
              return error( "System " + std::to_string( system_id ) + " violates REQUIRES_ALL constraint for feature " + m_feature_names[static_cast<unsigned_integer>( feature )] + " and dependent feature " + m_feature_names[dep] );
            }
          }
        }
      }

      for ( const auto& [feature, deps] : m_requires_any ) {
        z3::solver solver( ctx );
        if ( feature == -1 ) {
          z3::expr_vector exprs( ctx );
          for ( const auto& dep : deps ) {
            exprs.push_back( ctx.bool_val( system & ( 1 << dep ) ) );
          }
          solver.add( z3::mk_or( exprs ) );
          if ( solver.check() != z3::sat ) {
            std::string dep_str;
            for ( const auto& dep : deps ) {
              dep_str += m_feature_names[dep] + " ";
            }
            return error( "System " + std::to_string( system_id ) + " violates " + static_cast<std::string>( kw::ROOT ) + " REQUIRES_ANY constraint for dependent features " + dep_str );
          }
        }
        if ( system & ( 1 << feature ) ) {
          z3::expr_vector exprs( ctx );
          for ( const auto& dep : deps ) {
            exprs.push_back( ctx.bool_val( system & ( 1 << dep ) ) );
          }
          solver.add( z3::mk_or( exprs ) );
          if ( solver.check() != z3::sat ) {
            return error( "System " + std::to_string( system_id ) + " violates REQUIRES_ANY constraint for feature " + m_feature_names[static_cast<unsigned_integer>( feature )] );
          }
        }
      }

      for ( const auto& [feature, deps] : m_requires_one ) {
        z3::solver solver( ctx );
        if ( feature == -1 ) {
          z3::expr_vector exprs( ctx );
          for ( const auto& dep : deps ) {
            exprs.push_back( ctx.bool_val( system & ( 1 << dep ) ) );
          }
          solver.add( z3::mk_or( exprs ) );
          solver.add( z3::mk_xor( exprs ) );
          if ( solver.check() != z3::sat ) {
            std::string dep_str;
            for ( const auto& dep : deps ) {
              dep_str += m_feature_names[dep] + " ";
            }
            return error( "System " + std::to_string( system_id ) + " violates " + static_cast<std::string>( kw::ROOT ) + " REQUIRES_ONE constraint for dependent feature " + dep_str );
          }
        }
        if ( system & ( 1 << feature ) ) {
          z3::expr_vector exprs( ctx );
          for ( const auto& dep : deps ) {
            exprs.push_back( ctx.bool_val( system & ( 1 << dep ) ) );
          }
          solver.add( z3::mk_or( exprs ) );
          solver.add( z3::mk_xor( exprs ) );
          if ( solver.check() != z3::sat ) {
            std::string dep_str;
            for ( const auto& dep : deps ) {
              dep_str += m_feature_names[dep] + " ";
            }
            return error( "System " + std::to_string( system_id ) + " violates REQUIRES_ONE constraint for feature " + m_feature_names[static_cast<unsigned_integer>( feature )] + " and dependent feature " + dep_str );
          }
        }
      }

      for ( const auto& [feature, deps] : m_excludes_all ) {
        z3::solver solver( ctx );
        if ( feature == -1 ) {
          for ( const auto& dep : deps ) {
            z3::expr dep_expr = !ctx.bool_val( system & ( 1 << dep ) );
            solver.add( dep_expr );
            if ( solver.check() != z3::sat ) {
              return error( "System " + std::to_string( system_id ) + " violates " + static_cast<std::string>( kw::ROOT ) + " EXCLUDES_ALL constraint for feature " + m_feature_names[dep] );
            }
          }
        }
        if ( system & ( 1 << feature ) ) {
          for ( const auto& dep : deps ) {
            z3::expr dep_expr = !ctx.bool_val( system & ( 1 << dep ) );
            solver.add( dep_expr );
            if ( solver.check() != z3::sat ) {
              return error( "System " + std::to_string( system_id ) + " violates EXCLUDES_ALL constraint for feature " + m_feature_names[static_cast<unsigned_integer>( feature )] + " and dependent feature " + m_feature_names[dep] );
            }
          }
        }
      }
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
    while ( m_is.get( c ) ) {
      if ( c == '\n' ) {
        m_position.first++;
        m_position.second = 0;
      } else {
        m_position.second++;
      }

      if ( std::isalnum( c ) || c == '_' ) {
        token += c;
      } else {
        if ( !token.empty() ) {
          m_is.unget();
          m_position.second--;

          if ( std::all_of( token.begin(), token.end(), ::isdigit ) ) {
            return Token( TokenType::VALUE, token, m_position,
                          { m_position.first, m_position.second - token.length() } );
          }

          auto keyword_check = check_keyword( toupper( token ) );
          if ( keyword_check.has_value() ) {
            if ( !csv_separator ) {
              m_is.get( c );
              csv_separator = c;
              m_is.unget();
            }
            return keyword_check.value();
          }

          return Token( TokenType::NAME, token, m_position,
                        { m_position.first, m_position.second - token.length() } );
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
      if ( std::all_of( token.begin(), token.end(), ::isdigit ) ) {
        return Token( TokenType::VALUE, token, m_position, { m_position.first, m_position.second - token.length() } );
      }

      auto keyword_check = check_keyword( toupper( token ) );
      if ( keyword_check.has_value() ) {
        return keyword_check.value();
      }

      return Token( TokenType::NAME, token, m_position, { m_position.first, m_position.second - token.length() } );
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
          return error( "Unexpected keyword: " + token.value() + " at " + token.position_string() );
        }
      } else {
        return error( "Unexpected token: " + token.value() + " at " + token.position_string() );
      }
    }

    if ( !spec.m_spl_specified ) {
      spec.m_spl_name = "NOT SPECIFIED";
    }
    if ( !spec.m_max_rank_specified ) {
      spec.m_max_rank = spec.m_F;
    }
    if ( !spec.m_feature_names_specified ) {
      return error( "Features not specified in file " + spec.m_filename );
    }

    return result;
  }

  bool Parser::parse_spl( spl_specification_t& spec ) {
    if ( spec.m_spl_specified ) {
      return error( "SPL specified more than once in file " + spec.m_filename );
    }
    auto token = m_tokenizer.next();
    if ( token.type() != TokenType::NAME ) {
      return error( "Expected SPL name, but got: " + token.value() );
    }
    spec.m_spl_name = token.value();
    spec.m_spl_specified = true;
    return true;
  }

  bool Parser::parse_max_rank( spl_specification_t& spec ) {
    if ( spec.m_max_rank_specified ) {
      return error( "MAX_RANK specified more than once in file " + spec.m_filename );
    }
    auto token = m_tokenizer.next();
    if ( token.type() != TokenType::VALUE ) {
      return error( "Expected MAX_RANK value, but got: " + token.value() );
    }
    spec.m_max_rank = std::stoull( token.value() );
    spec.m_max_rank_specified = true;
    return true;
  }

  bool Parser::parse_features( spl_specification_t& spec ) {
    if ( spec.m_feature_names_specified ) {
      return error( "Features specified more than once in file " + spec.m_filename );
    }
    auto token = m_tokenizer.next();
    if ( token.type() != TokenType::VALUE ) {
      return error( "Error occurred when reading number of independent features in file " + spec.m_filename );
    }
    unsigned_integer f { std::stoull( token.value() ) };
    spec.m_F = f;
    spec.m_max_available_systems = ( 1 << f );
    spec.m_feature_names = feature_names_t( f );
    std::unordered_set<std::string> feature_set;
    for ( auto& feature : spec.m_feature_names ) {
      token = m_tokenizer.next();
      if ( token.type() != TokenType::NAME ) {
        return error( "Error occurred when reading name of independent feature in file " + spec.m_filename );
      }
      if ( feature_set.find( token.value() ) != feature_set.end() ) {
        return error( "Duplicate feature found in file " + spec.m_filename + ": " + token.value() + " at " +
                      token.position_string() );
      }
      feature_set.insert( token.value() );
      feature = token.value();
    }
    spec.m_feature_names_specified = true;
    return true;
  }

  bool Parser::parse_explicit_dependencies( spl_specification_t& spec ) {
    if ( spec.m_explicit_dependencies_specified ) {
      return error( "Explicit dependencies specified more than once in file " + spec.m_filename );
    }

    Token token;
    while ( true ) {
      token = m_tokenizer.next();
      if ( token.type() == TokenType::KEYWORD && toupper( token.value() ) == kw::END ) {
        break;
      }

      if ( token.type() != TokenType::NAME && toupper( token.value() ) != kw::ROOT ) {
        return error( "Expected a feature name or ROOT but received: " + token.value() + " at " + token.position_string() );
      }

      std::string feature_name = token.value();
      std::string feature_name_upper = toupper( feature_name );
      std::optional<signed int> feature_index_opt;
      if ( feature_name_upper != kw::ROOT ) {
        feature_index_opt = spec.feature_index( feature_name );
        if ( !feature_index_opt.has_value() ) {
          return error( "Feature index not found for: " + feature_name );
        }
      } else {
        feature_index_opt = -1; // Special index for ROOT
      }

      token = m_tokenizer.next();
      if ( token.type() != TokenType::KEYWORD ) {
        return error( "Expected a dependency keyword but received: " + token.value() + " at " + token.position_string() );
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
            return error( "Feature index not found for: " + token.value() + " at " + token.position_string() );
          }
          dependent_features.insert( static_cast<unsigned_integer>( index_opt.value() ) );
        } else if ( token.type() == TokenType::COMMENT ) {
          while ( m_tokenizer.next().type() != TokenType::KEYWORD ) {
            // Skip until the next keyword or end of dependencies section
          }
          break;
        } else {
          return error( "Unexpected token: " + token.value() + " at " + token.position_string() );
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
      return error( "Feature categories specified more than once in file " + spec.m_filename );
    }

    std::unordered_set<std::string> category_set;
    Token token;
    do {
      token = m_tokenizer.next();
      if ( token.type() != TokenType::KEYWORD ) {
        return error( "Expected a category but received: " + token.value() + " at " + token.position_string() );
      }

      std::string token_value = toupper( token.value() );
      if ( token_value == kw::END ) {
        break;
      }

      auto it = category_map.find( token_value );
      if ( it != category_map.end() ) {
        if ( category_set.find( token_value ) != category_set.end() ) {
          return error( "Duplicate category: " + token.value() + " at " + token.position_string() );
        }
        category_set.insert( token_value );
        spec.m_categories |= it->second;
      } else {
        return error( "Unexpected category: " + token.value() + " at " + token.position_string() );
      }
    } while ( token.value() != kw::END );

    spec.m_categories_specified = true;
    return true;
  }

  bool Parser::parse_available_systems( spl_specification_t& spec ) {
    if ( spec.m_available_systems_specified ) {
      return error( "Available systems specified more than once in file " + spec.m_filename );
    }
    if ( spec.m_missing_systems_specified ) {
      return error( "Only one of AVAILABLE_SYSTEMS or MISSING_SYSTEMS can be specified in file " + spec.m_filename );
    }
    Token token = m_tokenizer.next();
    while ( token.type() != TokenType::EMPTY ) {
      if ( token.type() == TokenType::VALUE ) {
        if ( std::stoull( token.value() ) > spec.m_max_available_systems ) {
          return error( "System " + token.value() + " exceeds the number of available systems: " + std::to_string( spec.m_max_available_systems ) );
        }
        spec.m_available_systems.push_back( std::stoull( token.value() ) );
      } else if ( token.type() == TokenType::KEYWORD && toupper( token.value() ) == kw::END ) {
        break;
      } else {
        return error( "Unexpected token in AVAILABLE_SYSTEMS section: " + token.value() );
      }
      token = m_tokenizer.next();
    }
    spec.m_available_systems_specified = true;
    return true;
  }

  bool Parser::parse_missing_systems( spl_specification_t& spec ) {
    if ( spec.m_missing_systems_specified ) {
      return error( "Missing systems specified more than once in file " + spec.m_filename );
    }
    if ( spec.m_available_systems_specified ) {
      return error( "Only one of AVAILABLE_SYSTEMS or MISSING_SYSTEMS can be specified in file " + spec.m_filename );
    }
    Token token = m_tokenizer.next();
    while ( token.type() != TokenType::EMPTY ) {
      if ( token.type() == TokenType::VALUE ) {
        if ( std::stoull( token.value() ) > spec.m_max_available_systems ) {
          return error( "System " + token.value() + " exceeds the number of available systems: " + std::to_string( spec.m_max_available_systems ) );
        }
        spec.m_missing_systems.push_back( std::stoull( token.value() ) );
      } else if ( token.type() == TokenType::KEYWORD && toupper( token.value() ) == kw::END ) {
        break;
      } else {
        return error( "Unexpected token in MISSING_SYSTEMS section: " + token.value() );
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
      m_os << kw::CATEGORIES;
      for ( const auto& [name, bit] : category_map ) {
        if ( spec.m_categories & bit ) {
          m_os << name << std::endl;
        }
      }
      m_os << std::endl
           << kw::END << std::endl;
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
      error( "Failed to open input file: " + m_filename );
      return false;
    }

    feature_isolation::Parser parser( input_file );
    if ( !parser.parse_specification( m_spec ) ) {
      error( "Failed to parse SPL specification in file: " + m_filename );
      return false;
    }
    if ( !m_spec.validate_systems() ) {
      return error( "Invalid system configuration in file " + m_spec.m_filename );
    }

    return true;
  }

  void spl_processor::print( const std::string& output_filename ) const {
    std::ofstream output_file( output_filename );
    if ( !output_file.is_open() ) {
      error( "Failed to open output file: " + output_filename );
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
