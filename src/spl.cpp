#include "../include/spl.hpp"
#include <algorithm>
#include <z3++.h>

namespace feature_isolation {
  char csv_separator { '\t' };

  namespace error_msgs {
    const std::string FEATURE_NOT_FOUND = "Feature {} not found at line {}, column {}";
    const std::string FEATUREINDEX_NOT_FOUND = "Feature {} is not a specified feature name.";
    const std::string SYSTEM_CONSTRAINT = "System {} violates {} constraint for feature {} and dependent feature {}";
    const std::string UNEXPECTED = "Unexpected {}: {} at line {}, column {}";
    const std::string SPECIFIED_MORE_THAN_ONCE = "{} specified more than once in file {}";
    const std::string EXPECTED_BUT_GOT = "Expected {} but got: {}";
    const std::string ERROR_READING = "Error occurred when reading {} in file {}";
    const std::string DUPLICATE_FOUND = "Duplicate {} found: {} at line {}, column {}";
    const std::string FAILED_TO = "Failed to {} {}: {}";
  } // namespace error_msgs

  const std::unordered_map<std::string_view, token_type_t> kw::keywords = {
      { kw::COMMENT, token_type_t::COMMENT },
      { kw::SPL, token_type_t::KEYWORD },
      { kw::FEATURES, token_type_t::KEYWORD },
      { kw::ROOT, token_type_t::NAME },
      { kw::CATEGORIES, token_type_t::KEYWORD },
      { kw::F, token_type_t::KEYWORD },
      { kw::A, token_type_t::KEYWORD },
      { kw::O, token_type_t::KEYWORD },
      { kw::N, token_type_t::KEYWORD },
      { kw::AN, token_type_t::KEYWORD },
      { kw::ON, token_type_t::KEYWORD },
      { kw::ANF, token_type_t::KEYWORD },
      { kw::ONF, token_type_t::KEYWORD },
      { kw::END, token_type_t::KEYWORD },
      { kw::EXPLICIT_DEPENDENCIES, token_type_t::KEYWORD },
      { kw::REQUIRES_ALL, token_type_t::KEYWORD },
      { kw::REQUIRES_ANY, token_type_t::KEYWORD },
      { kw::REQUIRES_ONE, token_type_t::KEYWORD },
      { kw::EXCLUDES_ALL, token_type_t::KEYWORD },
      { kw::MISSING_SYSTEMS, token_type_t::KEYWORD },
      { kw::AVAILABLE_SYSTEMS, token_type_t::KEYWORD },
      { kw::MAX_RANK, token_type_t::KEYWORD } };

  const std::unordered_map<std::string, unsigned_integer> category_map = {
      { std::string( kw::F ), F }, { std::string( kw::A ), A }, { std::string( kw::O ), O }, { std::string( kw::N ), N }, { std::string( kw::AN ), AN }, { std::string( kw::ON ), ON }, { std::string( kw::ANF ), ANF }, { std::string( kw::ONF ), ONF } };

  template <typename... Args>
  [[noreturn]] void error( const std::string_view& msg, Args&&... args ) {
    std::string formatted_msg = fmt::format( fmt::runtime( msg ), std::forward<Args>( args )... );
    throw std::runtime_error( formatted_msg );
  }

  std::string toupper( std::string s ) {
    std::transform( s.begin(), s.end(), s.begin(), ::toupper );
    return s;
  }

