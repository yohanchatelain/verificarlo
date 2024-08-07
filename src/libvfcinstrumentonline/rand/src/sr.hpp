#ifndef __VERIFICARLO_SRLIB_SR_HPP_
#define __VERIFICARLO_SRLIB_SR_HPP_

#include "debug.hpp"
#include "eft.hpp"
#include "rand.hpp"
#include "utils.hpp"

template <typename T> bool isnumber(T a, T b) {
  // fast check for a or b is not 0, inf or nan
  debug_start();
  using U = typename sr::utils::IEEE754<T>::U;
  constexpr auto naninf_mask = sr::utils::IEEE754<T>::inf_nan_mask;
  U a_bits = reinterpret_cast<U &>(a);
  U b_bits = reinterpret_cast<U &>(b);
  bool ret = ((a_bits != 0) and ((a_bits & naninf_mask) != naninf_mask)) and
             ((b_bits != 0) and ((b_bits & naninf_mask) != naninf_mask));
  debug_print("a_bits = 0x%016x\n", a_bits);
  debug_print("b_bits = 0x%016x\n", b_bits);
  debug_print("0x%016x & 0x%016x = 0x%016x\n", a_bits, naninf_mask,
              a_bits & naninf_mask);
  debug_print("0x%016x & 0x%016x = 0x%016x\n", b_bits, naninf_mask,
              b_bits & naninf_mask);
  debug_print("isnumber(%.13a, %.13a) = %d\n", a, b, ret);
  debug_end();
  return ret;
}

template <typename B, typename T> B isnumberx2(T a, T b);

template <>
scalar::boolx2_t isnumberx2(const scalar::floatx2_t &a,
                            const scalar::floatx2_t &b) {
  // fast check for a or b is not 0, inf or nan
  uint64_t naninf_mask = sr::utils::IEEE754<float>::inf_nan_mask;
  uint64_t naninf_mask_high = naninf_mask << 32;
  uint64_t naninf_mask_low = naninf_mask;

  bool ret_high =
      ((a.u32[0] != 0) and
       ((a.u32[0] & naninf_mask_high) != naninf_mask_high)) and
      ((b.u32[0] != 0) and ((b.u32[0] & naninf_mask_high) != naninf_mask_high));
  bool ret_low =
      ((a.u32[1] != 0) and
       ((a.u32[1] & naninf_mask_low) != naninf_mask_low)) and
      ((b.u32[1] != 0) and ((b.u32[1] & naninf_mask_low) != naninf_mask_low));
  return scalar::boolx2_t{.u32 = {ret_high, ret_low}};
}

template <>
sse4_2::boolx2_t isnumberx2(const sse4_2::doublex2_t &a,
                            const sse4_2::doublex2_t &b) {
  constexpr sse4_2::doublex2_t naninf_mask = {
      sr::utils::IEEE754<double>::inf_nan_mask,
      sr::utils::IEEE754<double>::inf_nan_mask};
  const auto a_neq_0 = sse4_2::neq0d(a);
  const auto b_neq_0 = sse4_2::neq0d(b);
  const auto naninf_a = sse4_2::bitwise_and(a, naninf_mask);
  const auto naninf_b = sse4_2::bitwise_and(b, naninf_mask);
  const auto a_neq_naninf = sse4_2::neqd(naninf_a, naninf_mask);
  const auto b_neq_naninf = sse4_2::neqd(naninf_b, naninf_mask);
  const auto neq0 = sse4_2::bitwise_and(a_neq_0, b_neq_0);
  const auto neq_naninf = sse4_2::bitwise_and(a_neq_naninf, b_neq_naninf);
  const auto ret = sse4_2::bitwise_and(neq0, neq_naninf);
  return ret;
}

