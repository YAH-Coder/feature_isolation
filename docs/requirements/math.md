# Arithmetic and combinations

The public arithmetic helpers provide unsigned operations, counts, and finite selections used by the SPL analysis.

## MATH-001 — Unsigned arithmetic

The arithmetic interface shall expose `std::uintmax_t` operands and results. It shall use checked arithmetic by default. Where `USE_UNCHECKED_ARITHMETIC` is defined, the `arithmetic` namespace shall use unsigned wraparound.

If checked addition or multiplication overflows, then the operation shall raise `std::overflow_error`. If checked subtraction would produce a negative result, then the operation shall raise `std::underflow_error`. A failed checked assignment shall preserve its input.

When `add_assign`, `subtract_assign`, or `multiply_assign` succeeds, the operation shall update the supplied value and return the updated value. When `subtract_assign_return` succeeds, the operation shall update the supplied value and return its original value.

**Example — Assignment and range errors**

- **Given** the default checked arithmetic and the input values below.
- **When** the named operation is called.
- **Then** the return, exception, and updated value are as shown.

| Operation | Input | Expected outcome |
| --- | --- | --- |
| `add_assign(x, 3)` | `x = 2` | Returns `5`, stores `5`. |
| `subtract_assign_return(x, 3)` | `x = 5` | Returns `5`, stores `2`. |
| `add_assign(x, 1)` | `x` is the largest unsigned value. | `std::overflow_error`, original value preserved. |
| `subtract_assign(x, 1)` | `x = 0` | `std::underflow_error`, original value preserved. |

[Public arithmetic interface](../../include/fi_mathhelper.hpp#L24) · [Arithmetic tests](../../src/mathhelper_test.cpp#L123)

## MATH-002 — Counts

When a caller requests a count within the supported numeric domain, the arithmetic helpers shall return the result defined below.

| Operation | Required result |
| --- | --- |
| `power(base, exponent)`, `power2(exponent)` | Exponentiation with an unsigned exponent. An exponent of zero yields one. |
| `product(from, to)` | Product of the inclusive range. An endpoint of zero or an inverted range yields zero. |
| `factorial(n)` | The factorial of `n`, with `0!` equal to one. |
| `combinations(n, k)` | The binomial count for `0 ≤ k ≤ n`. Choosing none or all yields one. |
| `sum_of_combinations(n, lower, upper)` | Sum of binomial counts over the inclusive range. An inverted range yields zero. The two-argument form starts at zero. |

If a checked operation cannot return a count within its supported range, then the helper shall report the range failure through an exception.

**Example — Counts and boundary cases**

- **Given** calls to the public arithmetic helpers with checked arithmetic.
- **When** the calculations below are evaluated.
- **Then** they return the listed numbers.

| Calculation | Expected result |
| --- | ---: |
| `power2(3)` | `8` |
| `power(0, 0)` | `1` |
| `product(3, 5)` | `60` |
| `product(5, 3)` | `0` |
| `factorial(0)` | `1` |
| `combinations(5, 2)` | `10` |
| `sum_of_combinations(4, 1, 2)` | `10` |

**Current limitation.** Some large binomial counts raise a range exception even though the mathematical result fits in the public integer type. See [MATH-D02](#open-decisions). [Count operations](../../include/fi_mathhelper.hpp#L188) · [Count tests](../../src/mathhelper_test.cpp#L159)

## MATH-003 — Selection and bit values

When a caller requests selections of size `k` from a sequence, `combination_t` shall yield the first selection immediately and advance in lexicographic order of input positions. Repeated values in different positions shall be eligible independently. A size of zero shall yield one empty selection. After the last selection, `next()` shall return false.

If `k` exceeds the number of input positions, then the selector shall raise `std::logic_error`.

When a caller converts a bitset using `to_unsigned_integer`, the converter shall assign bit zero the least significant place value. If the width exceeds the unsigned integer type's width, then the interface shall reject the call at compile time.

**Example — Ordered selections**

- **Given** the sequence `[A, B, C]` and a selection size of `2`.
- **When** the caller reads the initial selection and advances until exhausted.
- **Then** the selections are `[A, B]`, `[A, C]`, and `[B, C]`, in that order.

**Example — Bit significance**

- **Given** a four-bit bitset with bits `0` and `2` set.
- **When** a caller converts it using `to_unsigned_integer`.
- **Then** the result is `5`.

[Combination interface](../../include/fi_mathhelper.hpp#L285) · [Bitset conversion](../../include/fi_mathhelper.hpp#L368)

## Open decisions

- **MATH-D01** Decide whether the public `initialize()` method should reset an existing selection. The interface describes initial selection on a fresh instance, and a second call changes the current selection. [Initialization contract](../../include/fi_mathhelper.hpp#L311)
- **MATH-D02** Determine whether checked count operations must succeed whenever the mathematical result fits in `std::uintmax_t`. The documented supported domain for `combinations(n, k)` requires `k ≤ n`. [Count precondition](../../include/fi_mathhelper.hpp#L239)
