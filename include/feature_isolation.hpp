#ifndef FEATURE_ISOLATION_HPP
#define FEATURE_ISOLATION_HPP

#include "../include/fi_mathhelper.hpp"
#include <algorithm>
#include <array>
#include <bitset>
#include <iostream>
#include <string>

namespace feature_isolation {
   static const unsignedInteger F{1llu};
   static const unsignedInteger O{feature_isolation::power2(2)};
   static const unsignedInteger A{feature_isolation::power2(3)};
   static const unsignedInteger N{feature_isolation::power2(4)};
   static const unsignedInteger FN{F + N};
   static const unsignedInteger ON{feature_isolation::power2(5)};
   static const unsignedInteger AN{feature_isolation::power2(6)};
   static const unsignedInteger ONF{feature_isolation::power2(7)};
   static const unsignedInteger ANF{feature_isolation::power2(8)};
   static const unsignedInteger FOA{F + O + A};
   static const unsignedInteger FOAN{FOA + N};
   static const unsignedInteger FOANONAN{FOAN + ON + AN};
   static const unsignedInteger ALL{FOANONAN + ONF + ANF};

   template <unsignedInteger f>
   class feature_isolationT {
    public:
      inline static constexpr unsignedInteger F{f};
      inline static constexpr unsignedInteger S{feature_isolation::power2(F)};
      inline static constexpr unsignedInteger O{feature_isolation::sum_of_combinations(F, 2)};
      inline static constexpr unsignedInteger A{O};
      inline static constexpr unsignedInteger N{F};
      inline static constexpr unsignedInteger ON{O};
      inline static constexpr unsignedInteger AN{O};
      inline static constexpr unsignedInteger ONF{feature_isolation::sum_of_combinations(F, 1) - 1};
      inline static constexpr unsignedInteger ANF{ONF};
      inline static constexpr unsignedInteger DF{f + 4 * O + 2 * ONF};
      inline static constexpr unsignedInteger T = {F + DF};
      inline static std::bitset<S> m_available_systems{~0llu};
      inline static unsignedInteger m_feature_categories{feature_isolation::ALL};
      struct featureT {
         std::string name{};
         std::bitset<S> id{};
      };
      struct systemT {
         std::vector<featureT> features{};
         unsignedInteger F{},
             O{},
             A{},
             N{},
             ON{},
             AN{},
             ONF{},
             ANF{};
      };
      template <unsignedInteger size>
      using featuresT = std::array<featureT, size>;
      using systemsT = std::array<systemT, S>;
      inline static systemsT m_systems;
      inline static featuresT<F> m_F;
      static void F_features() {
         using namespace std::string_literals; // for using "x"s as string literal
         unsignedInteger i{0llu};
         for (unsignedInteger shift_to_left{0llu}; shift_to_left < F; ++shift_to_left) {
            std::string name{"f"s + std::to_string(shift_to_left + 1)};
            unsignedInteger mask{1llu << shift_to_left};
            std::bitset<S> difference_expression{};
            std::vector<unsignedInteger> systems;
            for (unsignedInteger system{0llu}; system < S; ++system) {
               if (system & mask) {
                  difference_expression.set(system);
                  systems.push_back(system + 1);
               }
            }
            featureT feature{name, difference_expression};
            m_F.at(i++) = feature;
            for (const auto& s : systems) {
               m_systems.at(s - 1).features.push_back(feature);
               ++(m_systems.at(s - 1).F);
            }
         }
      }
      inline static featuresT<N> m_N;
      static void N_features() {
         using namespace std::string_literals; // for using "x"s as string literal
         unsignedInteger i{0llu};
         for (unsignedInteger shift_to_left{0llu}; shift_to_left < F; ++shift_to_left) {
            std::string name{"!f"s + std::to_string(shift_to_left + 1)};
            unsignedInteger mask{1llu << shift_to_left};
            std::bitset<S> difference_expression{};
            std::vector<unsignedInteger> systems;
            for (unsignedInteger system{0llu}; system < S; ++system) {
               if (!(system & mask)) {
                  difference_expression.set(system);
                  systems.push_back(system + 1);
               }
            }
            featureT feature{name, difference_expression};
            m_N.at(i++) = feature;
            for (const auto& s : systems) {
               m_systems.at(s - 1).features.push_back(feature);
               ++(m_systems.at(s - 1).N);
            }
         }
      }
      inline static featuresT<O> m_O;
      static void O_features() {
         using namespace std::string_literals; // for using "x"s as string literal
         unsignedInteger i{0llu};
         for (unsignedInteger k{2llu}; k <= F; ++k) {
            std::vector<unsignedInteger> features;
            for (unsignedInteger n{1llu}; n <= F; ++n) {
               features.push_back(n);
            }
            feature_isolation::combinationT or_features{features, k};
            do {
               std::string name{};
               unsignedInteger mask{};
               auto draw{or_features()};
               for (unsignedInteger n{0llu}; const auto& element : draw) {
                  name += "f"s + std::to_string(element);
                  if (++n < draw.size()) {
                     name += " + "s;
                  }
                  mask |= (1llu << (element - 1));
               }
               std::bitset<S> difference_expression{};
               std::vector<unsignedInteger> systems;
               for (unsignedInteger system{0llu}; system < S; ++system) {
                  if (system & mask) {
                     difference_expression.set(system);
                     systems.push_back(system + 1);
                  }
               }
               featureT feature{name, difference_expression};
               m_O.at(i++) = feature;
               for (const auto& s : systems) {
                  m_systems.at(s - 1).features.push_back(feature);
                  ++(m_systems.at(s - 1).O);
               }
            } while (or_features.next());
         }
      }
      inline static featuresT<A> m_A;
      static void A_features() {
         using namespace std::string_literals; // for using "x"s as string literal
         unsignedInteger i{0llu};
         for (unsignedInteger k{2llu}; k <= F; ++k) {
            std::vector<unsignedInteger> features;
            for (unsignedInteger n{1}; n <= F; ++n) {
               features.push_back(n);
            }
            feature_isolation::combinationT and_features{features, k};
            do {
               std::string name{};
               unsignedInteger mask{};
               auto draw{and_features()};
               for (unsignedInteger n{0llu}; const auto& element : draw) {
                  name += "f"s + std::to_string(element);
                  if (++n < draw.size()) {
                     name += " * "s;
                  }
                  mask |= (1llu << (element - 1));
               }
               std::bitset<S> difference_expression{};
               std::vector<unsignedInteger> systems;
               for (unsignedInteger system{0llu}; system < S; ++system) {
                  if ((system & mask) == mask) {
                     difference_expression.set(system);
                     systems.push_back(system + 1);
                  }
               }
               featureT feature{name, difference_expression};
               m_A.at(i++) = feature;
               for (const auto& s : systems) {
                  m_systems.at(s - 1).features.push_back(feature);
                  ++(m_systems.at(s - 1).A);
               }
            } while (and_features.next());
         }
      }
      inline static featuresT<ON> m_ON;
      static void ON_features() {
         using namespace std::string_literals; // for using "x"s as string literal
         unsignedInteger i{0llu};
         for (unsignedInteger k{2llu}; k <= F; ++k) {
            std::vector<unsignedInteger> features;
            for (unsignedInteger n{1llu}; n <= F; ++n) {
               features.push_back(n);
            }
            feature_isolation::combinationT or_not_features{features, k};
            do {
               std::string name{};
               unsignedInteger mask{};
               auto draw{or_not_features()};
               for (unsignedInteger n{0llu}; const auto& element : draw) {
                  name += "!f"s + std::to_string(element);
                  if (++n < draw.size()) {
                     name += " + "s;
                  }
                  mask |= 1llu << (element - 1);
               }
               std::bitset<S> difference_expression{};
               std::vector<unsignedInteger> systems;
               for (unsignedInteger system{0llu}; system < S; ++system) {
                  if (!((system & mask) == mask)) {
                     difference_expression.set(system);
                     systems.push_back(system + 1);
                  }
               }
               featureT feature{name, difference_expression};
               m_ON.at(i++) = feature;
               for (const auto& s : systems) {
                  m_systems.at(s - 1).features.push_back(feature);
                  ++(m_systems.at(s - 1).ON);
               }
            } while (or_not_features.next());
         }
      }
      inline static featuresT<AN> m_AN;
      static void AN_features() {
         using namespace std::string_literals; // for using "x"s as string literal
         unsignedInteger i{0llu};
         for (unsignedInteger k{2llu}; k <= F; ++k) {
            std::vector<unsignedInteger> features;
            for (unsignedInteger n{1llu}; n <= F; ++n) {
               features.push_back(n);
            }
            feature_isolation::combinationT and_not_features{features, k};
            do {
               std::string name{};
               unsignedInteger mask{};
               auto draw{and_not_features()};
               for (unsignedInteger n{0llu}; const auto& element : draw) {
                  name += "!f"s + std::to_string(element);
                  if (++n < draw.size()) {
                     name += " * "s;
                  }
                  mask |= 1llu << (element - 1);
               }
               std::bitset<S> difference_expression{};
               std::vector<unsignedInteger> systems;
               for (unsignedInteger system{0llu}; system < S; ++system) {
                  if (!(system & mask)) {
                     difference_expression.set(system);
                     systems.push_back(system + 1);
                  }
               }
               featureT feature{name, difference_expression};
               m_AN.at(i++) = feature;
               for (const auto& s : systems) {
                  m_systems.at(s - 1).features.push_back(feature);
                  ++(m_systems.at(s - 1).AN);
               }
            } while (and_not_features.next());
         }
      }
      inline static featuresT<ONF> m_ONF;
      static void ONF_features() {
         using namespace std::string_literals; // for using "x"s as string literal
         unsignedInteger i{0llu};
         for (unsignedInteger k{1llu}; k < F; ++k) {
            std::vector<unsignedInteger> features;
            for (unsignedInteger n{1}; n <= F; ++n) {
               features.push_back(n);
            }
            feature_isolation::combinationT independent_features{features, k};
            do {
               std::string name{};
               unsignedInteger f_mask{},
                   n_mask{};
               auto draw{independent_features()};
               unsignedInteger f_index{0llu};
               for (unsignedInteger n{0llu}; n < F; ++n) {

                  if (f_index < draw.size() && draw.at(f_index) == (n + 1)) {
                     name += "f"s + std::to_string(n + 1);
                     f_mask |= 1llu << n;
                     ++f_index;
                  } else {
                     name += "!f"s + std::to_string(n + 1);
                     n_mask |= 1llu << n;
                  }
                  if (n < F - 1) {
                     name += " + "s;
                  }
               }
               std::bitset<S> difference_expression{};
               std::vector<unsignedInteger> systems;
               for (unsignedInteger system{0llu}; system < S; ++system) {
                  if ((system & f_mask) || (!((system & n_mask) == n_mask))) {
                     difference_expression.set(system);
                     systems.push_back(system + 1);
                  }
               }
               featureT feature{name, difference_expression};
               m_ONF.at(i++) = feature;
               for (const auto& s : systems) {
                  m_systems.at(s - 1).features.push_back(feature);
                  ++(m_systems.at(s - 1).ONF);
               }
            } while (independent_features.next());
         }
      }
      inline static featuresT<ANF> m_ANF;
      static void ANF_features() {
         using namespace std::string_literals; // for using "x"s as string literal
         unsignedInteger i{0llu};
         for (unsignedInteger k{1llu}; k < F; ++k) {
            std::vector<unsignedInteger> features;
            for (unsignedInteger n{1}; n <= F; ++n) {
               features.push_back(n);
            }
            feature_isolation::combinationT independent_features{features, k};
            do {
               std::string name{};
               unsignedInteger f_mask{},
                   n_mask{};
               auto draw{independent_features()};
               unsignedInteger f_index{0llu};
               for (unsignedInteger n{0llu}; n < F; ++n) {

                  if (f_index < draw.size() && draw.at(f_index) == (n + 1)) {
                     name += "f"s + std::to_string(n + 1);
                     f_mask |= 1llu << n;
                     ++f_index;
                  } else {
                     name += "!f"s + std::to_string(n + 1);
                     n_mask |= 1llu << n;
                  }
                  if (n < F - 1) {
                     name += " * "s;
                  }
               }
               std::bitset<S> difference_expression{};
               std::vector<unsignedInteger> systems;
               for (unsignedInteger system{0llu}; system < S; ++system) {
                  if ((system & f_mask) && (!((system & n_mask) == n_mask))) {
                     difference_expression.set(system);
                     systems.push_back(system + 1);
                  }
               }
               featureT feature{name, difference_expression};
               m_ANF.at(i++) = feature;
               for (const auto& s : systems) {
                  m_systems.at(s - 1).features.push_back(feature);
                  ++(m_systems.at(s - 1).ANF);
               }
            } while (independent_features.next());
         }
      }
      static void initialize(const unsignedInteger categories,
                             const std::bitset<S>& available_systems = std::bitset<S>(~0llu)) {
         m_available_systems = available_systems;
         m_feature_categories = categories;
         if (categories & feature_isolation::F) {
            F_features();
         }
         if (categories & feature_isolation::N) {
            N_features();
         }
         if (categories & feature_isolation::O) {
            O_features();
         }
         if (categories & feature_isolation::A) {
            A_features();
         }
         if (categories & feature_isolation::ON) {
            ON_features();
         }
         if (categories & feature_isolation::AN) {
            AN_features();
         }
         if (categories & feature_isolation::ONF) {
            ONF_features();
         }
         if (categories & feature_isolation::ANF) {
            ANF_features();
         }
      }
      feature_isolationT(const unsignedInteger categories = feature_isolation::ALL,
                         const std::bitset<S>& available_systems = std::bitset<S>(~0llu)) {
         initialize(categories, available_systems);
      }
      static void print_general_information(std::ostream& os) {
         unsignedInteger o{(m_feature_categories & feature_isolation::O ? O
                                                                        : 0)};
         unsignedInteger a{(m_feature_categories & feature_isolation::A ? A : 0)};
         unsignedInteger n{(m_feature_categories & feature_isolation::N ? N : 0)};
         unsignedInteger on{(m_feature_categories & feature_isolation::ON ? ON : 0)};
         unsignedInteger an{(m_feature_categories & feature_isolation::AN ? AN : 0)};
         unsignedInteger onf{(m_feature_categories & feature_isolation::ONF ? ONF : 0)};
         unsignedInteger anf{(m_feature_categories & feature_isolation::ANF ? ANF : 0)};
         unsignedInteger df{o + a + n + on + an + onf + anf};
         unsignedInteger t{F + df};
         os << "F"
            << "\t" << F << '\n'
            << "S"
            << "\t" << S << '\n'
            << "O"
            << "\t" << o << '\n'
            << "A"
            << "\t" << a << '\n'
            << "N"
            << "\t" << n << '\n'
            << "ON"
            << "\t" << on << '\n'
            << "AN"
            << "\t" << an << '\n'
            << "ONF"
            << "\t" << onf << '\n'
            << "ANF"
            << "\t" << anf << '\n'
            << "DF"
            << "\t" << df << '\n'
            << "T"
            << "\t" << t << '\n'
            << std::endl;
      }
      static unsignedInteger to_llu(const std::bitset<S>& bits) {
         unsignedInteger result{};
         for (unsignedInteger n{0llu}; n < S; ++n) {
            if (bits[n]) {
               result |= 1llu << n;
            }
         }
         return result;
      }
      template <unsignedInteger size>
      static void print_features(std::ostream& os, const featuresT<size>& features, const std::string& category) {
         if (feature_isolation::to_unsignedInteger(features.at(0).id) == 0) // dirty hack to suppress output of empty feature categories.
         {
            return;
         }
         os << features.size() << " " << category << "-FEATURES" << std::endl;
         for (const auto& e : features) {
            os << to_llu(e.id) << "\t" << e.name << "\t" << e.id << std::endl;
         }
         os << std::endl;
      }
      static void print_independent_features(std::ostream& os) {
         print_features(os, m_F, "INDEPENDENT");
      }
      static void print_or_features(std::ostream& os) {
         print_features(os, m_O, "OR");
      }
      static void print_and_features(std::ostream& os) {
         print_features(os, m_A, "AND");
      }
      static void print_not_features(std::ostream& os) {
         print_features(os, m_N, "NOT");
      }
      static void print_or_not_features(std::ostream& os) {
         print_features(os, m_ON, "OR-NOT");
      }
      static void print_and_not_features(std::ostream& os) {
         print_features(os, m_AN, "AND-NOT");
      }
      static void print_or_not_features_independent_features(std::ostream& os) {
         print_features(os, m_ONF, "OR-NOT-FEATURES-INDEPENDENT");
      }
      static void print_and_not_features_independent_features(std::ostream& os) {
         print_features(os, m_ANF, "AND-NOT-FEATURES-INDEPENDENT");
      }
      static void print_systems_with_feature_names(std::ostream& os) {
         os << 'S' << "\t"
            << "FEATURES" << std::endl;
         for (unsignedInteger n{0llu}; const auto& system : m_systems) {
            os << "S" << ++n << "\t";
            for (const auto& feature : system.features) {
               os << feature.name << "\t";
            }
            os << std::endl;
         }
         os << std::endl;
      }
      static void print_all_features(std::ostream& os) {
         print_independent_features(os);
         print_or_features(os);
         print_and_features(os);
         print_not_features(os);
         print_or_not_features(os);
         print_and_not_features(os);
         print_or_not_features_independent_features(os);
         print_and_not_features_independent_features(os);
      }
      static void statistics(std::ostream& os) {
         os << "S"
            << "\t"
            << "F"
            << "\t"
            << "O"
            << "\t"
            << "A"
            << "\t"
            << "N"
            << "\t"
            << "ON"
            << "\t"
            << "AN"
            << "\t"
            << "ONF"
            << "\t"
            << "ANF"
            << "\t"
            << "T"
            << "\t"
            << "DF" << std::endl;
         for (unsignedInteger n{0llu}; const auto& s : m_systems) {
            os << "S" << ++n << "\t" << s.F << "\t" << s.O << "\t" << s.A << "\t"
               << s.N << "\t" << s.ON << "\t" << s.AN << "\t"
               << s.ONF << "\t" << s.ANF << "\t"
               << s.features.size() << "\t" << (s.features.size() - s.F)
               << std::endl;
         }
      }
      static void systems_as_feature_matrix(std::ostream& os) {
         std::vector<unsignedInteger> e_ids{};
         for (const auto& feature : m_F) {
            e_ids.push_back(feature_isolation::to_unsignedInteger(feature.id));
         }
         for (const auto& feature : m_O) {
            e_ids.push_back(feature_isolation::to_unsignedInteger(feature.id));
         }
         for (const auto& feature : m_A) {
            e_ids.push_back(feature_isolation::to_unsignedInteger(feature.id));
         }
         for (const auto& feature : m_N) {
            e_ids.push_back(feature_isolation::to_unsignedInteger(feature.id));
         }
         for (const auto& feature : m_ON) {
            e_ids.push_back(feature_isolation::to_unsignedInteger(feature.id));
         }
         for (const auto& feature : m_AN) {
            e_ids.push_back(feature_isolation::to_unsignedInteger(feature.id));
         }
         for (const auto& feature : m_ONF) {
            e_ids.push_back(feature_isolation::to_unsignedInteger(feature.id));
         }
         for (const auto& feature : m_ANF) {
            e_ids.push_back(feature_isolation::to_unsignedInteger(feature.id));
         }
         std::sort(e_ids.begin(), e_ids.end());
         std::vector<unsignedInteger> frequencies(T);
         os << "\t"
            << "EXPRESSION-IDs" << std::endl;
         os << "S";
         for (unsignedInteger n{0llu}; n < T; ++n) {
            os << "\t" << e_ids.at(n);
         }
         os << std::endl;
         std::cout << std::endl;
         for (unsignedInteger n{0llu}; const auto& s : m_systems) {
            std::bitset<T> all_features{};
            for (const auto& feature : s.features) {
               auto index{std::find(e_ids.begin(), e_ids.end(),
                                    feature_isolation::to_unsignedInteger(feature.id)) -
                          e_ids.begin()};
               all_features[index] = true;
               frequencies.at(index) += 1;
            }
            os << "S" << ++n;
            for (unsignedInteger i{0llu}; i < all_features.size(); ++i) {
               os << "\t";
               if (all_features[i]) {
                  os << '1';
               } else {
                  os << '0';
               }
            }
            os << std::endl;
         }
         os << 'H';
         for (const auto& h : frequencies) {
            os << "\t" << h;
         }
         os << std::endl;
         os << std::endl;
      }
      static void print_available_systems(std::ostream& os) {
         os << "SYSTEMS IDS"
            << "\t"
            << "(POSITIVE = AVAILABLE)"
            << "\t"
            << "((NEGATIVE = NOT AVAILABLE)" << std::endl;
         const unsignedInteger max_column{feature_isolation::power2(F / 2)};
         unsignedInteger column{};
         for (unsignedInteger i{}; i < m_available_systems.size(); ++i) {
            if (!m_available_systems[i]) {
               os << '-';
            }
            os << (i + 1);
            if (++column == max_column) {
               os << std::endl;
               column = 0llu;
            } else {
               os << "\t";
            }
         }
         os << std::endl;
      }
      static std::vector<unsignedInteger> system_to_features(unsignedInteger system_index) {
         std::vector<unsignedInteger> result;
         for (const auto& feature : m_systems.at(system_index).features) {
            result.push_back(feature_isolation::to_unsignedInteger(feature.id));
         }
         std::sort(result.begin(), result.end());
         return result;
      }
      static void print(std::string msg, std::vector<unsignedInteger>& v) {
         std::cout << msg;
         for (auto& s : v)
            std::cout << " " << s;
         std::cout << std::endl;
      }
      static std::string to_feature_name(unsignedInteger id) {
         for (const auto& feature : m_F) {
            if (feature_isolation::to_unsignedInteger(feature.id) == id) {
               return feature.name;
            }
         }
         for (const auto& feature : m_O) {
            if (feature_isolation::to_unsignedInteger(feature.id) == id) {
               return feature.name;
            }
         }
         for (const auto& feature : m_A) {
            if (feature_isolation::to_unsignedInteger(feature.id) == id) {
               return feature.name;
            }
         }
         for (const auto& feature : m_N) {
            if (feature_isolation::to_unsignedInteger(feature.id) == id) {
               return feature.name;
            }
         }
         for (const auto& feature : m_ON) {
            if (feature_isolation::to_unsignedInteger(feature.id) == id) {
               return feature.name;
            }
         }
         for (const auto& feature : m_AN) {
            if (feature_isolation::to_unsignedInteger(feature.id) == id) {
               return feature.name;
            }
         }
         for (const auto& feature : m_ONF) {
            if (feature_isolation::to_unsignedInteger(feature.id) == id) {
               return feature.name;
            }
         }
         for (const auto& feature : m_ANF) {
            if (feature_isolation::to_unsignedInteger(feature.id) == id) {
               return feature.name;
            }
         }
         return "INVALID";
      }
      static bool isolate_feature(std::ostream& os, const featureT& feature) {
         using namespace std;
         decltype(featureT::id) intersections{m_available_systems & feature.id};
         decltype(featureT::id) unions{m_available_systems & ~feature.id};

         cout << "I =";
         for (unsignedInteger i{0}; i < intersections.size(); ++i)
            if (intersections[i])
               cout << " " << i + 1;
         cout << endl;
         cout << "U =";
         for (unsignedInteger i{0}; i < unions.size(); ++i)
            if (unions[i])
               cout << " " << i + 1;
         cout << endl;

         /////////////////////////////////////
         // INTERSECTIONS ////////////////////
         /////////////////////////////////////
         vector<unsignedInteger> intersection;
         vector<unsignedInteger> actual_intersections;
         bool first{true};
         for (unsignedInteger i{0}; i < intersections.size(); ++i) {
            if (intersections[i]) {
               if (first) {
                  intersection = system_to_features(i);
                  actual_intersections.push_back(i + 1);
                  first = false;
               } else {
                  vector<unsignedInteger> result;
                  actual_intersections.push_back(i + 1);
                  std::ranges::set_intersection(intersection,
                                                system_to_features(i),
                                                back_inserter(result));
                  if (result.size() == 1 &&
                      result.at(0) == feature_isolation::to_unsignedInteger(feature.id)) {
                     os << feature_isolation::to_unsignedInteger(feature.id) << "\t"
                        << feature.name << "\t"
                        << "{ " << feature.name << " }"
                        << "\t"
                        << "(";
                     for (bool first{true}; const auto& s : actual_intersections) {
                        if (first) {
                           first = false;
                           os << " S" << s;
                        } else {
                           os << " & S" << s;
                        }
                     }
                     os << " )" << std::endl;
                     cout << "I=======================" << endl;
                     return true;
                  }
                  print("I ", result);
                  intersection = result;
               }
               cout << "CI ";
               for (const auto& i : actual_intersections)
                  cout << " " << i;
               cout << endl;
            }
         }
         /////////////////////////////////////
         // DIFFERENCES //////////////////////
         /////////////////////////////////////
         vector<unsignedInteger> difference{intersection};
         vector<unsignedInteger> actual_differences;
         first = true;
         for (unsignedInteger i{0}; i < unions.size(); ++i) {
            if (unions[i]) {
               vector<unsignedInteger> result;
               actual_differences.push_back(i + 1);
               std::ranges::set_difference(difference,
                                           system_to_features(i),
                                           back_inserter(result));
               if (result.size() == 1 &&
                   result.at(0) == feature_isolation::to_unsignedInteger(feature.id)) {
                  os << feature_isolation::to_unsignedInteger(feature.id) << "\t"
                     << feature.name << "\t"
                     << "{ " << feature.name << " }"
                     << "\t"
                     << "(";
                  for (bool first{true}; const auto& s : actual_intersections) {
                     if (first) {
                        first = false;
                        os << " S" << s;
                     } else {
                        os << " & S" << s;
                     }
                  }
                  os << " ) \\ (";
                  for (bool first{true}; const auto& s : actual_differences) {
                     if (first) {
                        first = false;
                        os << " S" << s;
                     } else {
                        os << " | S" << s;
                     }
                  }
                  os << " )" << std::endl;
                  cout << "D=======================" << endl;
                  return true;
               }
               print("D ", result);
               difference = result;
               cout << "CU ";
               for (const auto& i : actual_differences)
                  cout << " " << i;
               cout << endl;
            }
         }
         os << feature_isolation::to_unsignedInteger(feature.id) << "\t"
            << feature.name << "\t";
         os << "{ ";
         for (bool first{true}; const auto& f_id : difference) {
            if (first) {
               first = false;
               os << " ";
            } else {
               os << ", ";
            }
            os << to_feature_name(f_id);
         }
         os << " }"
            << "\t";
         os << std::endl;
         cout << "=======================" << endl;
         return false;
      }
   };
} // namespace feature_isolation
#endif // FEATURE_ISOLATION_HPP
