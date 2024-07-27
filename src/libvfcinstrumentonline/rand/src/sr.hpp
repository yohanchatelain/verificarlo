#ifndef __VERIFICARLO_SRLIB_SR_HPP_
#define __VERIFICARLO_SRLIB_SR_HPP_

#include "debug.hpp"
#include "eft.hpp"
#include "utils.hpp"

template <typename T> T sr_round(T sigma, T tau, T z) {
  debug_print("%s", "sr_round\n");
  T eps = std::is_same<T, float>::value ? 0x1.0p-23f : 0x1.0p-52;
  debug_print("eps = %.13a\n", eps);
  bool sign_tau = tau < 0;
  bool sign_sigma = sigma < 0;
  uint32_t eta;
  if (sign_tau != sign_sigma) {
    // eta = get_exponent(predecessor(std::abs(sigma)));
    eta = sigma * (1 - std::numeric_limits<T>::epsilon());
  } else {
    eta = get_exponent(sigma);
  }
  debug_print("eta = %u\n", eta);
  T ulp = (sign_tau ? 1 : -1) * pow2<T>(eta) * eps;
  debug_print("ulp = %.13a\n", ulp);
  T pi = ulp * z;
  debug_print("pi = %.13a\n", pi);
  T round;
  if (std::abs(tau + pi) >= std::abs(ulp)) {
    round = ulp;
  } else {
    round = 0;
  }
  debug_print("sr_round(%.13a, %.13a, %.13a) = %.13a\n", sigma, tau, z, round);
  return round;
}

template <typename T> T sr_add(T a, T b) {
  T z = get_rand_double01();
  T tau, sigma, round;
  twosum(a, b, &tau, &sigma);
  round = sr_round(sigma, tau, z);
  return sigma + round;
}

template <typename T> T sr_sub(T a, T b) { return sr_add(a, -b); }

template <typename T> T sr_mul(T a, T b) {
  T z = get_rand_double01();
  T tau, sigma, round;
  twoprodfma(a, b, &tau, &sigma);
  round = sr_round(sigma, tau, z);
  return sigma + round;
}

template <typename T> T __attribute__((target("fma"))) sr_div(T a, T b) {
  T z = get_rand_double01();
  T sigma = a / b;
  T tau = std::fma(-sigma, b, a) / b;
  T round = sr_round(sigma, tau, z);
  return sigma + round;
}

template <typename T> T __attribute__((target("fma,sse2"))) sr_sqrt(T a) {
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