#ifndef FEATURE_ISOLATION_HPP
#define FEATURE_ISOLATION_HPP

#include "../include/fi_mathhelper.hpp"
#include <algorithm> // Because of iota() and find().
#include <array>
#include <climits>
#include <iostream>
#include <numeric> // Because of CHAR_BIT.
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// The following includes are required for spl.hpp
#include <string_view>
#include <fstream>
#include <source_location>
#include <cctype>
#include <limits>
#include <map>
// end

namespace feature_isolation {
  constexpr bool MSB_LSB { true }; // Customization point for MSB at
                                   // left-most position.
  constexpr unsigned_integer feature_digits { sizeof( unsigned_integer ) * CHAR_BIT };

  using feature_t = std::vector<bool>;
  using features_t = std::vector<feature_t>;
  using feature_ids_t = std::set<unsigned_integer>;

  class system_t {
  public:
    feature_ids_t F_feature_ids,
        N_feature_ids,
        A_feature_ids,
        O_feature_ids,
        AN_feature_ids,
        ON_feature_ids,
        ANF_feature_ids,
        ONF_feature_ids;
    std::array<feature_ids_t*, 8> category_sets = {
        &F_feature_ids,
        &N_feature_ids,
        &A_feature_ids,
        &O_feature_ids,
        &AN_feature_ids,
        &ON_feature_ids,
        &ANF_feature_ids,
        &ONF_feature_ids };
    [[nodiscard]] auto DF() const;
    [[nodiscard]] auto T() const;
    auto all_features() const;
  };
  using systems_t = std::vector<system_t>;

  class feature_analysis_t; // Pending declaration.

  std::string to_string( const feature_t& fd );

  constexpr unsigned_integer F { one };
  constexpr unsigned_integer N { feature_isolation::power2( 2 ) };
  constexpr unsigned_integer A { feature_isolation::power2( 3 ) };
  constexpr unsigned_integer O { feature_isolation::power2( 4 ) };
  constexpr unsigned_integer FN { F + N };
  constexpr unsigned_integer AN { feature_isolation::power2( 5 ) };
  constexpr unsigned_integer ON { feature_isolation::power2( 6 ) };
  constexpr unsigned_integer ANF { feature_isolation::power2( 7 ) };
  constexpr unsigned_integer ONF { feature_isolation::power2( 8 ) };
  constexpr unsigned_integer FAO { F + A + O };
  constexpr unsigned_integer FAON { FAO + N };
  constexpr unsigned_integer FAONANON { FAON + AN + ON };
  constexpr unsigned_integer ALL { FAONANON + ANF + ONF };

  static const std::array<std::string, 8> feature_categories = { "F", "N", "A", "O", "AN", "ON", "ANF", "ONF" };

  class feature_analysis_t {
  public:
    explicit feature_analysis_t( unsigned_integer f,
                                 unsigned_integer max_rank,
                                 unsigned_integer feature_categories = ::feature_isolation::ALL,
                                 char separator = '\t' )
        : F { f },
          N { F },
          A { sum_of_combinations( F, two ) },
          O { A },
          AN { O },
          ON { O },
          ANF { sum_of_combinations( F, 1 ) - 1 },
          ONF { ANF },
          DF { f + 4 * A + 2 * ANF },
          T { F + DF },
          R { max_rank },
          C { feature_categories },
          S { power2( F ) },
          F_features { initialize_F_features() },
          N_features { initialize_N_features() },
          A_features { initialize_A_features() },
          O_features { initialize_O_features() },
          AN_features { initialize_AN_features() },
          ON_features { initialize_ON_features() },
          ANF_features { initialize_ANF_features() },
          ONF_features { initialize_ONF_features() },
          F_offset { zero },
          N_offset { F_offset + F_offset },
          A_offset { N_offset + N },
          O_offset { A_offset + A },
          AN_offset { O_offset + O },
          ON_offset { AN_offset + AN },
          ANF_offset { ON_offset + ON },
          ONF_offset { ANF_offset + ANF },
          feature_offsets { F_offset, N_offset, A_offset, O_offset, AN_offset, ON_offset, ANF_offset, ONF_offset },
          all_features { &F_features, &N_features, &A_features, &O_features, &AN_features, &ON_features, &ANF_features, &ONF_features },
          m_systems { initialize_systems() },
          m_separator { separator } {};
    using cui = const unsigned_integer;
    cui F,
        N,
        A,
        O,
        AN,
        ON,
        ANF,
        ONF,
        DF,
        T,
        R,
        C,
        S;
    const features_t F_features,
        N_features,
        A_features,
        O_features,
        AN_features,
        ON_features,
        ANF_features,
        ONF_features;
    [[nodiscard]] feature_t at( unsigned_integer id ) const;
    void print_feature( std::ostream& os, const feature_t& feature ) const;
    void print_features( std::ostream& os ) const;
    void print_categories( std::ostream& os ) const;
    void print_system_statistics( std::ostream& os, const system_t& system, unsigned_integer id ) const;
    void print_systems_statistics( std::ostream& os ) const;
    void print_system_features( std::ostream& os, const system_t& system, unsigned_integer id ) const;
    void print_systems_features( std::ostream& os ) const;
    void print( std::ostream& os ) const;
    [[nodiscard]] char separator() const { return m_separator; }
    void separator( char separator ) { m_separator = separator; }

