#include "feature_analysis.hpp"

namespace feature_isolation {
  [[nodiscard]] auto system_t::DF() const {
    return N_feature_ids.size() + A_feature_ids.size() + O_feature_ids.size() + AN_feature_ids.size() + ON_feature_ids.size() + ANF_feature_ids.size() + ONF_feature_ids.size();
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

  feature_analysis_t::feature_analysis_t( unsigned_integer f, unsigned_integer max_rank, unsigned_integer feature_categories )
      : F { f }, N { F }, A { sum_of_combinations( F, two ) }, O { A }, AN { O }, ON { O }, ANF { sum_of_combinations( F, 1 ) - 1 }, ONF { ANF }, DF { f + 4 * A + 2 * ANF }, T { F + DF }, R { max_rank }, C { feature_categories }, S { power2( F ) },
        F_features { initialize_F_features() }, N_features { initialize_N_features() }, A_features { initialize_A_features() }, O_features { initialize_O_features() }, AN_features { initialize_AN_features() }, ON_features { initialize_ON_features() }, ANF_features { initialize_ANF_features() }, ONF_features { initialize_ONF_features() },
        F_offset { zero }, N_offset { F_offset + F_offset }, A_offset { N_offset + N }, O_offset { A_offset + A }, AN_offset { O_offset + O }, ON_offset { AN_offset + AN }, ANF_offset { ON_offset + ON }, ONF_offset { ANF_offset + ANF },
        feature_offsets { F_offset, N_offset, A_offset, O_offset, AN_offset, ON_offset, ANF_offset, ONF_offset },
        all_features { &F_features, &N_features, &A_features, &O_features, &AN_features, &ON_features, &ANF_features, &ONF_features },
        m_systems { initialize_systems() } {}

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
    os << "FEATURE ID" << separator() << "CATEGORY" << separator() << "DESCRIPTION" << std::endl;
    for ( auto index { zero }; index < 8; ++index ) {
      if ( C & ( 1 << index ) ) {
        for ( auto id { zero }; const auto& feature : *( all_features[index] ) ) {
          os << ( ++id + feature_offsets[index] ) << separator() << feature_categories[index] << separator() << '\"';
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
    os << id << separator() << system.F_feature_ids.size() << separator()
       << system.N_feature_ids.size() << separator()
       << system.A_feature_ids.size() << separator()
       << system.O_feature_ids.size() << separator()
       << system.AN_feature_ids.size() << separator()
       << system.ON_feature_ids.size() << separator()
       << system.ANF_feature_ids.size() << separator()
       << system.ONF_feature_ids.size() << separator()
       << system.DF() << separator()
       << ( system.DF() + system.F_feature_ids.size() ) << std::endl;
    ;
  }

  void feature_analysis_t::print_systems_statistics( std::ostream& os ) const {
    os << "SYSTEM ID" << separator()
       << "F" << separator()
       << "N" << separator()
       << "A" << separator()
       << "O" << separator()
       << "AN" << separator()
       << "ON" << separator()
       << "ANF" << separator()
       << "ONF" << separator()
       << "DF" << separator()
       << "T" << std::endl;
    for ( auto id { zero }; const auto& system : m_systems ) {
      print_system_statistics( os, system, ++id );
    }
  }

 void feature_analysis_t::print_system_features(std::ostream& os, const system_t& system, unsigned_integer id) const {
    os << "SYSTEM ID" << separator() << id << separator();

    // Print all feature IDs for this system
    os << "F: ";
    for (const auto& feature_id : system.F_feature_ids) {
        os << feature_id << " ";
    }

    os << "| N: ";
    for (const auto& feature_id : system.N_feature_ids) {
        os << feature_id << " ";
    }

    os << "| A: ";
    for (const auto& feature_id : system.A_feature_ids) {
        os << feature_id << " ";
    }

    os << "| O: ";
    for (const auto& feature_id : system.O_feature_ids) {
        os << feature_id << " ";
    }

    os << "| AN: ";
    for (const auto& feature_id : system.AN_feature_ids) {
        os << feature_id << " ";
    }

    os << "| ON: ";
    for (const auto& feature_id : system.ON_feature_ids) {
        os << feature_id << " ";
    }

    os << "| ANF: ";
    for (const auto& feature_id : system.ANF_feature_ids) {
        os << feature_id << " ";
    }

    os << "| ONF: ";
    for (const auto& feature_id : system.ONF_feature_ids) {
        os << feature_id << " ";
    }

    os << std::endl;
}


  void feature_analysis_t::print_systems_features( std::ostream& os ) const {
    for ( auto id { zero }; const auto& system : m_systems ) {
      print_system_features( os, system, ++id );
      os << std::endl;
    }
  }

  void feature_analysis_t::print( std::ostream& os ) const {
    os << separator()
       << "F" << separator()
       << "N" << separator()
       << "A" << separator()
       << "O" << separator()
       << "AN" << separator()
       << "ON" << separator()
       << "ANF" << separator()
       << "ONF" << separator()
       << "DF" << separator()
       << "T" << separator()
       << "S" << separator()
       << "R" << separator()
       << "C" << separator()
       << "\nIN PRINCIPLE" << separator()
       << F << separator()
       << N << separator()
       << A << separator()
       << O << separator()
       << AN << separator()
       << ON << separator()
       << ANF << separator()
       << ONF << separator()
       << DF << separator()
       << T << separator()
       << S << separator()
       << F << separator()
       << "F|N|A|O|AN|ON|ANF|ONF";
    auto actual_df { N_features.size() + A_features.size() + O_features.size() + AN_features.size() + ON_features.size() + ANF_features.size() + ONF_features.size() };
    os << "\nACTUALLY" << separator()
       << F_features.size() << separator()
       << N_features.size() << separator()
       << A_features.size() << separator()
       << O_features.size() << separator()
       << AN_features.size() << separator()
       << ON_features.size() << separator()
       << ANF_features.size() << separator()
       << ONF_features.size() << separator()
       << actual_df << separator()
       << ( F_features.size() + actual_df ) << separator()
       << S << separator()
       << R << separator();
    print_categories( os );
    os << std::endl;
    os << "HIER SOLLTEN FEATURE IDs KOMMEN \n";
    print_systems_features( os );
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

  void feature_analysis_t::insert_id( systems_t& systems, const feature_t& feature, unsigned_integer feature_id, unsigned_integer category ) const {
    for ( auto system_id { zero }; auto system : feature ) {
      if ( system ) {
        switch ( category ) {
        case ::feature_isolation::F:
          systems.at( system_id ).F_feature_ids.insert( feature_id );
          break;
        case ::feature_isolation::N:
          systems.at( system_id ).N_feature_ids.insert( feature_id );
          break;
        case ::feature_isolation::A:
          systems.at( system_id ).A_feature_ids.insert( feature_id );
          break;
        case ::feature_isolation::O:
          systems.at( system_id ).O_feature_ids.insert( feature_id );
          break;
        case ::feature_isolation::AN:
          systems.at( system_id ).AN_feature_ids.insert( feature_id );
          break;
        case ::feature_isolation::ON:
          systems.at( system_id ).ON_feature_ids.insert( feature_id );
          break;
        case ::feature_isolation::ANF:
          systems.at( system_id ).ANF_feature_ids.insert( feature_id );
          break;
        case ::feature_isolation::ONF:
          systems.at( system_id ).ONF_feature_ids.insert( feature_id );
          break;
        }
      }
      ++system_id;
    }
  }

  systems_t feature_analysis_t::initialize_systems() const {
    systems_t result( S, system_t {} );
    auto feature_id { zero };

    // Lambda function to reverse the system IDs in the features
    auto reverse_feature = [&]( const features_t& features, unsigned_integer offset, unsigned_integer category ) {
      auto reversed_features = features;
      for ( const auto& feature : reversed_features ) {
        feature_t reversed_feature( S );
        for ( auto system_id { zero }; system_id < S; ++system_id ) {
          reversed_feature[S - system_id - 1] = feature[system_id];
        }
        insert_id( result, reversed_feature, feature_id++, category );
      }
    };

    if ( C & feature_isolation::F ) {
      reverse_feature( F_features, zero, ::feature_isolation::F );
    }
    if ( C & feature_isolation::N ) {
      feature_id = F;
      reverse_feature( N_features, F, ::feature_isolation::N );
    }
    if ( C & feature_isolation::A ) {
      feature_id = F + N;
      reverse_feature( A_features, F + N, ::feature_isolation::A );
    }
    if ( C & feature_isolation::O ) {
      feature_id = F + N + A;
      reverse_feature( O_features, F + N + A, ::feature_isolation::O );
    }
    if ( C & feature_isolation::AN ) {
      feature_id = F + N + A + O;
      reverse_feature( AN_features, F + N + A + O, ::feature_isolation::AN );
    }
    if ( C & feature_isolation::ON ) {
      feature_id = F + N + A + O + AN;
      reverse_feature( ON_features, F + N + A + O + AN, ::feature_isolation::ON );
    }
    if ( C & feature_isolation::ANF ) {
      feature_id = F + N + A + O + AN + ON;
      reverse_feature( ANF_features, F + N + A + O + AN + ON, ::feature_isolation::ANF );
    }
    if ( C & feature_isolation::ONF ) {
      feature_id = F + N + A + O + AN + ON + ANF;
      reverse_feature( ONF_features, F + N + A + O + AN + ON + ANF, ::feature_isolation::ONF );
    }

    return result;
  }

  [[nodiscard]] char separator( const char s ) {
    static char sep = '\t';
    if ( s != 0 ) {
      sep = s;
    }
    return sep;
  }

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
} // namespace feature_isolation
