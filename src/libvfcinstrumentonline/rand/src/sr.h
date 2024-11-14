#ifndef __VERFICARLO_SR_H__
#define __VERFICARLO_SR_H__

#include "debug.hpp"
#include "eft.hpp"
#include "utils.hpp"
#include "xoroshiro256+_hw.h"

namespace sr {
namespace scalar {

template <typename T> bool isnumber(T a, T b) {
  // fast check for a or b is not 0, inf or nan
  debug_start();
  using U = typename sr::utils::IEEE754<T>::U;
  constexpr auto naninf_mask = sr::utils::IEEE754<T>::inf_nan_mask;
  U *a_bits = reinterpret_cast<U *>(&a);
  U *b_bits = reinterpret_cast<U *>(&b);
  bool ret = ((a_bits != 0) and (((*a_bits) & naninf_mask) != naninf_mask)) and
             ((b_bits != 0) and (((*b_bits) & naninf_mask) != naninf_mask));
  debug_print("a_bits = 0x%016x\n", *a_bits);
  debug_print("b_bits = 0x%016x\n", *b_bits);
  debug_print("0x%016x & 0x%016x = 0x%016x\n", *a_bits, naninf_mask,
              *a_bits & naninf_mask);
  debug_print("0x%016x & 0x%016x = 0x%016x\n", *b_bits, naninf_mask,
              *b_bits & naninf_mask);
  debug_print("isnumber(%.13a, %.13a) = %d\n", a, b, ret);
  debug_end();
  return ret;
}

template <typename T> inline T sround(const T sigma, const T tau) {
  using namespace sr::utils;
  debug_start();
  if (tau == 0) {
    debug_end();
    return sigma;
  }
  constexpr int32_t mantissa = IEEE754<T>::mantissa;
  const bool sign_tau = tau < 0;
  const bool sign_sigma = sigma < 0;
  const int32_t eta = (sign_tau != sign_sigma)
                          ? get_exponent(get_predecessor_abs(sigma))
                          : get_exponent(sigma);
  const T ulp = (sign_tau ? -1 : 1) * pow2<T>(eta - mantissa);
  const T z = xoroshiro256plus::static_dispatch::uniform(T{});
  const T pi = ulp * z;
  const T round = (std::abs(tau + pi) >= std::abs(ulp)) ? ulp : 0;

  debug_print("z     = %+.13a\n", z);
  debug_print("sigma = %+.13a\n", sigma);
  debug_print("tau   = %+.13a\n", tau);
  debug_print("eta   = %d\n", eta);
  debug_print("pi    = %+.13a\n", pi);
  debug_print("tau+pi= %+.13a\n", tau + pi);
  debug_print("ulp   = %+.13a\n", ulp);
  debug_print("sr_round(%+.13a, %+.13a, %+.13a) = %+.13a\n", sigma, tau, z,
              round);
  debug_end();
  return round;
}

template <typename T> inline T add(T a, T b) {
  debug_start();
  if (not isnumber(a, b)) {
    debug_end();
    return a + b;
  }
  T tau, sigma, round;
  twosum(a, b, sigma, tau);
  round = sround(sigma, tau);
  debug_print("sr_add(%+.13a, %+.13a) = %+.13a + %+.13a\n", a, b, sigma, round);
  debug_end();
  return sigma + round;
}

template <typename T> inline T sub(T a, T b) { return add(a, -b); }

template <typename T> inline T mul(T a, T b) {
  debug_start();
  if (not isnumber(a, b)) {
    debug_end();
    return a * b;
  }
  T tau, sigma;
  twoprodfma(a, b, sigma, tau);
  const T round = sround(sigma, tau);
  debug_end();
  return sigma + round;
}

template <typename T> inline T div(T a, T b) {
  debug_start();
  if (not isnumber(a, b)) {
    debug_end();
    return a / b;
  }
  const T sigma = a / b;
  debug_print("sigma = %+.13a / %+.13a = %+.13a\n", a, b, sigma);

  const T tau = std::fma(-sigma, b, a) / b;

  debug_print("-sigma * b + a = %+.13a * %+.13a + %+.13a = %+.13a\n", -sigma, b,
              a, std::fma(-sigma, b, a));
  debug_print("tau = (-%+.13a * %+.13a + %+.13a) / %+.13a\n", -sigma, b, a, b);

  const T round = sround(sigma, tau);
  debug_print("sr_div(%+.13a, %+.13a) = %+.13a + %+.13a\n", a, b, sigma, tau);
  debug_end();
  return sigma + round;
}

template <typename T> inline T sqrt(T a) {
  if (not isnumber(a, a)) {
    return a;
  }

  T sigma;
#if defined(__SSE2__)
  if constexpr (std::is_same<T, float>::value) {
    // call assembly sqrtss
    asm("sqrtss %1, %0"
        : "=x"(sigma) // Output operand
        : "x"(a)      // Input operand
    );
  } else {
    // call assembly sqrtsd
    asm("sqrtsd %1, %0"
        : "=x"(sigma) // Output operand
        : "x"(a)      // Input operand
    );
  }
#else
  T sigma = std::sqrt(a);
#endif
  T tau = std::fma(-sigma, sigma, a) / (2 * sigma);
  T round = sround(sigma, tau);
  return sigma + round;
}

} // namespace scalar
} // namespace sr

#endif // HIGHWAY_HWY_SRLIB_RAND_SR_H_