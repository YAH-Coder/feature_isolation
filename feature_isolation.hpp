#ifndef FEATURE_ISOLATION_HPP
#define FEATURE_ISOLATION_HPP

#include <bitset>
#include <climits>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace feature_isolation {

using unsignedInteger = std::uintmax_t;

namespace checked_arithmetic {

template <typename IntegerType>
constexpr IntegerType add(IntegerType a, IntegerType b) {

  static_assert(std::is_integral<IntegerType>::value,
                "IntegerType must be an integral type.");

  if (a > 0 && b > std::numeric_limits<IntegerType>::max() - a) {
    throw std::overflow_error("Overflow in addition.");
  }
  return a + b;
}

template <typename IntegerType>
constexpr IntegerType subtract(IntegerType a, IntegerType b) {

  static_assert(std::is_integral<IntegerType>::value,
                "IntegerType must be an integral type.");

  if (b < 0 && a > std::numeric_limits<IntegerType>::max() + b) {
    throw std::overflow_error("Overflow in subtraction.");
  }
  if (a < b) {
    throw std::underflow_error("Underflow in subtraction.");
  }
  return a - b;
}

template <typename IntegerType>
constexpr IntegerType multiply(IntegerType a, IntegerType b) {

  static_assert(std::is_integral<IntegerType>::value,
                "IntegerType must be an integral type.");

  if (a > 1 && b > 1 && b > std::numeric_limits<IntegerType>::max() / a) {
    throw std::overflow_error("Overflow in multiplication.");
  }
  return a * b;
}

} // namespace checked_arithmetic

namespace unchecked_arithmetic {

template <typename IntegerType>
constexpr IntegerType add(IntegerType a, IntegerType b) {
  return a + b;
}

template <typename IntegerType>
constexpr IntegerType subtract(IntegerType a, IntegerType b) {
  return a - b;
}

template <typename IntegerType>
constexpr IntegerType multiply(IntegerType a, IntegerType b) {
  return a * b;
}
} // namespace unchecked_arithmetic

constexpr cInteger power(cInteger base, cInteger exponent) {
  cInteger result{1llu};
  while (exponent--) {
    result *= base;
  }
  return result;
}

constexpr cInteger power2(cInteger exponent) { return power(2ull, exponent); }

constexpr cInteger product(cInteger from, cInteger to) {
  if (from == 0llu || to == 0llu)
    return 0llu;
  cInteger result{1llu};
  while (from <= to) {
    result *= to--;
  }
  return result;
}

constexpr cInteger factorial(cInteger n) {
  return n == 0llu ? 1llu : product(1llu, n);
}

constexpr cInteger combinations(cInteger n, cInteger k) {
  return product(n + 1llu - k, n) / factorial(k);
}

constexpr cInteger sum_of_combinations(cInteger n, cInteger k) {
  cInteger result{0llu};
  while (k <= n) {
    result += combinations(n, k++);
  }
  return result;
}

template <class T> class combinationT {
public:
  combinationT(const std::vector<T> symbols, std::size_t k)
      : m_symbols(symbols), m_k(k) {
    if (k > symbols.size()) {
      throw std::logic_error("n = " + std::to_string(symbols.size()) +
                             ", k = " + std::to_string(k) +
                             " violates n >= k!\n");
    }
    initialize();
  }
  void initialize() {
    std::size_t start{};
    for (auto i{k()}; i > 0; --i) {
      m_state.push_back(start++);
    }
  };
  std::size_t k() const { return m_k; }
  bool next() {
    for (auto i{m_state.size()}; i > 0; --i) {
      auto v{m_state.at(i - 1)};
      if (v < m_symbols.size() && v + 1 + k() - i < m_symbols.size()) {
        m_state.at(i - 1) += 1;
        for (auto j{i}; j < m_state.size(); ++j) {
          m_state.at(j) = m_state.at(j - 1) + 1;
        }
        return true;
      }
    }
    return false;
  }
  std::vector<T> operator()() const {
    std::vector<T> result;
    for (const auto &e : m_state) {
      result.push_back(m_symbols.at(e));
    }
    return result;
  }

private:
  std::vector<T> m_symbols;
  std::vector<std::size_t> m_state;
  std::size_t m_k;
};

static constexpr char tab{'\t'};

} // namespace feature_isolation
#endif // FEATURE_ISOLATION_HPP
