#ifndef __VERIFICARLO_SRLIB_UTILS_HPP__
#define __VERIFICARLO_SRLIB_UTILS_HPP__

#include <cstdint>
#include <immintrin.h>
#include <type_traits>

#include "vector_types.hpp"

#include "debug.hpp"
#ifdef __clang__
typedef __float128 Float128;
#elif defined(__GNUC__) || defined(__GNUG__)
typedef _Float128 Float128;
#else
#error "Unsupported compiler. Please use GCC or Clang."
#endif

namespace sr::utils {

template <typename T> struct IEEE754 {};

// specialize IEEE754 for float and double

template <> struct IEEE754<float> {
  using I = int32_t;
  using U = uint32_t;
  static constexpr I sign = 1;
  static constexpr I exponent = 8;
  static constexpr I mantissa = 23;
  static constexpr I precision = 24;
  static constexpr float ulp = 0x1.0p-24f;
  static constexpr I bias = 127;
  static constexpr U exponent_mask = 0xFF;
  static constexpr float max_normal = 0x1.fffffep127f;
  static constexpr float min_normal = 0x1.0p-126f;
  static constexpr float min_subnormal = 0x1.0p-149f;
  static constexpr I min_exponent = -126;
  static constexpr I max_exponent = 127;
  static constexpr I min_exponent_subnormal = -149;
  static constexpr U inf_nan_mask = 0x7F800000;
};

template <> struct IEEE754<scalar::floatx2_t> {
  using F = scalar::floatx2_t;
  using I = scalar::int32x2_t;
  using U = scalar::uint32x2_t;
  static constexpr I sign = {.i32 = {1, 1}};
  static constexpr I exponent = {.i32 = {8, 8}};
  static constexpr I mantissa = {.i32 = {23, 23}};
  static constexpr I precision = {.i32 = {24, 24}};
  static constexpr F ulp = {.f = {0x1.0p-24f, 0x1.0p-24f}};
  static constexpr I bias = {.i32 = {127, 127}};
  static constexpr U exponent_mask = {.u32 = {0xFF, 0xFF}};
  static constexpr F max_normal = {.f = {0x1.fffffep127f, 0x1.fffffep127f}};
  static constexpr F min_normal = {.f = {0x1.0p-126f, 0x1.0p-126f}};
  static constexpr F min_subnormal = {.f = {0x1.0p-149f, 0x1.0p-149f}};
  static constexpr I min_exponent = {.i32 = {-126, -126}};
  static constexpr I max_exponent = {.i32 = {127, 127}};
  static constexpr I min_exponent_subnormal = {.i32 = {-149, -149}};
  static constexpr U inf_nan_mask = {.u32 = {0x7F800000, 0x7F800000}};
};

template <> struct IEEE754<double> {
  using I = int64_t;
  using U = uint64_t;
  static constexpr I sign = 1;
  static constexpr I exponent = 11;
  static constexpr I mantissa = 52;
  static constexpr I precision = 53;
  static constexpr double ulp = 0x1.0p-53;
  static constexpr I bias = 1023;
  static constexpr U exponent_mask = 0x7FF;
  static constexpr double max_normal = 0x1.fffffffffffffp1023;
  static constexpr double min_normal = 0x1.0p-1022;
  static constexpr double min_subnormal = 0x1.0p-1074;
  static constexpr I min_exponent = -1022;
  static constexpr I max_exponent = 1023;
  static constexpr I min_exponent_subnormal = -1074;
  static constexpr U inf_nan_mask = 0x7FF0000000000000;
};

template <> struct IEEE754<sse4_2::doublex2_t> {
  using F = sse4_2::doublex2_t;
  using I = sse4_2::int64x2_t;
  using U = sse4_2::uint64x2_t;
  static constexpr I sign = I{1, 1};
  static constexpr I exponent = I{11, 11};
  static constexpr I mantissa = I{52, 52};
  static constexpr I precision = I{53, 53};
  static constexpr F ulp = F{0x1.0p-53, 0x1.0p-53};
  static constexpr I bias = I{1023, 1023};
  static constexpr U exponent_mask = U{0x7FF, 0x7FF};
  static constexpr F max_normal =
      F{0x1.fffffffffffffp1023, 0x1.fffffffffffffp1023};
  static constexpr F min_normal = F{0x1.0p-1022, 0x1.0p-1022};
  static constexpr F min_subnormal = F{0x1.0p-1074, 0x1.0p-1074};
  static constexpr I min_exponent = I{-1022, -1022};
  static constexpr I max_exponent = I{1023, 1023};
  static constexpr I min_exponent_subnormal = I{-1074, -1074};
  static constexpr U inf_nan_mask = U{0x7FF0000000000000, 0x7FF0000000000000};
};

// Implement other functions (get_exponent, predecessor, abs, pow2, etc.) using
// templates
// "Emulating round-to-nearest ties-to-zero "augmented” floating-point
// operations using round-to-nearest ties-to-even arithmetic"
// Sylvie Boldo, Christoph Q. Lauter, Jean-Michel Muller
// ALGORITHM 4: MyulpH(𝑎): Computes
// sign(𝑎) · pred(|𝑎|) and sign(𝑎) · ulp𝐻 (𝑎) for |𝑎| > 2𝑒min .
// Uses the FP constant 𝜓 = 1 − 2^{−𝑝} .
// 𝑧 ← RN𝑒 (𝜓𝑎) (= pred(|a|))
// 𝛿 ← RN𝑒 (𝑎 − 𝑧)
// return 𝛿
template <typename T> T get_predecessor_abs(T a) {
  T phi = std::is_same<T, float>::value ? 1.0f - 0x1.0p-24f : 1.0 - 0x1.0p-53;
  T z = a * phi;
  return z;
}

template <> scalar::floatx2_t get_predecessor_abs(scalar::floatx2_t a) {
  scalar::floatx2_t phi = {.f = {1.0f - 0x1.0p-24f, 1.0f - 0x1.0p-24f}};
  scalar::floatx2_t z = scalar::mul(a, phi);
  return z;
}

template <typename T, typename U = typename IEEE754<T>::U>
U get_unbiased_exponent(T a) {
  if (a == 0) {
    return 0;
  }
  constexpr U mantissa = IEEE754<T>::mantissa;
  constexpr U exponent_mask = IEEE754<T>::exponent_mask;
  U exp = reinterpret_cast<U &>(a);
  exp = ((exp >> mantissa) & exponent_mask);
  return exp;
}

template <typename T, typename I = typename IEEE754<T>::I> I get_exponent(T a) {
  debug_start();
  if (a == 0) {
    debug_end();
    return 0;
  }
  constexpr I bias = IEEE754<T>::bias;
  constexpr I mantissa = IEEE754<T>::mantissa;
  constexpr I exponent_mask = IEEE754<T>::exponent_mask;
  debug_print("a = %+.13a\n", a);
  debug_print("bias = %d\n", bias);
  debug_print("mantissa = %d\n", mantissa);
  debug_print("exponent_mask = %d\n", exponent_mask);
  I exp = reinterpret_cast<I &>(a);
  debug_print("a = 0x%016x\n", exp);
  const auto raw_exp = (exp >> mantissa) & exponent_mask;
  debug_print("raw exponent = %d\n", raw_exp);
  exp = raw_exp - bias;
  debug_print("get_exponent(%.13a) = %d\n", a, exp);
  debug_end();
  return exp;
}

template <>
__attribute__((target("sse4.2,avx,avx2,avx512f"))) scalar::int32x2_t
get_exponent(scalar::floatx2_t a) {
  scalar::int32x2_t zero = {.u64 = 0};
  if (a.d == 0) {
    return zero;
  }
  int32_t mantissa = IEEE754<float>::mantissa;
  uint32_t exponent_mask = IEEE754<float>::exponent_mask;
  int32_t bias = IEEE754<float>::bias;
  a.u64 >>= mantissa;
  a.u32[0] &= exponent_mask - bias;
  a.u32[1] &= exponent_mask - bias;
  return a;
}

template <typename T> T pow2(int n) {
  // if n <= min_exponent, take into account precision loss due to subnormal
  // numbers
  using U = typename IEEE754<T>::I;
  constexpr auto mantissa = IEEE754<T>::mantissa;
  constexpr auto min_exponent = IEEE754<T>::min_exponent;

  const auto is_subnormal = n < min_exponent;
  const int precision_loss = (is_subnormal) ? min_exponent - n - 1 : 0;
  n = (is_subnormal) ? 1 : n;
  T res = (is_subnormal) ? 0 : 1;
  U i = *reinterpret_cast<U *>(&res);
  i += static_cast<U>(n) << (mantissa - precision_loss);
  res = *reinterpret_cast<T *>(&i);

  debug_print("pow2(%d) = %.13a\n", n, res);

  return res;
}

// TODO: finish to implement this function
template <typename T> T add_round_odd(T a, T b) {
  // return addition with rounding to odd
  // https://www.lri.fr/~melquion/doc/08-tc.pdf
  T x, e;
  twosum(a, b, &x, &e);
  return (e == 0 || *reinterpret_cast<T *>(&x) & 1) ? x : x + 1;
}

float predecessor_float(float a);
double predecessor_double(double a);

int32_t get_exponent_float(float a);
int64_t get_exponent_double(double a);

float pow2_float(int32_t n);
double pow2_double(int64_t n);

} // namespace sr::utils

#endif // __VERIFICARLO_SRLIB_UTILS_HPP__
