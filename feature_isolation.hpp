#ifndef FEATURE_ISOLATION_HPP
#define FEATURE_ISOLATION_HPP

#include <bitset>
#include <climits>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace feature_isolation {
constexpr std::uintmax_t power(std::uintmax_t base, std::uintmax_t exponent) {
  std::uintmax_t result{1llu};
  while (exponent--) {
    result *= base;
  }
  return result;
}

constexpr std::uintmax_t power2(std::uintmax_t exponent) {
  return power(2ull, exponent);
}

constexpr std::uintmax_t product(std::uintmax_t from, std::uintmax_t to) {
  if (from == 0llu || to == 0llu)
    return 0llu;
  std::uintmax_t result{1llu};
  while (from <= to) {
    result *= to--;
  }
  return result;
}

constexpr std::uintmax_t factorial(std::uintmax_t n) {
  return n == 0llu ? 1llu : product(1llu, n);
}

constexpr std::uintmax_t combinations(std::uintmax_t n, std::uintmax_t k) {
  return product(n + 1llu - k, n) / factorial(k);
}

constexpr std::uintmax_t sum_of_combinations(std::uintmax_t n,
                                             std::uintmax_t k) {
  std::uintmax_t result{0llu};
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