  z3::expr exactly( unsigned n, const z3::expr_vector& xs ) {
    z3::context& ctx = xs[0].ctx();
    z3::expr sum = z3::sum( xs );
    return sum == ctx.int_val( n );
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
        m_max_rank( max_rank.value_or( F ) ),
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
    if ( !validate_systems() ) {
      error( "Failed to validate systems during SPL specification construction." );
    }
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
          error( error_msgs::SYSTEM_CONSTRAINT, std::to_string( system_id ), constraint_type, std::string( kw::ROOT ), m_feature_names[*deps.begin()] );
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
              error( error_msgs::SYSTEM_CONSTRAINT, std::to_string( system_id ), constraint_type, std::string( kw::ROOT ), m_feature_names[*deps.begin()] );
            }
          }
        }
      }
    }
    return true;
  }

  token_t::token_t( const token_type_t& tt, const std::string& v, const position_t& b, const position_t& e )
      : m_token_type( tt ), m_value( v ), m_begin( b ), m_end( e ) {}

  [[nodiscard]] const token_type_t& token_t::type() const {
    return m_token_type;
  }

  [[nodiscard]] const std::string& token_t::value() const {
    if ( m_token_type == token_type_t::EMPTY ) {
      error( "token_t::value() MUST not be called if m_token_type == token_type_t::EMPTY." );
    }
    return m_value;
  }

  [[nodiscard]] const position_t& token_t::begin() const {
    if ( m_token_type == token_type_t::EMPTY ) {
      error( "token_t::begin() MUST not be called if m_token_type == token_type_t::EMPTY." );
    }
    return m_begin;
  }

  [[nodiscard]] const position_t& token_t::end() const {
    if ( m_token_type == token_type_t::EMPTY ) {
      error( "token_t::end() MUST not be called if m_token_type == token_type_t::EMPTY." );
    }
    return m_end;
  }

  [[nodiscard]] const std::string token_t::position_line() const {
    return ( std::to_string( m_begin.first ) );
  }

  [[nodiscard]] const std::string token_t::position_column() const {
    return ( std::to_string( m_begin.second ) );
  }

  tokenizer_t::tokenizer_t( std::istream& is )
      : m_is { is }, m_position { 1, 0 } {}

  token_t tokenizer_t::next() {
    std::string token;
    char c;
    position_t token_begin;
    position_t token_end;
    position_t current_position;

    while ( m_is.get( c ) ) {
      current_position = m_position;

      if ( c == '\n' ) {
        m_position.first++;
        m_position.second = 1;
      } else {
        m_position.second++;
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
            return result;
          }

          auto keyword_check = check_keyword( toupper( token ) );
          if ( keyword_check.has_value() ) {
            token_t result( keyword_check->type(), token, token_begin, token_end );
            return result;
          }

          token_t result( token_type_t::NAME, token, token_begin, token_end );
          return result;
        }

        if ( std::isspace( c ) ) {
          continue;
        } else if ( c == '/' && m_is.peek() == '/' ) {
          m_is.get( c );
          m_position.second++;
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

    if ( !token.empty() ) {
      if ( std::all_of( token.begin(), token.end(), ::isdigit ) ) {
        token_t result( token_type_t::VALUE, token, token_begin, token_end );
        return result;
      }

      auto keyword_check = check_keyword( toupper( token ) );
      if ( keyword_check.has_value() ) {
        token_t result( keyword_check->type(), token, token_begin, token_end );
        return result;
      }

      token_t result( token_type_t::NAME, token, token_begin, token_end );
      return result;
    }

    token_t result( token_type_t::EMPTY, "", m_position, m_position );
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

  parser_t::parser_t( std::istream& is, std::string filename )
      : m_tokenizer { is }, m_spec { parse_specification( filename ) } {}

  spl_specification_t parser_t::parse_specification( const std::string& filename ) {
    std::optional<std::string> spl_name;
    std::optional<unsigned_integer> max_rank;
    unsigned_integer max_available_systems = 0;
    unsigned_integer F = 3;
    feature_names_t feature_names;
    unsigned_integer categories = feature_isolation::F;
    dependency_maps_t dependencies;
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
          error( error_msgs::UNEXPECTED, "keyword", token.value(), token.position_line(), token.position_column() );
        }
      } else {
        error( error_msgs::UNEXPECTED, "", token.value(), token.position_line(), token.position_column() );
      }
    }

    max_available_systems = ( 1 << F );

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
        calculated_available_systems,
        missing_systems,
        spl_specified,
        feature_names_specified,
        categories_specified,
        explicit_dependencies_specified,
        missing_systems_specified,
        available_systems_specified,
        max_rank_specified,
        categories );
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
      error( error_msgs::FEATUREINDEX_NOT_FOUND, feature );
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
      error( error_msgs::EXPECTED_BUT_GOT, std::string( kw::MAX_RANK ), "value", token.value() );
    }
    return std::stoull( token.value() );
  }

  std::pair<unsigned_integer, feature_names_t> parser_t::parse_features() {
    auto token = m_tokenizer.next();
    unsigned_integer f;

    try {
      f = std::stoull( token.value() );
    } catch ( const std::invalid_argument& ) {
      error( "Failed to parse SPL specification: expected a numeric feature count, but got '{}'", token.value() );
    } catch ( const std::out_of_range& ) {
      error( "Failed to parse SPL specification: feature count '{}' is out of range.", token.value() );
    }

    feature_names_t feature_names( f );
    for ( auto& feature : feature_names ) {
      token = m_tokenizer.next();
      feature = token.value();
    }
    return { f, feature_names };
  }

  unsigned_integer parser_t::parse_categories() {
    std::unordered_set<std::string> category_set;
    unsigned_integer categories = 0;
    token_t token;

    do {
      token = m_tokenizer.next();
      if ( category_set.find( token.value() ) != category_set.end() ) {
        error( error_msgs::DUPLICATE_FOUND, "category", token.value(), token.position_line(), token.position_column() );
      }

      std::string token_value = toupper( token.value() );
      if ( token_value == kw::END ) {
        break;
      }

      auto it = category_map.find( token_value );
      if ( it != category_map.end() ) {
        if ( category_set.find( token_value ) != category_set.end() ) {
          error( error_msgs::DUPLICATE_FOUND, "category", token.value(), token.position_line(), token.position_column() );
        }
        category_set.insert( token_value );
        categories |= it->second;
      } else {
        error( error_msgs::UNEXPECTED, "category", token.value(), token.position_line(), token.position_column() );
      }
    } while ( token.value() != kw::END );

    return categories;
  }

  parser_t::dependency_maps_t parser_t::parse_explicit_dependencies( const feature_names_t& feature_names ) {
    dependency_maps_t dependencies;

    while ( true ) {
      auto token = m_tokenizer.next();
      if ( token.type() == token_type_t::KEYWORD && toupper( token.value() ) == kw::END ) {
        break;
      }

      std::string feature_name = token.value();
      auto feature_index_opt = feature_index( feature_name, feature_names );
      if ( !feature_index_opt.has_value() ) {
        error( error_msgs::FEATURE_NOT_FOUND, feature_name, token.position_line(), token.position_column() );
      }

      signed int feature_index = feature_index_opt.value();

      token = m_tokenizer.next();
      if ( token.type() != token_type_t::KEYWORD ) {
        error( error_msgs::EXPECTED_BUT_GOT, "a dependency keyword", token.value(), token.position_line(), token.position_column() );
      }

      std::string dependency_type = toupper( token.value() );

      std::set<unsigned_integer> dependent_features;
      while ( true ) {
        token = m_tokenizer.next();

        if ( token.type() == token_type_t::KEYWORD && toupper( token.value() ) == kw::END ) {
          break;
        }

        if ( token.type() == token_type_t::NAME || token.type() == token_type_t::VALUE ) {
          auto index_opt = this->feature_index( token.value(), feature_names );
          if ( !index_opt.has_value() ) {
            error( error_msgs::FEATURE_NOT_FOUND, token.value(), token.position_line(), token.position_column() );
          }
          dependent_features.insert( static_cast<unsigned_integer>( index_opt.value() ) );
        } else {
          error( error_msgs::UNEXPECTED, "token", token.value(), token.position_line(), token.position_column() );
        }
      }

      if ( dependency_type == kw::REQUIRES_ALL ) {
        dependencies.requires_all[feature_index] = dependent_features;
      } else if ( dependency_type == kw::REQUIRES_ANY ) {
        dependencies.requires_any[feature_index] = dependent_features;
      } else if ( dependency_type == kw::REQUIRES_ONE ) {
        dependencies.requires_one[feature_index] = dependent_features;
      } else if ( dependency_type == kw::EXCLUDES_ALL ) {
        dependencies.excludes_all[feature_index] = dependent_features;
      } else {
        error( error_msgs::UNEXPECTED, "dependency type", dependency_type, token.position_line(), token.position_column() );
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
      if ( token.type() != token_type_t::VALUE ) {
        error( error_msgs::EXPECTED_BUT_GOT, "system id", token.value() );
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
        error( error_msgs::EXPECTED_BUT_GOT, "system id", token.value() );
      }
      systems.push_back( std::stoull( token.value() ) );
    }
    return systems;
  }

  printer_t::printer_t( std::ostream& os )
      : m_os { os } {}

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
      error( error_msgs::FAILED_TO, "open", "input", m_filename );
    }

    parser_t parser( input_file, m_filename );

    return parser.m_spec;
  }

  void spl_processor_t::print( const std::string& output_filename ) const {
    std::ofstream output_file( output_filename );
    if ( !output_file.is_open() ) {
      error( error_msgs::FAILED_TO, "open", "output", output_filename );
    }

    printer_t printer( output_file );
    printer.print_specification( m_spec );
  }

  const spl_specification_t& spl_processor_t::get_specification() const {
    return m_spec;
  }

} // namespace feature_isolation
