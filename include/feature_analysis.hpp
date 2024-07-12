#ifndef FEATURE_ANALYSIS_HPP
#define FEATURE_ANALYSIS_HPP

#include "../include/fi_mathhelper.hpp"
#include <algorithm> // Because of iota() and find().
#include <array>
#include <climits>
#include <iostream>
#include <memory>  // Add this include
#include <numeric> // Because of CHAR_BIT.
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace feature_isolation
{
  constexpr bool MSB_LSB {
    true
  }; // Customization point for MSB at left-most position.
  constexpr unsigned_integer feature_digits { sizeof(unsigned_integer)
                                              * CHAR_BIT };

  using feature_t  = std::vector<bool>;
  using features_t = std::vector<feature_t>;
  using ids_t      = std::set<unsigned_integer>;

  template<typename T> void print(std::string m, const T& c);

  // feature_ids_t and system_ids_t are turned into their own types.
  // This has the advantage that the compiler recognizes when the use of
  // these types gets mixed up. The disadvantage is that list initialization
  // is no longer automatically available for these types, as they are no
  // longer plain old data types. But this does not currently cause any
  // problems in the program.
  struct feature_ids_t: ids_t
  { };
  struct system_ids_t: ids_t
  { };
  // using feature_ids_t = ids_t;
  // using system_ids_t = ids_t;

  std::string to_string(const feature_t& fd);

  constexpr unsigned_integer F { one };
  constexpr unsigned_integer N { feature_isolation::power2(1) };
  constexpr unsigned_integer A { feature_isolation::power2(2) };
  constexpr unsigned_integer O { feature_isolation::power2(3) };
  constexpr unsigned_integer FN { F + N };
  constexpr unsigned_integer AN { feature_isolation::power2(4) };
  constexpr unsigned_integer ON { feature_isolation::power2(5) };
  constexpr unsigned_integer ANF { feature_isolation::power2(6) };
  constexpr unsigned_integer ONF { feature_isolation::power2(7) };
  constexpr unsigned_integer FAO { F + A + O };
  constexpr unsigned_integer FAON { FAO + N };
  constexpr unsigned_integer FAONANON { FAON + AN + ON };
  constexpr unsigned_integer ALL { FAONANON + ANF + ONF };

  static const std::array<std::string, 8> feature_categories
      = { "F", "N", "A", "O", "AN", "ON", "ANF", "ONF" };

  [[nodiscard]] char separator(const char s = 0);

  class system_t
  {
    public:
      // clang-format off
      feature_ids_t F_feature_ids,
                    N_feature_ids,
                    A_feature_ids,
                    O_feature_ids,
                    AN_feature_ids,
                    ON_feature_ids,
                    ANF_feature_ids,
                    ONF_feature_ids;
      // clang-format on
      [[nodiscard]] unsigned_integer DF() const;
      [[nodiscard]] unsigned_integer T() const;
      [[nodiscard]] feature_ids_t    all_features() const;
  };

  using systems_t = std::vector<system_t>;

  class feature_analysis_t
  {
    public:
      explicit feature_analysis_t(unsigned_integer f,
                                  unsigned_integer max_rank,
                                  unsigned_integer feature_categories_
                                  = ::feature_isolation::ALL);

      using cui = const unsigned_integer;
      cui F, N, A, O, AN, ON, ANF, ONF, DF, T, R, C, S;
      // clang-format off
      const features_t F_features, 
                       N_features, 
                       A_features, 
                       O_features,
                       AN_features,
                       ON_features,
                       ANF_features, 
                       ONF_features;
      // clang-format on
      [[nodiscard]] feature_t at(unsigned_integer feature_id) const;
      [[nodiscard]] unsigned_integer
          category(unsigned_integer feature_id) const;
      [[nodiscard]] std::string
           category_name(unsigned_integer feature_id) const;
      void print_feature(std::ostream& os, const feature_t& feature) const;
      void print_features(std::ostream& os) const;
      void print_categories(std::ostream& os) const;
      void print_system_statistics(std::ostream&    os,
                                   const system_t&  system,
                                   unsigned_integer id) const;
      void print_systems_statistics(std::ostream& os) const;
      void print_system_features(std::ostream&    os,
                                 const system_t&  system,
                                 unsigned_integer system_id) const;
      void print_systems_features(std::ostream& os) const;
      void print(std::ostream& os) const;
    public:
      // clang-format off
      cui F_offset, 
          N_offset, 
          A_offset, 
          O_offset, 
          AN_offset, 
          ON_offset,
          ANF_offset,
          ONF_offset;
      // cang-format on
      std::array<cui, 8>               m_feature_offsets;
      std::array<const features_t*, 8> m_all_features;
    public:
      const systems_t m_systems;
    private:
      [[nodiscard]] features_t initialize_F_features() const;
      [[nodiscard]] features_t initialize_N_features() const;
      void and_combine(feature_t& f1, const feature_t& f2) const;
      [[nodiscard]] features_t initialize_A_features() const;
      void or_combine(feature_t& f1, const feature_t& f2) const;
      [[nodiscard]] features_t initialize_O_features() const;
      [[nodiscard]] features_t initialize_AN_features() const;
      [[nodiscard]] features_t initialize_ON_features() const;
      [[nodiscard]] features_t initialize_ANF_features() const;
      [[nodiscard]] features_t initialize_ONF_features() const;
      void                     insert_id(systems_t&       systems,
                                         const feature_t& feature,
                                         unsigned_integer feature_id,
                                         unsigned_integer category) const;
      [[nodiscard]] systems_t  initialize_systems() const;
  };

} // namespace feature_isolation

#endif // FEATURE_ANALYSIS_HPP