  private:
    cui F_offset,
        N_offset,
        A_offset,
        O_offset,
        AN_offset,
        ON_offset,
        ANF_offset,
        ONF_offset;
    std::array<cui, 8> feature_offsets;
    std::array<const features_t*, 8> all_features;

  public:
    const systems_t m_systems;

  private:
    char m_separator;
    [[nodiscard]] features_t initialize_F_features() const;
    [[nodiscard]] features_t initialize_N_features() const;
    void and_combine( feature_t& f1, const feature_t& f2 ) const;
    [[nodiscard]] features_t initialize_A_features() const;
    void or_combine( feature_t& f1, const feature_t& f2 ) const;
    [[nodiscard]] features_t initialize_O_features() const;
    [[nodiscard]] features_t initialize_AN_features() const;
    [[nodiscard]] features_t initialize_ON_features() const;
    [[nodiscard]] features_t initialize_ANF_features() const;
    [[nodiscard]] features_t initialize_ONF_features() const;
    void insert_id( systems_t& systems, const feature_t feature, unsigned_integer feature_id, unsigned_integer category ) const;
    [[nodiscard]] systems_t initialize_systems() const;
  };

  // ====================== system_t - member functions ====================== //
  [[nodiscard]] auto system_t::DF() const {
    return N_feature_ids.size() +
           A_feature_ids.size() + O_feature_ids.size() +
           AN_feature_ids.size() + ON_feature_ids.size() +
           ANF_feature_ids.size() + ONF_feature_ids.size();
  }
  auto system_t::all_features() const {
    auto result { F_feature_ids };
    result.insert( N_feature_ids.begin(), N_feature_ids.end() );
    result.insert( A_feature_ids.begin(), A_feature_ids.end() );
    result.insert( O_feature_ids.begin(), O_feature_ids.end() );
    result.insert( AN_feature_ids.begin(), AN_feature_ids.end() );
    result.insert( ON_feature_ids.begin(), ON_feature_ids.end() );
    result.insert( ANF_feature_ids.begin(), ANF_feature_ids.end() );
    result.insert( ONF_feature_ids.begin(), ONF_feature_ids.end() );
    return result;
  }

