/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
/**
 * @file fi_mathhelper.hpp
 * @brief Low-level arithmetic and combinatorics helpers used by the feature-isolation modules.
 */
#ifndef FI_MATHHELPER_HPP
#define FI_MATHHELPER_HPP

#include <bitset>
#include <climits>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace feature_isolation
{

  /** @brief Unsigned integer type used for identifiers, counts, and combinatorial quantities. */
  using unsigned_integer = std::uintmax_t;
  /** @brief Canonical zero constant in the project-wide integer type. */
  constexpr unsigned_integer zero = static_cast<unsigned_integer>(0);
  /** @brief Canonical one constant in the project-wide integer type. */
  constexpr unsigned_integer one  = static_cast<unsigned_integer>(1);
  /** @brief Canonical two constant in the project-wide integer type. */
  constexpr unsigned_integer two  = static_cast<unsigned_integer>(2);

  /**
   * @brief Overflow-checked arithmetic primitives.
   *
   * These helpers throw instead of silently wrapping when the requested operation
   * cannot be represented in @ref unsigned_integer.
   */
  namespace checked_arithmetic
  {

    /** @brief Add two values with overflow checking. */
    constexpr unsigned_integer add(unsigned_integer a, unsigned_integer b)
    {
      if (a > zero && b > std::numeric_limits<unsigned_integer>::max() - a)
      {
        throw std::overflow_error("Overflow in addition.");
      }
      return a + b;
    }

    /** @brief Apply @p a += b with overflow checking and return the updated value. */
    constexpr unsigned_integer add_assign(unsigned_integer& a,
                                          unsigned_integer  b)
    {
      if (a > zero && b > std::numeric_limits<unsigned_integer>::max() - a)
      {
        throw std::overflow_error("Overflow in addition.");
      }
      a += b;
      return a;
    }

    /** @brief Subtract two values with underflow checking. */
    constexpr unsigned_integer subtract(unsigned_integer a, unsigned_integer b)
    {
      if (a < b)
      {
        throw std::underflow_error("Underflow in subtraction.");
      }
      return a - b;
    }

    /** @brief Apply @p a -= b with underflow checking and return the updated value. */
    constexpr unsigned_integer subtract_assign(unsigned_integer& a,
                                               unsigned_integer  b)
    {
      if (a < b)
      {
        throw std::underflow_error("Underflow in subtraction.");
      }
      a -= b;
      return a;
    }

    /** @brief Subtract in place and return the original value of @p a. */
    constexpr unsigned_integer subtract_assign_return(unsigned_integer& a,
                                                      unsigned_integer  b)
    {
      if (a < b)
      {
        throw std::underflow_error("Underflow in subtraction.");
      }
      a -= b;
      return (a + b);
    }

    /** @brief Multiply two values with overflow checking. */
    constexpr unsigned_integer multiply(unsigned_integer a, unsigned_integer b)
    {
      if (a > one && b > one
          && b > std::numeric_limits<unsigned_integer>::max() / a)
      {
        throw std::overflow_error("Overflow in multiplication.");
      }
      return a * b;
    }

    /** @brief Apply @p a *= b with overflow checking and return the updated value. */
    constexpr unsigned_integer multiply_assign(unsigned_integer& a,
                                               unsigned_integer  b)
    {
      if (a > one && b > one
          && b > std::numeric_limits<unsigned_integer>::max() / a)
      {
        throw std::overflow_error("Overflow in multiplication.");
      }
      a *= b;
      return a;
    }

  } // namespace checked_arithmetic

  /** @brief Arithmetic primitives without overflow or underflow checks. */
  namespace unchecked_arithmetic
  {

    /** @brief Return @p a + @p b without range checks. */
    constexpr unsigned_integer add(unsigned_integer a, unsigned_integer b)
    {
      return a + b;
    }

    /** @brief Apply @p a += b without range checks and return the updated value. */
    constexpr unsigned_integer add_assign(unsigned_integer& a,
                                          unsigned_integer  b)
    {
      a += b;
      return a;
    }

    /** @brief Return @p a - @p b without range checks. */
    constexpr unsigned_integer subtract(unsigned_integer a, unsigned_integer b)
    {
      return a - b;
    }

    /** @brief Apply @p a -= b without range checks and return the updated value. */
    constexpr unsigned_integer subtract_assign(unsigned_integer& a,
                                               unsigned_integer  b)
    {
      a -= b;
      return a;
    }

    /** @brief Subtract in place and return the original value of @p a without checks. */
    constexpr unsigned_integer subtract_assign_return(unsigned_integer& a,
                                                      unsigned_integer  b)
    {
      a -= b;
      return (a + b);
    }

    /** @brief Return @p a * @p b without range checks. */
    constexpr unsigned_integer multiply(unsigned_integer a, unsigned_integer b)
    {
      return a * b;
    }

    /** @brief Apply @p a *= b without range checks and return the updated value. */
    constexpr unsigned_integer multiply_assign(unsigned_integer& a,
                                               unsigned_integer  b)
    {
      a *= b;
      return a;
    }

  } // namespace unchecked_arithmetic

#ifdef USE_UNCHECKED_ARITHMETIC
  /** @brief Compile-time-selected arithmetic backend. */
  namespace arithmetic = feature_isolation::unchecked_arithmetic;
#else
  /** @brief Compile-time-selected arithmetic backend. */
  namespace arithmetic = feature_isolation::checked_arithmetic;
#endif

  /**
   * @brief Compute @p base raised to the non-negative @p exponent.
   * @throws std::overflow_error When the selected arithmetic backend is the checked backend
   * and an intermediate product exceeds @ref unsigned_integer.
   */
  constexpr unsigned_integer power(unsigned_integer base,
                                   unsigned_integer exponent)
  {
    unsigned_integer result { one };
    for (unsigned_integer i = 0; i < exponent; arithmetic::add_assign(i, one))
    {
      arithmetic::multiply_assign(result, base);
    }
    return result;
  }

  /** @brief Compute 2 raised to @p exponent. */
  constexpr unsigned_integer power2(unsigned_integer exponent)
  {
    return power(two, exponent);
  }

  /**
   * @brief Compute the product of the inclusive integer range [from, to].
   * @return Zero when either bound is zero or when @p from is larger than @p to.
   */
  constexpr unsigned_integer product(unsigned_integer from, unsigned_integer to)
  {
    if (from == zero || to == zero || (from > to))
    {
      return zero;
    }
    unsigned_integer result { one };
    while (from <= to)
    {
      arithmetic::multiply_assign(result,
                                  arithmetic::subtract_assign_return(to, one));
    }
    return result;
  }

  /**
   * @brief Compute the factorial @p n!.
   * @throws std::overflow_error When the selected arithmetic backend is the checked backend
   * and the result cannot be represented in @ref unsigned_integer.
   */
  constexpr unsigned_integer factorial(unsigned_integer n)
  {
    return n == zero ? one : product(one, n);
  }

  /**
   * @brief Compute the binomial coefficient "n choose k".
   * @pre @p k <= @p n.
   * @throws std::underflow_error When the selected arithmetic backend is checked and @p k > @p n.
   * @throws std::overflow_error When an intermediate product or factorial exceeds @ref unsigned_integer.
   */
  constexpr unsigned_integer combinations(unsigned_integer n,
                                          unsigned_integer k)
  {
    if (k == zero || n == k)
    {
      return one;
    }
    return (product(arithmetic::subtract(arithmetic::add(n, one), k), n)
            / factorial(k));
  }

  /**
   * @brief Sum binomial coefficients over the inclusive interval [lower_k, upper_k].
   * @return Zero when @p lower_k is larger than @p upper_k.
   * @throws std::underflow_error or std::overflow_error Propagated from @ref combinations when the
   * selected arithmetic backend rejects one of the requested coefficients.
   */
  constexpr unsigned_integer sum_of_combinations(unsigned_integer n,
                                                 unsigned_integer lower_k,
                                                 unsigned_integer upper_k)
  {
    unsigned_integer result { zero };
    for (unsigned_integer i = lower_k; i <= upper_k;
         arithmetic::add_assign(i, one))
    {
      arithmetic::add_assign(result, combinations(n, i));
    }
    return result;
  }

  /**
   * @brief Sum binomial coefficients from 0 through @p k.
   * @throws std::underflow_error or std::overflow_error Propagated from the three-argument overload.
   */
  constexpr unsigned_integer sum_of_combinations(unsigned_integer n,
                                                 unsigned_integer k)
  {
    return sum_of_combinations(n, 0, k);
  }

  /**
   * @brief Stateful helper that enumerates all size-@p k combinations of a symbol vector.
   * @tparam T Symbol type.
   */
  template<class T> class combination_t
  {
    public:
      /**
       * @brief Construct a combination enumerator.
       * @param symbols Source symbols.
       * @param k Combination size.
       * @throws std::logic_error If @p k exceeds the number of symbols.
       */
      combination_t(const std::vector<T> symbols, std::size_t k)
          : m_symbols(symbols)
          , m_k(k)
      {
        if (k > symbols.size())
        {
          throw std::logic_error("n = " + std::to_string(symbols.size())
                                 + ", k = " + std::to_string(k)
                                 + " violates n >= k!");
        }
        initialize();
      }

      /**
       * @brief Initialize the state to the lexicographically first combination.
       * @note The current implementation appends state and is therefore intended
       * to be used on a freshly constructed instance.
       */
      void initialize()
      {
        std::size_t start {};
        for (auto i { k() }; i > zero; --i)
        {
          m_state.push_back(start++);
        }
      }

      /** @brief Return the configured combination size. */
      std::size_t k() const
      {
        return m_k;
      }

      /** @brief Advance to the next combination and report whether one exists. */
      bool next()
      {
        for (auto i { m_state.size() }; i > zero; --i)
        {
          auto v { m_state.at(i - one) };
          if (v < m_symbols.size() && v + one + k() - i < m_symbols.size())
          {
            m_state.at(i - one) += one;
            for (auto j { i }; j < m_state.size(); ++j)
            {
              m_state.at(j) = m_state.at(j - one) + one;
            }
            return true;
          }
        }
        return false;
      }

      /** @brief Return the current combination as a materialized vector. */
      std::vector<T> operator() () const
      {
        std::vector<T> result;
        for (const auto& e : m_state)
        {
          result.push_back(m_symbols.at(e));
        }
        return result;
      }
    private:
      std::vector<T>           m_symbols; /**< Source symbol sequence. */
      std::vector<std::size_t> m_state;   /**< Current symbol indices. */
      std::size_t              m_k;       /**< Requested combination size. */
  };

  /* static constexpr char tab{'	'}; */

  /**
   * @brief Convert a bitset into @ref unsigned_integer.
   * @tparam N Number of bits in the bitset.
   * @param bits Bitset interpreted least-significant-bit first.
   * @return Integer representation of @p bits.
   */
  template<unsigned_integer N>
  unsigned_integer to_unsigned_integer(const std::bitset<N>& bits)
  {
    static_assert(N <= sizeof(unsigned_integer) * CHAR_BIT);
    unsigned_integer result { zero };
    for (std::uintmax_t n { zero }; n < N; ++n)
    {
      if (bits[n])
      {
        result |= one << n;
      }
    }
    return result;
  }

} // namespace feature_isolation

#endif // FI_MATHHELPER_HPP