template <typename T> T sr_round(const T sigma, const T tau, const T z) {
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

template <typename T> T sr_roundx2(const T sigma, const T tau, const T z) {
  using namespace sr::utils;
  if (tau == 0) {
    return sigma;
  }
  constexpr int32_t mantissa = IEEE754<T>::mantissa;
  const bool sign_tau = tau < 0;
  const bool sign_sigma = sigma < 0;
  const int32_t eta = (sign_tau != sign_sigma)
                          ? get_exponent(get_predecessor_abs(sigma))
                          : get_exponent(sigma);
  const T ulp = (sign_tau ? -1 : 1) * pow2<T>(eta - mantissa);
  const T pi = ulp * z;
  const T round = (std::abs(tau + pi) >= std::abs(ulp)) ? ulp : 0;
  return round;
}

// template <>
// scalar::floatx2_t sr_roundx2(const scalar::floatx2_t &sigma,
//                              const scalar::floatx2_t &tau,
//                              const scalar::floatx2_t &z) {
//   using T = scalar::floatx2_t;
//   if (sse4_2::not0(isnumberx2<sse4_2::boolx2_t, T>(sigma, tau))) {
//     return {sr_round(sigma.f[0], tau.f[0], z.f[0]),
//             sr_round(sigma.f[1], tau.f[1], z.f[1])};
//   }
//   constexpr int32_t mantissa = sr::utils::IEEE754<float>::mantissa;
//   const auto sign_tau = sse4_2::cmplt(tau, sse4_2::setzero());
//   const auto sign_sigma = sse4_2::cmplt(sigma, sse4_2::setzero());
//   const scalar::int32x2_t eta;
//   const auto sign_cmp = scalar::bitwise_xor(sign_tau, sign_sigma);
//   scalar::int32x2_t eta;
//   eta.u32[0] = sse4_2::blendv(
//       sse4_2::get_exponent(sse4_2::get_predecessor_abs(sigma.f[0])),
//       sse4_2::get_exponent(sigma.f[0]), sign_cmp.u32[0]);
//   eta.u32[1] = sse4_2::blendv(
//       sse4_2::get_exponent(sse4_2::get_predecessor_abs(sigma.f[1])),
//       sse4_2::get_exponent(sigma.f[1]), sign_cmp.u32[1]);
// }

template <typename T> T sr_add(T a, T b) {
  debug_start();
  if (not isnumber(a, b)) {
    debug_end();
    return a + b;
  }
  T z = get_rand_double01();
  T tau, sigma, round;
  twosum(a, b, sigma, tau);
  round = sr_round(sigma, tau, z);
  debug_print("sr_add(%+.13a, %+.13a) = %+.13a + %+.13a\n", a, b, sigma, round);
  debug_end();
  return sigma + round;
}

// template <typename T> T sr_addx2(T a, T b) {
//   if (sse4_2::not0(isnumberx2<sse4_2::boolx2_t, T>(a, b))) {
//     if constexpr (std::is_same<T, scalar::floatx2_t>::value) {
//       return {.f = {sr_add(a.f[0], b.f[0]), sr_add(a.f[1], b.f[1])}};
//     }
//     if constexpr (std::is_same<T, scalar::double_t>::value) {
//       return {.f = {sr_add(a.f[0], b.f[0]), sr_add(a.f[1], b.f[1])}};
//     }
//     return sse4_2::add(a, b);
//   }
//   T z = get_rand_double01_x2();
//   T tau, sigma, round;
//   twosumx2(a, b, sigma, tau);
//   round = sr_roundx2(sigma, tau, z);
//   return sigma + round;
// }

// template <>
// scalar::floatx2_t sr_addx2(scalar::floatx2_t a, scalar::floatx2_t b) {
//   using T = scalar::floatx2_t;
//   if (sse4_2::not0(isnumberx2<sse4_2::boolx2_t, T>(a, b))) {
//     return {.f = {sr_add(a.f[0], b.f[0]), sr_add(a.f[1], b.f[1])}};
//   }
//   T z = get_rand_float01_x2();
//   T tau, sigma, round;
//   twosumx2(a, b, sigma, tau);
//   round = sr_roundx2(sigma, tau, z);
//   return scalar::add(sigma, round);
// }

template <typename T> T sr_sub(T a, T b) { return sr_add(a, -b); }

template <typename T> T sr_mul(T a, T b) {
  debug_start();
  if (not isnumber(a, b)) {
    debug_end();
    return a * b;
  }
  const T z = get_rand_double01();
  T tau, sigma;
  twoprodfma(a, b, sigma, tau);
  const T round = sr_round(sigma, tau, z);
  debug_end();
  return sigma + round;
}

template <typename T> T sr_div(T a, T b) {
  debug_start();
  if (not isnumber(a, b)) {
    debug_end();
    return a / b;
  }
  const T z = get_rand_double01();
  const T sigma = a / b;
  debug_print("sigma = %+.13a / %+.13a = %+.13a\n", a, b, sigma);
  const T tau = std::fma(-sigma, b, a) / b;
  debug_print("-sigma * b + a = %+.13a * %+.13a + %+.13a = %+.13a\n", -sigma, b,
              a, std::fma(-sigma, b, a));
  debug_print("tau = (-%+.13a * %+.13a + %+.13a) / %+.13a\n", -sigma, b, a, b);
  const T round = sr_round(sigma, tau, z);
  debug_print("sr_div(%+.13a, %+.13a) = %+.13a + %+.13a\n", a, b, sigma, tau);
  debug_end();
  return sigma + round;
}

template <typename T> T sr_sqrt(T a) {
  if (not isnumber(a, a)) {
    return a;
  }
  T z = get_rand_double01();
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
  T round = sr_round(sigma, tau, z);
  return sigma + round;
}

// specializations for float and double
float sr_add_float(float a, float b) { return sr_add<float>(a, b); }
float sr_sub_float(float a, float b) { return sr_sub<float>(a, b); }
float sr_mul_float(float a, float b) { return sr_mul<float>(a, b); }
float sr_div_float(float a, float b) { return sr_div<float>(a, b); }
float sr_sqrt_float(float a) { return sr_sqrt<float>(a); }

double sr_add_double(double a, double b) { return sr_add<double>(a, b); }
double sr_sub_double(double a, double b) { return sr_sub<double>(a, b); }
double sr_mul_double(double a, double b) { return sr_mul<double>(a, b); }
double sr_div_double(double a, double b) { return sr_div<double>(a, b); }
double sr_sqrt_double(double a) { return sr_sqrt<double>(a); }

#endif // _VERIFICARLO_SRLIB_SR_HPP_