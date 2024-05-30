#ifndef SPL_HPP
#define SPL_HPP

#include "feature_analysis2.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <minisat/core/Solver.h>
#include <set>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace feature_isolation {
  namespace kw {
    constexpr std::string_view
        COMMENT { "//" },
        SPL { "SPL" },
        FEATURES { "FEATURES" },
        CATEGORIES { "CATEGORIES" },
        F { "F" },
        A { "A" },
        O { "O" },
        N { "N" },
        AN { "AN" },
        ON { "ON" },
        ANF { "ANF" },
        ONF { "ONF" },
        END { "END" },
        EXPLICIT_DEPENDENCIES { "EXPLICIT_DEPENDENCIES" },
        REQUIRES { "REQUIRES" },
        EXCLUDES { "EXCLUDES" },
        MISSING_SYSTEMS { "MISSING_SYSTEMS" },
        AVAILABLE_SYSTEMS { "AVAILABLE_SYSTEMS" },
        MAX_RANK { "MAX_RANK" };
  }

  [[nodiscard]] std::string to_upper( std::string s ) {
    std::transform( s.begin(), s.end(), s.begin(), ::toupper );
    return s;
  }

  class spl_specification_t final {
  public:
    using feature_names_t = std::vector<std::string>;
    using dependencies_t = std::multimap<unsigned_integer, unsigned_integer>;
    using systems_t = std::set<unsigned_integer>;

    explicit spl_specification_t( const std::string& filename );
    ~spl_specification_t();
    [[nodiscard]] bool setup();
    [[nodiscard]] bool parse();
    void parse_comment();
    void parse_spl();
    void parse_features();
    void parse_categories();
    void parse_explicit_dependencies();
    void parse_missing_systems();
    void parse_available_systems();
    void parse_skip_line();
    void parse_feature();
    void parse_dependency( const std::string& f1, const std::string& dependency, const std::string& f2 );
    void parse_max_rank();
    std::string format_parsed_data() const;
    void output_to_csv( const std::string& output_filename ) const;
    unsigned_integer feature_index( const std::string& f );
    const std::string m_filename;
    std::ifstream m_file;
    const bool m_success_open_file;
    std::string m_spl { "NOT_SPECIFIED" };
    unsigned_integer m_F {};
    feature_names_t m_feature_names;
    unsigned_integer m_categories { F };
    unsigned_integer m_max_rank {};
    dependencies_t m_requires, m_is_required_by, m_excludes, m_is_excluded_by;
    systems_t m_missing_systems, m_available_systems;
    bool m_spl_specified { false },
        m_feature_names_specified { false },
        m_categories_specified { false },
        m_explicit_dependencies_specified { false },
        m_missing_systems_specified { false },
        m_available_systems_specified { false },
        m_max_rank_specified { false };
    const bool m_success_parse;

  private:
    //    void throw_error( const std::string& message ) const;
    void throw_error( const std::string& message,
                      const std::string& fu = std::source_location::current().function_name(),
                      const std::string& fi = std::source_location::current().file_name() ) const;
    void validate_constraints() const; // New function to validate constraints
  };

  /*  void spl_specification_t::throw_error( const std::string& message ) const {
      throw std::runtime_error { message + " in " +
                                  + std::source_location::current().function_name()
                                 " (" + std::source_location::current().file_name() + ")." };
    } */
  void spl_specification_t::throw_error( const std::string& message,
                                         const std::string& fu,
                                         const std::string& fi ) const {
    throw std::runtime_error { message + " in " + fu + " (" + fi + ")." };
  }

  spl_specification_t::spl_specification_t( const std::string& filename )
      : m_filename { filename }, m_success_open_file { setup() },
        m_success_parse { parse() } {
    validate_constraints();
  }

  spl_specification_t::~spl_specification_t() {
    m_file.close();
  }

  bool spl_specification_t::setup() {
    m_line_number = 1;
    m_file.open( m_filename );
    if ( !m_file ) {
      throw_error( "Cannot open file named " + m_filename );
    }
    return true;
  }

  bool spl_specification_t::parse() {
    std::unordered_map<std::string, void ( spl_specification_t::* )()> keyword_map = {
        { kw::COMMENT.data(), &spl_specification_t::parse_comment },
        { kw::SPL.data(), &spl_specification_t::parse_spl },
        { kw::FEATURES.data(), &spl_specification_t::parse_features },
        { kw::CATEGORIES.data(), &spl_specification_t::parse_categories },
        { kw::EXPLICIT_DEPENDENCIES.data(), &spl_specification_t::parse_explicit_dependencies },
        { kw::MISSING_SYSTEMS.data(), &spl_specification_t::parse_missing_systems },
        { kw::AVAILABLE_SYSTEMS.data(), &spl_specification_t::parse_available_systems },
        { kw::MAX_RANK.data(), &spl_specification_t::parse_max_rank } };

    std::string token;
    while ( m_file >> token ) {
      auto keyword = to_upper( token );
      auto it = keyword_map.find( keyword );
      if ( it != keyword_map.end() ) {
        ( this->*( it->second ) )();
      } else {
        throw_error( "Unexpected token " + token );
      }
    }
    // Still need to set default values for items that have not been specified.
    // That's obviously only one, namely m_max_rank.
    if ( !m_max_rank_specified ) {
      m_max_rank = m_F;
    }
    return true;
  }

  void spl_specification_t::parse_comment() {
    parse_skip_line();
  }

  void spl_specification_t::parse_spl() {
    if ( m_spl_specified ) {
      throw_error( "Name of SPL specified more than once in file " + m_filename );
    }
    std::string token;
    m_file >> token;
    m_line_number++;
    if ( !m_file ) {
      throw_error( "Error occurred when reading name of SPL in file " + m_filename );
    }
    m_spl = token;
    m_spl_specified = true;
  }

  void spl_specification_t::parse_features() {
    if ( m_feature_names_specified ) {
      throw_error( "Features specified more than once in file " + m_filename );
    }
    unsigned_integer f;
    m_file >> f;
    m_line_number++;
    if ( !m_file || f == 0u ) {
      throw_error( "Error occurred when reading number of independent features in file " + m_filename );
    }
    m_F = f;
    m_feature_names = feature_names_t( f );
    for ( auto& feature : m_feature_names ) {
      std::string token;
      m_file >> token;
      m_line_number++;
      if ( !m_file ) {
        throw_error( "Error occurred when reading name of independent feature in file " + m_filename );
      }
      while ( to_upper( token ) == kw::COMMENT ) {
        parse_skip_line();
        m_file >> token;
        m_line_number++;
        if ( !m_file ) {
          throw_error( "Error occurred when reading name of independent feature in file " + m_filename );
        }
      }
      feature = token;
    }
    m_feature_names_specified = true;
  }

  void spl_specification_t::parse_categories() {
    if ( m_categories_specified ) {
      throw_error( "Feature categories specified more than once in file " + m_filename );
    }
    std::string token;
    do {
      m_file >> token;
      m_line_number++;
      if ( !m_file ) {
        throw_error( "Error occurred when reading feature category in file " + m_filename );
      }
      token = to_upper( token );
      if ( token == kw::F ) {
        // Nothing to do, F is already (and always) included
      } else if ( token == kw::A && ( m_categories & A ) == 0 ) {
        m_categories |= A;
      } else if ( token == kw::O && ( m_categories & O ) == 0 ) {
        m_categories |= O;
      } else if ( token == kw::N && ( m_categories & N ) == 0 ) {
        m_categories |= N;
      } else if ( token == kw::AN && ( m_categories & AN ) == 0 ) {
        m_categories |= AN;
      } else if ( token == kw::ON && ( m_categories & ON ) == 0 ) {
        m_categories |= ON;
      } else if ( token == kw::ANF && ( m_categories & ANF ) == 0 ) {
        m_categories |= ANF;
      } else if ( token == kw::ONF && ( m_categories & ONF ) == 0 ) {
        m_categories |= ONF;
      } else if ( token == kw::COMMENT ) {
        parse_skip_line();
      } else if ( token != kw::END ) {
        throw_error( "Error occurred when reading feature category in file " + m_filename );
      }
    } while ( token != kw::END );
    m_categories_specified = true;
  }

  void spl_specification_t::parse_explicit_dependencies() {
    if ( m_explicit_dependencies_specified ) {
      throw_error( "Explicit dependencies specified more than once in file " + m_filename );
    }
    if ( !m_feature_names_specified ) {
      throw_error( "Features must be specified before explicit dependencies in file " + m_filename );
    }
    std::string feature1, dependency, feature2, keyword;
    do {
      m_file >> feature1;
      m_line_number++;
      if ( !m_file ) {
        throw_error( "Error occurred when reading 1st feature of dependency in file " + m_filename );
      }
      keyword = to_upper( feature1 );
      if ( keyword == kw::COMMENT ) {
        parse_skip_line();
      } else if ( keyword != kw::END ) {
        m_file >> dependency;
        if ( !m_file ) {
          throw_error( "Error occurred when reading dependency in file " + m_filename );
        }
        dependency = to_upper( dependency );
        if ( dependency != kw::REQUIRES && dependency != kw::EXCLUDES ) {
          throw_error( "Dependency is specified as " + dependency + " in file " + m_filename );
        }
        m_file >> feature2;
        if ( !m_file ) {
          throw_error( "Error occurred when reading 2nd feature of dependency in file " + m_filename );
        }
        parse_dependency( feature1, dependency, feature2 );
      }
    } while ( keyword != kw::END );
    m_explicit_dependencies_specified = true;
  }

  void spl_specification_t::parse_missing_systems() {
    if ( m_missing_systems_specified ) {
      throw_error( "Missing systems specified more than once in file " + m_filename );
    }
    if ( m_available_systems_specified ) {
      throw_error( "Missing systems cannot be specified if available systems have been specified before in file " + m_filename );
    }
    if ( !m_feature_names_specified ) {
      throw_error( "Features must be specified before missing systems in file " + m_filename );
    }

    unsigned_integer total_systems = 1 << m_F;

    std::string token;
    do {
      m_file >> token;
      if ( !m_file ) {
        throw_error( "Error occurred when reading missing systems in file " + m_filename );
      }
      token = to_upper( token );
      if ( token == kw::COMMENT ) {
        parse_skip_line();
      } else if ( token != kw::END ) {
        unsigned_integer id { std::stoull( token ) };
        if ( id < 1 || id > total_systems ) {
          throw_error( "Missing system with ID " + token + " cannot exist in file " + m_filename );
        }
        if ( m_missing_systems.contains( id ) ) {
          throw_error( "Missing system with ID " + token + " has been already specified in file " + m_filename );
        }
        m_missing_systems.insert( id );
        m_line_number++;
      }
    } while ( token != kw::END );
    m_missing_systems_specified = true;
  }

  void spl_specification_t::parse_available_systems() {
    if ( m_available_systems_specified ) {
      throw_error( "Available systems specified more than once" );
    }
    if ( m_missing_systems_specified ) {
      throw_error( "Available systems cannot be specified if missing systems have been specified before" );
    }
    if ( !m_feature_names_specified ) {
      throw_error( "Features must be specified before available systems" );
    }

    unsigned_integer total_systems = 1 << m_F;
    std::string token;
    do {
      m_file >> token;
      if ( !m_file ) {
        throw_error( "Error occurred when reading available systems" );
      }
      token = to_upper( token );
      if ( token == kw::COMMENT ) {
        parse_skip_line();
      } else if ( token != kw::END ) {
        unsigned_integer id { std::stoull( token ) };
        if ( id < 1 || id > total_systems ) {
          throw_error( "Available system with ID " + token + " cannot exist" );
        }
        if ( m_available_systems.contains( id ) ) {
          throw_error( "Available system with ID " + token + " has been already specified" );
        }
        m_available_systems.insert( id );
        m_line_number++;
      }
    } while ( token != kw::END );
    m_available_systems_specified = true;
  }

  void spl_specification_t::parse_max_rank() {
    if ( m_max_rank_specified ) {
      throw_error( "Max rank of interaction specified more than once in file " + m_filename );
    }
    unsigned_integer rank;
    m_file >> rank;
    m_line_number++;
    if ( !m_file ) {
      throw_error( "Error occurred when reading max rank of interaction in file " + m_filename );
    }
    if ( rank > m_F ) {
      throw_error( "Max rank of interaction exceeds number of independent features in file " + m_filename );
    }
    m_max_rank = rank;
    m_max_rank_specified = true;
  }

  void spl_specification_t::parse_skip_line() {
    m_file.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
  }

  void spl_specification_t::parse_dependency( const std::string& f1, const std::string& dependency, const std::string& f2 ) {
    auto f1_index { feature_index( f1 ) },
        f2_index { feature_index( f2 ) };
    if ( f1_index == zero ) {
      throw_error( "Unknown feature " + f1 + " in dependency in file " + m_filename );
    }
    if ( f2_index == zero ) {
      throw_error( "Unknown feature " + f2 + " in dependency in file " + m_filename );
    }
    if ( dependency == kw::REQUIRES ) {
      m_requires.insert( std::make_pair( f1_index, f2_index ) );
      m_is_required_by.insert( std::make_pair( f2_index, f1_index ) );
    }
    if ( dependency == kw::EXCLUDES ) {
      m_excludes.insert( std::make_pair( f1_index, f2_index ) );
      m_is_excluded_by.insert( std::make_pair( f2_index, f1_index ) );
    }
  }

  unsigned_integer spl_specification_t::feature_index( const std::string& f ) {
    auto result { find( m_feature_names.begin(), m_feature_names.end(), f ) };
    if ( result == m_feature_names.end() ) {
      return zero;
    }
    return result - m_feature_names.begin() + one;
  }

  void spl_specification_t::validate_constraints() const {
    if ( !m_explicit_dependencies_specified ) {
      return;
    }

    // Create a MiniSat solver instance
    Minisat::Solver solver;

    // Create variables for each feature
    std::vector<Minisat::Var> feature_vars( m_feature_names.size() );
    for ( auto& var : feature_vars ) {
      var = solver.newVar();
    }

    // Encode requires constraints
    for ( const auto& [f1, f2] : m_requires ) {
      // f1 -> f2 is equivalent to !f1 or f2
      solver.addClause( ~Minisat::mkLit( feature_vars[f1 - 1] ), Minisat::mkLit( feature_vars[f2 - 1] ) );
    }

    // Encode excludes constraints
    for ( const auto& [f1, f2] : m_excludes ) {
      // f1 excludes f2 is equivalent to !f1 or !f2
      solver.addClause( ~Minisat::mkLit( feature_vars[f1 - 1] ), ~Minisat::mkLit( feature_vars[f2 - 1] ) );
    }

    // Calculate the total number of systems
    unsigned_integer total_systems = 1 << m_F; // 2^F systems

    // ASSUME ALL SYSTEMS ARE AVAILABLE IF NEITHER AVAILABLE SYSTEMS NOR MISSING SYSTEMS ARE SPECIFIED
    systems_t effective_available_systems;
    if ( m_available_systems_specified ) {
      effective_available_systems = m_available_systems;
    } else if ( m_missing_systems_specified ) {
      for ( unsigned_integer id = 1; id <= total_systems; ++id ) {
        bool valid = true;
        for ( const auto& [f1, f2] : m_excludes ) {
          if ( ( id & ( 1 << ( f1 - 1 ) ) ) && ( id & ( 1 << ( f2 - 1 ) ) ) ) {
            valid = false;
            break;
          }
        }
        for ( const auto& [f1, f2] : m_requires ) {
          if ( ( id & ( 1 << ( f1 - 1 ) ) ) && !( id & ( 1 << ( f2 - 1 ) ) ) ) {
            valid = false;
            break;
          }
        }
        if ( valid && !m_missing_systems.contains( id ) ) {
          effective_available_systems.insert( id );
        }
      }
    } else {
      for ( unsigned_integer id = 1; id <= total_systems; ++id ) {
        effective_available_systems.insert( id );
      }
    }

    // VALIDATE EFFECTIVE AVAILABLE SYSTEMS AGAINST CONSTRAINTS
    for ( const auto& system : effective_available_systems ) {
      for ( const auto& [f1, f2] : m_excludes ) {
        if ( f1 == f2 ) {
          // Self-exclusion: ensure the feature is not in the system
          if ( system & ( 1 << ( f1 - 1 ) ) ) {
            throw_error( "Available system " + std::to_string( system ) + " includes feature " + m_feature_names[f1 - 1] + " which excludes itself." );
          }
        } else {
          if ( ( system & ( 1 << ( f1 - 1 ) ) ) && ( system & ( 1 << ( f2 - 1 ) ) ) ) {
            throw_error( "Available system " + std::to_string( system ) + " inconsistently excludes both features " + m_feature_names[f1 - 1] + " and " + m_feature_names[f2 - 1] );
          }
        }
      }
      for ( const auto& [f1, f2] : m_requires ) {
        if ( ( system & ( 1 << ( f1 - 1 ) ) ) && !( system & ( 1 << ( f2 - 1 ) ) ) ) {
          throw_error( "Available system " + std::to_string( system ) + " inconsistently requires feature " + m_feature_names[f1 - 1] + " but does not include " + m_feature_names[f2 - 1] );
        }

        if ( f1 == f2 && !( system & ( 1 << ( f1 - 1 ) ) ) ) {
          throw_error( "Available system " + std::to_string( system ) + " does not include feature " + m_feature_names[f1 - 1] + " which requires itself." );
        }
      }
    }

    bool result = solver.solve();
    if ( !result ) {
      throw_error( "Constraints are inconsistent." );
    }
  }

  std::string spl_specification_t::format_parsed_data() const {
    std::ostringstream oss;

    if ( m_spl_specified ) {
      oss << kw::SPL << separator() << m_spl << "\n\n";
    }

    if ( m_feature_names_specified ) {
      oss << kw::FEATURES << separator() << m_F << "\n";
      for ( const auto& feature : m_feature_names ) {
        oss << feature << "\n";
      }
      oss << "\n";
    }

    if ( m_max_rank_specified ) {
      oss << kw::MAX_RANK << separator() << m_max_rank << "\n\n";
    }

    if ( m_categories_specified ) {
      oss << kw::CATEGORIES << "\n";
      std::vector<std::pair<unsigned_integer, std::string_view>> ordered_categories = {
          { F, kw::F }, { A, kw::A }, { O, kw::O }, { N, kw::N }, { AN, kw::AN }, { ON, kw::ON }, { ANF, kw::ANF }, { ONF, kw::ONF } };
      for ( const auto& [bit, name] : ordered_categories ) {
        if ( m_categories & bit ) {
          oss << name << "\n";
        }
      }
      oss << kw::END << "\n\n";
    }

    if ( m_explicit_dependencies_specified ) {
      oss << kw::EXPLICIT_DEPENDENCIES << "\n";
      for ( const auto& [key, value] : m_requires ) {
        oss << m_feature_names[key - 1] << separator() << kw::REQUIRES << separator() << m_feature_names[value - 1] << "\n";
      }
      for ( const auto& [key, value] : m_excludes ) {
        oss << m_feature_names[key - 1] << separator() << kw::EXCLUDES << separator() << m_feature_names[value - 1] << "\n";
      }
      oss << kw::END << "\n\n";
    }

    if ( m_missing_systems_specified ) {
      oss << kw::MISSING_SYSTEMS << "\n";
      for ( const auto& system : m_missing_systems ) {
        oss << system << "\n";
      }
      oss << kw::END << "\n\n";
    }

    if ( m_available_systems_specified ) {
      oss << kw::AVAILABLE_SYSTEMS << "\n";
      for ( const auto& system : m_available_systems ) {
        oss << system << "\n";
      }
      oss << kw::END << "\n\n";
    }

    return oss.str();
  }

  void spl_specification_t::output_to_csv( const std::string& output_filename ) const {
    std::ofstream output_file( output_filename );
    if ( !output_file ) {
      throw_error( "Cannot open output file: " + output_filename );
    }

    output_file << format_parsed_data();
    output_file.close();
  }
} // namespace feature_isolation
#endif // SPL_HPP
