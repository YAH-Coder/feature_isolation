#ifndef FI_MATHHELPER_HPP
#define FI_MATHHELPER_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace feature_isolation {

   using unsignedInteger = std::uintmax_t;
   const unsignedInteger zero = static_cast<unsignedInteger>(0);
   const unsignedInteger one = static_cast<unsignedInteger>(1);
   const unsignedInteger two = static_cast<unsignedInteger>(2);

   namespace checked_arithmetic {

      constexpr unsignedInteger add(unsignedInteger a, unsignedInteger b) {

         if (a > zero && b > std::numeric_limits<unsignedInteger>::max() - a) {
            throw std::overflow_error("Overflow in addition.");
         }
         return a + b;
      }

      constexpr unsignedInteger add_asign(unsignedInteger a, unsignedInteger b) {

         if (a > zero && b > std::numeric_limits<unsignedInteger>::max() - a) {
            throw std::overflow_error("Overflow in addition.");
         }
         return a += b;
      }

      constexpr unsignedInteger subtract(unsignedInteger a, unsignedInteger b) {

         if (b < zero && a > std::numeric_limits<unsignedInteger>::max() + b) {
            throw std::overflow_error("Overflow in subtraction.");
         }
         if (a < b) {
            throw std::underflow_error("Underflow in subtraction.");
         }
         return a - b;
      }

      constexpr unsignedInteger subtract_asign(unsignedInteger a, unsignedInteger b) {

         if (b < zero && a > std::numeric_limits<unsignedInteger>::max() + b) {
            throw std::overflow_error("Overflow in subtraction.");
         }
         if (a < b) {
            throw std::underflow_error("Underflow in subtraction.");
         }
         return a -= b;
      }

      constexpr unsignedInteger multiply(unsignedInteger a, unsignedInteger b) {

         if (a > one && b > one && b > std::numeric_limits<unsignedInteger>::max() / a) {
            throw std::overflow_error("Overflow in multiplication.");
         }
         return a * b;
      }

      constexpr unsignedInteger multiply_asign(unsignedInteger a, unsignedInteger b) {

         if (a > one && b > one && b > std::numeric_limits<unsignedInteger>::max() / a) {
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

#ifdef USE_UNCHECKED_ARITHMETIC
   namespace arithmetic = feature_isolation::unchecked_arithmetic;
#else
   namespace arithmetic = feature_isolation::checked_arithmetic;
#endif

   constexpr unsignedInteger power(unsignedInteger base,
                                   unsignedInteger exponent) {
      unsignedInteger result{one};
      while (arithmetic::subtract_asign(exponent, one)) {
         arithmetic::multiply_asign(result, base);
      }
      return result;
   }

   constexpr unsignedInteger power2(unsignedInteger exponent) {
      return power(two, exponent);
   }

   constexpr unsignedInteger product(unsignedInteger from, unsignedInteger to) {
      if (from == zero || to == zero)
         return zero;
      unsignedInteger result{one};
      while (from <= to) {
         arithmetic::multiply_asign(result, to--);
      }
      return result;
   }

   constexpr unsignedInteger factorial(unsignedInteger n) {
      return n == zero ? one : product(one, n);
   }

   constexpr unsignedInteger combinations(unsignedInteger n, unsignedInteger k) {
      return product(arithmetic::subtract(arithmetic::add(n, one), k), n) / factorial(k);
   }

   constexpr unsignedInteger sum_of_combinations(unsignedInteger n,
                                                 unsignedInteger k) {
      unsignedInteger result{zero};
      while (k <= n) {
         arithmetic::add_asign(result, combinations(n, arithmetic::add_asign(k, one)));
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
         for (auto i{k()}; i > zero; --i) {
            m_state.push_back(start++);
         }
      };
      std::size_t k() const { return m_k; }
      bool next() {
         for (auto i{m_state.size()}; i > zero; --i) {
            auto v{m_state.at(i - one)};
            if (v < m_symbols.size() && v + one + k() - i < m_symbols.size()) {
               m_state.at(i - one) += one;
               for (auto j{i}; j < m_state.size(); ++j) {
                  m_state.at(j) = m_state.at(j - one) + one;
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

#endif // FI_MATHHELPER_HPP
