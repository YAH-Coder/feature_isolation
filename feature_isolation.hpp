#ifndef FEATURE_ISOLATION_HPP
#define FEATURE_ISOLATION_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace feature_isolation {

   using unsignedInteger = std::uintmax_t;

   namespace checked_arithmetic {

      constexpr unsignedInteger add(unsignedInteger a, unsignedInteger b) {

         if (a > 0 && b > std::numeric_limits<unsignedInteger>::max() - a) {
            throw std::overflow_error("Overflow in addition.");
         }
         return a + b;
      }

      constexpr unsignedInteger add_asign(unsignedInteger a, unsignedInteger b) {

         if (a > 0 && b > std::numeric_limits<unsignedInteger>::max() - a) {
            throw std::overflow_error("Overflow in addition.");
         }
         return a += b;
      }

      constexpr unsignedInteger subtract(unsignedInteger a, unsignedInteger b) {

         if (b < 0 && a > std::numeric_limits<unsignedInteger>::max() + b) {
            throw std::overflow_error("Overflow in subtraction.");
         }
         if (a < b) {
            throw std::underflow_error("Underflow in subtraction.");
         }
         return a - b;
      }

      constexpr unsignedInteger subtract_asign(unsignedInteger a, unsignedInteger b) {

         if (b < 0 && a > std::numeric_limits<unsignedInteger>::max() + b) {
            throw std::overflow_error("Overflow in subtraction.");
         }
         if (a < b) {
            throw std::underflow_error("Underflow in subtraction.");
         }
         return a -= b;
      }

      constexpr unsignedInteger multiply(unsignedInteger a, unsignedInteger b) {

         if (a > 1 && b > 1 && b > std::numeric_limits<unsignedInteger>::max() / a) {
            throw std::overflow_error("Overflow in multiplication.");
         }
         return a * b;
      }

      constexpr unsignedInteger multiply_asign(unsignedInteger a, unsignedInteger b) {

         if (a > 1 && b > 1 && b > std::numeric_limits<unsignedInteger>::max() / a) {
            throw std::overflow_error("Overflow in multiplication.");
         }
         return a *= b;
      }

   } // namespace checked_arithmetic

   namespace unchecked_arithmetic {

      constexpr unsignedInteger add(unsignedInteger a, unsignedInteger b) {
         return a + b;
      }

      constexpr unsignedInteger add_asign(unsignedInteger a, unsignedInteger b) {
         return a += b;
      }

      constexpr unsignedInteger subtract(unsignedInteger a, unsignedInteger b) {
         return a - b;
      }

      constexpr unsignedInteger subtract_asign(unsignedInteger a, unsignedInteger b) {
         return a -= b;
      }

      constexpr unsignedInteger multiply(unsignedInteger a, unsignedInteger b) {
         return a * b;
      }

      constexpr unsignedInteger multiply_asign(unsignedInteger a, unsignedInteger b) {
         return a *= b;
      }

   } // namespace unchecked_arithmetic

   constexpr unsignedInteger power(unsignedInteger base,
                                   unsignedInteger exponent) {
      unsignedInteger result{1llu};
      while (checked_arithmetic::subtract_asign(exponent, 1llu)) {
         checked_arithmetic::multiply_asign(result, base);
      }
      return result;
   }

   constexpr unsignedInteger power2(unsignedInteger exponent) {
      return power(2ull, exponent);
   }

   constexpr unsignedInteger product(unsignedInteger from, unsignedInteger to) {
      if (from == 0llu || to == 0llu)
         return 0llu;
      unsignedInteger result{1llu};
      while (from <= to) {
         checked_arithmetic::multiply_asign(result, to--);
      }
      return result;
   }

   constexpr unsignedInteger factorial(unsignedInteger n) {
      return n == 0llu ? 1llu : product(1llu, n);
   }

   constexpr unsignedInteger combinations(unsignedInteger n, unsignedInteger k) {
      return product(checked_arithmetic::subtract(checked_arithmetic::add(n, 1llu), k), n) / factorial(k);
   }

   constexpr unsignedInteger sum_of_combinations(unsignedInteger n,
                                                 unsignedInteger k) {
      unsignedInteger result{0llu};
      while (k <= n) {
         result += combinations(n, k++);
      }
      return result;
   }

   template <class T>
   class combinationT {
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
         for (const auto& e : m_state) {
            result.push_back(m_symbols.at(e));
         }
         return result;
      }

    private:
      std::vector<T> m_symbols;
      std::vector<std::size_t> m_state;
      std::size_t m_k;
   };

   /* static constexpr char tab{'\t'}; */

} // namespace feature_isolation
#endif // FEATURE_ISOLATION_HPP
