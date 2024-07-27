#ifndef __VERIFICARLO_SRLIB_EFT_HPP__
#define __VERIFICARLO_SRLIB_EFT_HPP__

#include "debug.hpp"

template <typename T> void twosum(T a, T b, T *tau, T *sigma) {
  *sigma = a + b;
  T z = *sigma - a;
  *tau = (a - (*sigma - z)) + (b - z);
  debug_print("twosum(%.13a, %.13a) = %.13a, %.13a\n", a, b, *tau, *sigma);
}

template <typename T>
__attribute__((target("fma"))) void twoprodfma(T a, T b, T *tau, T *sigma) {
  *sigma = a * b;
  *tau = std::fma(a, b, -(*sigma));
  debug_print("twoprodfma(%.13a, %.13a) = %.13a, %.13a\n", a, b, *tau, *sigma);
}

#endif // __VERIFICARLO_SRLIB_EFT_HPP__