#ifndef __VERIFICARLO_SRLIB_SR_HPP_
#define __VERIFICARLO_SRLIB_SR_HPP_

#include "debug.hpp"
#include "eft.hpp"
#include "utils.hpp"

template <typename T> T sr_round(const T sigma, const T tau, const T z) {
  debug_start();
  constexpr T eps = IEEE754<T>::ulp;
  const bool sign_tau = tau < 0;
  const bool sign_sigma = sigma < 0;
  const uint32_t eta = (sign_tau != sign_sigma)
                           ? get_exponent(get_predecessor_abs(sigma))
                           : get_exponent(sigma);
  const T ulp = (sign_tau ? 1 : -1) * pow2<T>(eta) * eps;
  const T pi = ulp * z;
  const T round = (std::abs(tau + pi) >= std::abs(ulp)) ? ulp : 0;
  debug_print("eps = %.13a\n", eps);
  debug_print("eta = %u\n", eta);
  debug_print("ulp = %.13a\n", ulp);
  debug_print("pi = %.13a\n", pi);
  debug_print("sr_round(%.13a, %.13a, %.13a) = %.13a\n", sigma, tau, z, round);
  debug_end();
  return round;
}

template <typename T> bool isnumber(T a, T b) {
  // fast check for inf or nan {
  debug_start();
  const auto a_exp = get_unbiased_exponent(a);
  const auto b_exp = get_unbiased_exponent(b);
  constexpr auto exponent_mask = IEEE754<T>::exponent_mask;
  return (a_exp ^ exponent_mask) and (b_exp ^ exponent_mask);
}

template <typename T> T sr_add(T a, T b) {
  if (not isnumber(a, b)) {
    return a + b;
  }
  T z = get_rand_double01();
  T tau, sigma, round;
  twosum(a, b, sigma, tau);
  round = sr_round(sigma, tau, z);
  return sigma + round;
}

template <typename T> T sr_sub(T a, T b) { return sr_add(a, -b); }

template <typename T> T sr_mul(T a, T b) {
  if (not isnumber(a, b)) {
    return a * b;
  }
  const T z = get_rand_double01();
  T tau, sigma;
  twoprodfma(a, b, sigma, tau);
  const T round = sr_round(sigma, tau, z);
  return sigma + round;
}

template <typename T> T sr_div(T a, T b) {
  if (not isnumber(a, b)) {
    return a / b;
  }
  const T z = get_rand_double01();
  const T sigma = a / b;
  const T tau = std::fma(-sigma, b, a) / b;
  const T round = sr_round(sigma, tau, z);
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