  // ====================== feature_analysis_t - member functions ====================== //
  [[nodiscard]] feature_t feature_analysis_t::at( unsigned_integer id ) const {
    auto index { zero };
    while ( id < feature_offsets[index] ) {
      ++index;
    }
    return all_features[index]->at( feature_offsets[index] );
  }
  void feature_analysis_t::print_feature( std::ostream& os, const feature_t& feature ) const {
    os << to_string( feature );
  }
  void feature_analysis_t::print_features( std::ostream& os ) const {
    os << "FEATURE ID" << m_separator
       << "CATEGORY" << m_separator
       << "DESCRIPTION" << std::endl;
    for ( auto index { zero }; index < 8; ++index ) {
      if ( C & ( 1 << index ) ) {
        for ( auto id { zero }; const auto& feature : *( all_features[index] ) ) {
          os << ( ++id + feature_offsets[index] ) << m_separator
             << feature_categories[index] << m_separator << '\"';
          print_feature( os, feature );
          os << '\"' << std::endl;
        }
      }
    }
  }
  void feature_analysis_t::print_categories( std::ostream& os ) const {
    os << '\"'
       << ( ( C & ::feature_isolation::F ) ? "F" : "-" ) << '|'
       << ( ( C & ::feature_isolation::N ) ? "N" : "-" ) << '|'
       << ( ( C & ::feature_isolation::A ) ? "A" : "-" ) << '|'
       << ( ( C & ::feature_isolation::O ) ? "O" : "-" ) << '|'
       << ( ( C & ::feature_isolation::AN ) ? "AN" : "--" ) << '|'
       << ( ( C & ::feature_isolation::ON ) ? "ON" : "--" ) << '|'
       << ( ( C & ::feature_isolation::ANF ) ? "ANF" : "---" ) << '|'
       << ( ( C & ::feature_isolation::ONF ) ? "ONF" : "---" )
       << '\"';
  }
  void feature_analysis_t::print_system_statistics( std::ostream& os, const system_t& system, unsigned_integer id ) const {
    os << id << m_separator << system.F_feature_ids.size() << m_separator
       << system.N_feature_ids.size() << m_separator
       << system.A_feature_ids.size() << m_separator
       << system.O_feature_ids.size() << m_separator
       << system.AN_feature_ids.size() << m_separator
       << system.ON_feature_ids.size() << m_separator
       << system.ANF_feature_ids.size() << m_separator
       << system.ONF_feature_ids.size() << m_separator
       << system.DF() << m_separator
       << ( system.DF() + system.F_feature_ids.size() ) << std::endl;
    ;
  }
  void feature_analysis_t::print_systems_statistics( std::ostream& os ) const {
    os << "SYSTEM ID" << m_separator
       << "F" << m_separator
       << "N" << m_separator
       << "A" << m_separator
       << "O" << m_separator
       << "AN" << m_separator
       << "ON" << m_separator
       << "ANF" << m_separator
       << "ONF" << m_separator
       << "DF" << m_separator
       << "T" << std::endl;
    for ( auto id { zero }; const auto& system : m_systems ) {
      print_system_statistics( os, system, ++id );
    }
  }
  void feature_analysis_t::print_system_features( std::ostream& os, const system_t& system, unsigned_integer id ) const {
    auto counter { zero };
    for ( auto feature_id : system.all_features() ) {
      if ( ++counter == 10 ) {
        os << std::endl
           << m_separator;
        counter = 0;
      }
      os << m_separator << ( feature_id + 1 );
    }
  }
  void feature_analysis_t::print_systems_features( std::ostream& os ) const {
    for ( auto id { zero }; const auto& system : m_systems ) {
      os << "SYSTEM ID" << m_separator << ++id;
      print_system_features( os, system, id );
      os << std::endl;
    }
  }
  void feature_analysis_t::print( std::ostream& os ) const {
    os << m_separator
       << "F" << m_separator
       << "N" << m_separator
       << "A" << m_separator
       << "O" << m_separator
       << "AN" << m_separator
       << "ON" << m_separator
       << "ANF" << m_separator
       << "ONF" << m_separator
       << "DF" << m_separator
       << "T" << m_separator
       << "S" << m_separator
       << "R" << m_separator
       << "C" << m_separator
       << "\nIN PRINCIPLE" << m_separator
       << F << m_separator
       << N << m_separator
       << A << m_separator
       << O << m_separator
       << AN << m_separator
       << ON << m_separator
       << ANF << m_separator
       << ONF << m_separator
       << DF << m_separator
       << T << m_separator
       << S << m_separator
       << F << m_separator
       << "F|N|A|O|AN|ON|ANF|ONF";
    auto actual_df { N_features.size() + A_features.size() +
                     O_features.size() + AN_features.size() +
                     ON_features.size() + ANF_features.size() +
                     ONF_features.size() };
    os << "\nACTUALLY" << m_separator
       << F_features.size() << m_separator
       << N_features.size() << m_separator
       << A_features.size() << m_separator
       << O_features.size() << m_separator
       << AN_features.size() << m_separator
       << ON_features.size() << m_separator
       << ANF_features.size() << m_separator
       << ONF_features.size() << m_separator
       << actual_df << m_separator
       << ( F_features.size() + actual_df ) << m_separator
       << S << m_separator
       << R << m_separator;
    print_categories( os );
    os << std::endl;
  }
  [[nodiscard]] features_t feature_analysis_t::initialize_F_features() const {
    features_t result;
    result = features_t( F, feature_t( S, false ) );
    auto stride { one };
    for ( auto feature_index { zero }; feature_index < F; ++feature_index ) {
      bool value { true };
      auto counter { zero };
      for ( auto system_index { zero }; system_index < S; ++system_index ) {
        if ( counter == stride ) {
          value = !value;
          counter = zero;
        }
        ++counter;
        result.at( feature_index ).at( system_index ) = value;
      }
      stride *= 2;
    }
    return result;
  }
  [[nodiscard]] features_t feature_analysis_t::initialize_N_features() const {
    features_t result { F_features };
    for ( auto& feature : result )
      feature.flip();
    return result;
  }
  void feature_analysis_t::and_combine( feature_t& f1, const feature_t& f2 ) const {
    for ( auto index { zero }; index < f1.size(); ++index ) {
      f1.at( index ) = f1.at( index ) && f2.at( index );
    }
  }
  [[nodiscard]] features_t feature_analysis_t::initialize_A_features() const {
    if ( R > F ) {
      throw std::invalid_argument( "R > F in feature_analysis_t::initialize_A_features()" );
    }
    auto result = features_t( sum_of_combinations( F, 2, R ), feature_t( S, true ) );
    std::vector<unsigned_integer> feature_indices( F );
    std::iota( feature_indices.begin(), feature_indices.end(), zero );
    auto feature_index { zero };
    for ( auto drawing_size { two }; drawing_size <= R; ++drawing_size ) {
      combination selection { feature_indices, drawing_size };
      do {
        auto indices { selection() };
        for ( auto index { zero }; index < indices.size(); ++index ) {
          and_combine( result.at( feature_index ), F_features.at( indices.at( index ) ) );
        }
        ++feature_index;
      } while ( selection.next() );
    }
    return result;
  }
  void feature_analysis_t::or_combine( feature_t& f1, const feature_t& f2 ) const {
    for ( auto index { zero }; index < f1.size(); ++index ) {
      f1.at( index ) = f1.at( index ) || f2.at( index );
    }
  }
  [[nodiscard]] features_t feature_analysis_t::initialize_O_features() const {
    if ( R > F ) {
      throw std::invalid_argument( "R > F in feature_analysis_t::initialize_O_features()" );
    }
    auto result = features_t( sum_of_combinations( F, 2, R ), feature_t( S, false ) );
    std::vector<unsigned_integer> feature_indices( F );
    std::iota( feature_indices.begin(), feature_indices.end(), zero );
    auto feature_index { zero };
    for ( auto drawing_size { two }; drawing_size <= R; ++drawing_size ) {
      combination selection { feature_indices, drawing_size };
      do {
        auto indices { selection() };
        for ( auto index { zero }; index < indices.size(); ++index ) {
          or_combine( result.at( feature_index ), F_features.at( indices.at( index ) ) );
        }
        ++feature_index;
      } while ( selection.next() );
    }
    return result;
  }
  [[nodiscard]] features_t feature_analysis_t::initialize_AN_features() const {
    if ( R > F ) {
      throw std::invalid_argument( "R > F in feature_analysis_t::initialize_AN_features()" );
    }
    auto result = features_t( sum_of_combinations( F, 2, R ), feature_t( S, true ) );
    std::vector<unsigned_integer> feature_indices( F );
    std::iota( feature_indices.begin(), feature_indices.end(), zero );
    auto feature_index { zero };
    for ( auto drawing_size { two }; drawing_size <= R; ++drawing_size ) {
      combination selection { feature_indices, drawing_size };
      do {
        auto indices { selection() };
        for ( auto index { zero }; index < indices.size(); ++index ) {
          and_combine( result.at( feature_index ), N_features.at( indices.at( index ) ) );
        }
        ++feature_index;
      } while ( selection.next() );
    }
    return result;
  }
  [[nodiscard]] features_t feature_analysis_t::initialize_ON_features() const {
    if ( R > F ) {
      throw std::invalid_argument( "R > F in feature_analysis_t::initialize_ON_features()" );
    }
    auto result = features_t( sum_of_combinations( F, 2, R ), feature_t( S, false ) );
    std::vector<unsigned_integer> feature_indices( F );
    std::iota( feature_indices.begin(), feature_indices.end(), zero );
    auto feature_index { zero };
    for ( auto drawing_size { two }; drawing_size <= R; ++drawing_size ) {
      combination selection { feature_indices, drawing_size };
      do {
        auto indices { selection() };
        for ( auto index { zero }; index < indices.size(); ++index ) {
          or_combine( result.at( feature_index ), N_features.at( indices.at( index ) ) );
        }
        ++feature_index;
      } while ( selection.next() );
    }
    return result;
  }
  [[nodiscard]] features_t feature_analysis_t::initialize_ANF_features() const {
    if ( R > F ) {
      throw std::invalid_argument( "R > F in feature_analysis_t::initialize_ANF_features()" );
    }
    auto result = features_t( sum_of_combinations( F, 1 ) - 1, feature_t( S, false ) );
    std::vector<unsigned_integer> feature_indices( F );
    std::iota( feature_indices.begin(), feature_indices.end(), zero );
    auto feature_index { zero };
    for ( auto drawing_size { one }; drawing_size < F; ++drawing_size ) {
      combination selection { feature_indices, drawing_size };
      do {
        auto indices { selection() };
        for ( auto index { zero }; index < indices.size(); ++index ) {
          or_combine( result.at( feature_index ), F_features.at( indices.at( index ) ) );
        }
        feature_t not_features( S, false );
        for ( auto index { zero }; index < F; ++index ) {
          if ( std::find( indices.begin(), indices.end(), index ) == indices.end() ) {
            or_combine( not_features, N_features.at( index ) );
          }
        }
        and_combine( result.at( feature_index ), not_features );
        ++feature_index;
      } while ( selection.next() );
    }
    return result;
  }
  [[nodiscard]] features_t feature_analysis_t::initialize_ONF_features() const {
    if ( R > F ) {
      throw std::invalid_argument( "R > F in feature_analysis_t::initialize_ANF_features()" );
    }
    auto result = features_t( sum_of_combinations( F, 1 ) - 1, feature_t( S, false ) );
    std::vector<unsigned_integer> feature_indices( F );
    std::iota( feature_indices.begin(), feature_indices.end(), zero );
    auto feature_index { zero };
    for ( auto drawing_size { one }; drawing_size < F; ++drawing_size ) {
      combination selection { feature_indices, drawing_size };
      do {
        auto indices { selection() };
        for ( auto index { zero }; index < indices.size(); ++index ) {
          or_combine( result.at( feature_index ), F_features.at( indices.at( index ) ) );
        }
        feature_t not_features( S, false );
        for ( auto index { zero }; index < F; ++index ) {
          if ( std::find( indices.begin(), indices.end(), index ) == indices.end() ) {
            or_combine( not_features, N_features.at( index ) );
          }
        }
        or_combine( result.at( feature_index ), not_features );
        ++feature_index;
      } while ( selection.next() );
    }
    return result;
  }
  void feature_analysis_t::insert_id( systems_t& systems, const feature_t feature, unsigned_integer feature_id, unsigned_integer category ) const {
    for ( auto system_id { zero }; auto system : feature ) {
      if ( system ) {
        systems.at( system_id ).category_sets[category]->insert( feature_id );
      }
      ++system_id;
    }
  }
  [[nodiscard]] systems_t feature_analysis_t::initialize_systems() const {
    systems_t result( S, system_t {} );
    auto feature_id { zero };

    struct feature_info_t {
      unsigned_integer mask;
      const std::vector<feature_t>& features;
      unsigned_integer offset;
    };

    std::vector<feature_info_t> feature_info {
        { ::feature_isolation::F, F_features, F },
        { ::feature_isolation::N, N_features, F + N },
        { ::feature_isolation::A, A_features, F + N + A },
        { ::feature_isolation::O, O_features, F + N + A + O },
        { ::feature_isolation::AN, AN_features, F + N + A + O + AN },
        { ::feature_isolation::ON, ON_features, F + N + A + O + AN + ON },
        { ::feature_isolation::ANF, ANF_features, F + N + A + O + AN + ON + ANF },
        { ::feature_isolation::ONF, ONF_features, F + N + A + O + AN + ON + ANF + ONF } };
    unsigned_integer category { zero };
    for ( const auto& info : feature_info ) {
      if ( C & info.mask ) {
        for ( const auto& feature : info.features ) {
          insert_id( result, feature, feature_id++, category );
        }
      }
      feature_id = info.offset;
      category++;
    }

    return result;
  }

  // ====================== free functions ====================== //
  std::string to_string( const feature_t& fd ) {
    std::string result {};
    for ( const auto& b : fd ) {
      if constexpr ( MSB_LSB ) {
        result = std::to_string( b ) + result; //  MSB left most place, LSB right most place
      } else {
        result += std::to_string( b ); // LSB left most place, MSB right most place
      }
    }
    return result;
  }
#include "spl.hpp" // Either integrate or redesign as a separate header file.
} // namespace feature_isolation
#endif // FEATURE_ISOLATION
