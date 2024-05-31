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
    unsigned_integer f_count = system.category_sets[0].size();
    unsigned_integer n_count = system.category_sets[1].size();
    unsigned_integer a_count = system.category_sets[2].size();
    unsigned_integer o_count = system.category_sets[3].size();
    unsigned_integer an_count = system.category_sets[4].size();
    unsigned_integer on_count = system.category_sets[5].size();
    unsigned_integer anf_count = system.category_sets[6].size();
    unsigned_integer onf_count = system.category_sets[7].size();
    unsigned_integer df_count = f_count + n_count + a_count + o_count + an_count + on_count + anf_count + onf_count;
    unsigned_integer t_count = df_count + f_count;

    os << id << separator() << f_count << separator()
       << n_count << separator()
       << a_count << separator()
       << o_count << separator()
       << an_count << separator()
       << on_count << separator()
       << anf_count << separator()
       << onf_count << separator()
       << df_count << separator()
       << t_count << std::endl;
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

  void feature_analysis_t::print_system_features( std::ostream& os, const system_t& system, unsigned_integer id ) const {
    os << "SYSTEM ID" << separator() << id << separator();

    // Calculate the bitstring for (id - 1)
    unsigned_integer value = id - 1;
    std::string bitstring;
    for ( unsigned_integer i = 0; i < F; ++i ) {
      bitstring = ( ( value % 2 ) ? '1' : '0' ) + bitstring;
      value /= 2;
    }
    os << bitstring;
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
    for ( unsigned_integer system_id = 0; system_id < systems.size(); ++system_id ) {
      if ( feature[system_id] ) {
        systems[system_id].category_sets[category].insert( feature_id );
      }
    }
  }

  systems_t feature_analysis_t::initialize_systems() const {
    systems_t systems( S );

    // Define the size of the bitsets based on the number of systems
    constexpr size_t bitset_size = sizeof( unsigned_integer ) * 8;

    // Initialize bitstrings for each system
    for ( unsigned_integer system_id = 0; system_id < S; ++system_id ) {
      std::vector<std::bitset<bitset_size>> bitstrings( 8 ); // 0: Independent, 1: Not, 2: Or, 3: And, 4: Or-Not, 5: And-Not, 6: ANF, 7: ONF

      // Local sets to store feature IDs for each category
      std::array<std::set<unsigned_integer>, 8> local_category_sets;

      // Independent Features
      bitstrings[0] = std::bitset<bitset_size>( system_id );

      // Not-Features
      bitstrings[1] = ~bitstrings[0];

      // Or-Features
      for ( unsigned_integer i = 0; i < F; ++i ) {
        for ( unsigned_integer j = i + 1; j < F; ++j ) {
          bitstrings[2][i] = bitstrings[2][i] || ( bitstrings[0][i] || bitstrings[0][j] );
        }
      }

      // And-Features
      for ( unsigned_integer i = 0; i < F; ++i ) {
        for ( unsigned_integer j = i + 1; j < F; ++j ) {
          bitstrings[3][i] = bitstrings[3][i] && ( bitstrings[0][i] && bitstrings[0][j] );
        }
      }

      // Or-Not-Features
      for ( unsigned_integer i = 0; i < F; ++i ) {
        for ( unsigned_integer j = i + 1; j < F; ++j ) {
          bitstrings[4][i] = bitstrings[4][i] || ( bitstrings[1][i] || bitstrings[1][j] );
        }
      }

      // And-Not-Features
      for ( unsigned_integer i = 0; i < F; ++i ) {
        for ( unsigned_integer j = i + 1; j < F; ++j ) {
          bitstrings[5][i] = bitstrings[5][i] && ( bitstrings[1][i] && bitstrings[1][j] );
        }
      }

      // ANF-Features
      for ( unsigned_integer i = 0; i < F; ++i ) {
        bitstrings[6][i] = bitstrings[0][i] && bitstrings[5][i];
      }

      // ONF-Features
      for ( unsigned_integer i = 0; i < F; ++i ) {
        bitstrings[7][i] = bitstrings[0][i] || bitstrings[4][i];
      }

      // Insert feature IDs into local sets based on bitstrings
      auto insert_features = [&]( const std::bitset<bitset_size>& bitstring, unsigned_integer start_id, unsigned_integer category ) {
        for ( unsigned_integer feature_id = 0; feature_id < F; ++feature_id ) {
          if ( bitstring[feature_id] ) {
            local_category_sets[category].insert( start_id + feature_id );
          }
        }
      };

      insert_features( bitstrings[0], 1, 0 );                                 // Independent Features
      insert_features( bitstrings[1], F + 1, 1 );                             // Not-Features
      insert_features( bitstrings[2], F + N + 1, 2 );                         // Or-Features
      insert_features( bitstrings[3], F + N + A + 1, 3 );                     // And-Features
      insert_features( bitstrings[4], F + N + A + O + 1, 4 );                 // Or-Not-Features
      insert_features( bitstrings[5], F + N + A + O + AN + 1, 5 );            // And-Not-Features
      insert_features( bitstrings[6], F + N + A + O + AN + ON + 1, 6 );       // ANF-Features
      insert_features( bitstrings[7], F + N + A + O + AN + ON + ANF + 1, 7 ); // ONF-Features

      // Assign local sets to the current system's category sets
      systems[system_id].category_sets = std::move( local_category_sets );

      // Debug print all the contents of the sets for the current system
      std::cout << "System " << system_id << ":\n";
      for ( unsigned_integer category = 0; category < 8; ++category ) {
        std::cout << "  Category " << category << ": ";
        for ( const auto& feature_id : systems[system_id].category_sets[category] ) {
          std::cout << feature_id << " ";
        }
        std::cout << "\n";
      }
    }

    return systems;
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
