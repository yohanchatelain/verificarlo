#ifndef __VERIFICARLO_SRLIB_UTILS_HPP__
#define __VERIFICARLO_SRLIB_UTILS_HPP__

#include <cstdint>

#include "debug.hpp"

template <typename T> struct IEEE754 {
  using I = std::conditional_t<std::is_same<T, float>::value, int32_t, int64_t>;
  using U =
      std::conditional_t<std::is_same<T, float>::value, uint32_t, uint64_t>;
  static constexpr U bias = std::numeric_limits<T>::max_exponent - 1;
  static constexpr U mantissa = std::numeric_limits<T>::digits - 1;
  static constexpr U exponent_mask =
      (std::numeric_limits<T>::max_exponent << 1) - 1;
};

// Implement other functions (get_exponent, predecessor, abs, pow2, etc.) using
// templates
// Emulating round-to-nearest ties-to-zero ”augmented” floating-point operations
// using round-to-nearest ties-to-even arithmetic Sylvie Boldo, Christoph Q.
// Lauter, Jean-Michel Muller ALGORITHM 4: MyulpH(𝑎): Computes sign(𝑎) ·
// pred(|𝑎|) and sign(𝑎) · ulp𝐻 (𝑎) for |𝑎| > 2𝑒min . Uses
// the FP constant 𝜓 = 1 − 2^{−𝑝} .
// 𝑧 ← RN𝑒 (𝜓𝑎)
// 𝛿 ← RN𝑒 (𝑎 − 𝑧)
template <typename T> T predecessor(T a) {
  T eps = std::is_same<T, float>::value ? 0x1.0p-23f : 0x1.0p-52;
  T z = std::round(a * eps);
  T delta = std::round(a - z);
  return z - delta;
}

// template <typename T> T predecessor(T a) {
//   using I = typename IEEE754<T>::I;
//   debug_start();
//   T pred = 0;
//   if (a == 0) {
//     pred = -std::numeric_limits<T>::denorm_min();
//   } else if (a == -std::numeric_limits<T>::infinity()) {
//     pred = a;
//   } else if (std::isnan(a)) {
//     pred = a;
//   } else {
//     auto i = reinterpret_cast<I &>(a);
//     debug_print("a = 0x%.16x\n", i);
//     i += (i > 0) ? -1 : 1;
//     pred = reinterpret_cast<T &>(i);
//   }
//   debug_print("predecessor(%.13a) = %.13a\n", a, pred);
//   debug_end();
//   return pred;
// }

template <typename T, typename U = typename IEEE754<T>::U> U get_exponent(T a) {
  debug_start();
  if (a == 0) {
    debug_end();
    return 0;
  }
  constexpr U bias = IEEE754<T>::bias;
  constexpr U mantissa = IEEE754<T>::mantissa;
  constexpr U exponent_mask = IEEE754<T>::exponent_mask;
  debug_print("bias = %d\n", bias);
  debug_print("mantissa = %d\n", mantissa);
  debug_print("exponent_mask = %d\n", exponent_mask);
  U exp = reinterpret_cast<U &>(a);
  debug_print("a = 0x%016x\n", exp);
  debug_print("raw exponent = %d\n", (exp >> mantissa) & exponent_mask);
  exp = ((exp >> mantissa) & exponent_mask) - bias;
  debug_print("get_exponent(%.13a) = %ld\n", a, exp);
  debug_end();
  return exp;
}

template <typename T, typename I> T pow2(I n) {
  T res = 1;
  if constexpr (std::is_same<T, float>::value) {
    float x = 1.0f;
    uint32_t i = *reinterpret_cast<uint32_t *>(&x);
    i += n << 23;
    res = *reinterpret_cast<float *>(&i);
  } else {
    double x = 1.0;
    uint64_t i = *reinterpret_cast<uint64_t *>(&x);
    i += static_cast<uint64_t>(n) << 52;
    res = *reinterpret_cast<double *>(&i);
  }
  debug_print("pow2(%d) = %.13a\n", n, res);
  return res;
}

// TODO: finish implementing this function
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

#endif // __VERIFICARLO_SRLIB_UTILS_HPP__
