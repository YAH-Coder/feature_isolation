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

  using unsigned_integer          = std::uintmax_t;
  constexpr unsigned_integer zero = static_cast<unsigned_integer>(0);
  constexpr unsigned_integer one  = static_cast<unsigned_integer>(1);
  constexpr unsigned_integer two  = static_cast<unsigned_integer>(2);

  namespace checked_arithmetic
  {

    constexpr unsigned_integer add(unsigned_integer a, unsigned_integer b)
    {
      if (a > zero && b > std::numeric_limits<unsigned_integer>::max() - a)
      {
        throw std::overflow_error("Overflow in addition.");
      }
      return a + b;
    }

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

    constexpr unsigned_integer subtract(unsigned_integer a, unsigned_integer b)
    {
      if (a < b)
      {
        throw std::underflow_error("Underflow in subtraction.");
      }
      return a - b;
    }

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

    constexpr unsigned_integer multiply(unsigned_integer a, unsigned_integer b)
    {
      if (a > one && b > one
          && b > std::numeric_limits<unsigned_integer>::max() / a)
      {
        throw std::overflow_error("Overflow in multiplication.");
      }
      return a * b;
    }

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

  namespace unchecked_arithmetic
  {

    constexpr unsigned_integer add(unsigned_integer a, unsigned_integer b)
    {
      return a + b;
    }

    constexpr unsigned_integer add_assign(unsigned_integer& a,
                                          unsigned_integer  b)
    {
      a += b;
      return a;
    }

    constexpr unsigned_integer subtract(unsigned_integer a, unsigned_integer b)
    {
      return a - b;
    }

    constexpr unsigned_integer subtract_assign(unsigned_integer& a,
                                               unsigned_integer  b)
    {
      a -= b;
      return a;
    }

    constexpr unsigned_integer subtract_assign_return(unsigned_integer& a,
                                                      unsigned_integer  b)
    {
      a -= b;
      return (a + b);
    }

    constexpr unsigned_integer multiply(unsigned_integer a, unsigned_integer b)
    {
      return a * b;
    }

    constexpr unsigned_integer multiply_assign(unsigned_integer& a,
                                               unsigned_integer  b)
    {
      a *= b;
      return a;
    }

  } // namespace unchecked_arithmetic

#ifdef USE_UNCHECKED_ARITHMETIC
  namespace arithmetic = feature_isolation::unchecked_arithmetic;
#else
  namespace arithmetic = feature_isolation::checked_arithmetic;
#endif

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

  constexpr unsigned_integer power2(unsigned_integer exponent)
  {
    return power(two, exponent);
  }

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

  constexpr unsigned_integer factorial(unsigned_integer n)
  {
    return n == zero ? one : product(one, n);
  }

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

  // REFTACTORING REQUIRED: OF THE TWO FOLLOWING FUNCTIONS IMPLEMENT ONE BY
  // CALLING THE OTHER!
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

  constexpr unsigned_integer sum_of_combinations(unsigned_integer n,
                                                 unsigned_integer k)
  {
    unsigned_integer result { zero };
    for (unsigned_integer i = k; i <= n; arithmetic::add_assign(i, one))
    {
      arithmetic::add_assign(result, combinations(n, i));
    }
    return result;
  }

  template<class T> class combination
  {
    public:
      combination(const std::vector<T> symbols, std::size_t k)
          : m_symbols(symbols)
          , m_k(k)
      {
        if (k > symbols.size())
        {
          throw std::logic_error("n = " + std::to_string(symbols.size())
                                 + ", k = " + std::to_string(k)
                                 + " violates n >= k!\n");
        }
        initialize();
      }
      void initialize()
      {
        std::size_t start {};
        for (auto i { k() }; i > zero; --i)
        {
          m_state.push_back(start++);
        }
      }
      std::size_t k() const
      {
        return m_k;
      }
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
      std::vector<T>           m_symbols;
      std::vector<std::size_t> m_state;
      std::size_t              m_k;
  };

  /* static constexpr char tab{'\t'}; */

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

