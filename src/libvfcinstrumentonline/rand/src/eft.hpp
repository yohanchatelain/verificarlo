#ifndef __VERIFICARLO_SRLIB_EFT_HPP__
#define __VERIFICARLO_SRLIB_EFT_HPP__

#include <cmath>

#include "debug.hpp"
#include "vector_types.hpp"

// fast two sum if |a| > |b|
// a and b are not nan or inf
template <typename T> void twosum(T a, T b, T &sigma, T &tau) {
  if (std::abs(a) < std::abs(b))
    std::swap(a, b);
  sigma = a + b;
  T z = sigma - a;
  tau = (a - (sigma - z)) + (b - z);
  debug_print("twosum(%.13a, %.13a) = %.13a, %.13a\n", a, b, sigma, tau);
}

// template <typename T> void twosumx2(T a, T b, T &sigma, T &tau) {
//   if (std::abs(a[0]) < std::abs(b[0]))
//     std::swap(a[0], b[0]);
//   if (std::abs(a[1]) < std::abs(b[1]))
//     std::swap(a[1], b[1]);
//   sigma = a + b;
//   T z = sigma - a;
//   tau = (a - (sigma - z)) + (b - z);
//   debug_print("twosumx2(%.13a, %.13a) = %.13a, %.13a\n", a, b, sigma, tau);
// }

template <typename T> void twoprodfma(T a, T b, T &sigma, T &tau) {
  sigma = a * b;
  tau = std::fma(a, b, -sigma);
  debug_print("twoprodfma(%.13a, %.13a) = %.13a, %.13a\n", a, b, sigma, tau);
}

#endif // __VERIFICARLO_SRLIB_EFT_HPP__