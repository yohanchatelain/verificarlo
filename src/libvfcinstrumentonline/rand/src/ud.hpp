#ifndef __VERIFICARLO_SRLIB_UD_HPP__
#define __VERIFICARLO_SRLIB_UD_HPP__

#include "debug.hpp"
#include "eft.hpp"
#include "rand.hpp"

template <typename T> T ud_round(T a) {
  if (a == 0)
    return a;
  using IntType =
      typename std::conditional<sizeof(T) == 4, uint32_t, uint64_t>::type;
  IntType a_bits;
  std::memcpy(&a_bits, &a, sizeof(T));
  uint64_t rand = _get_rand_uint64();
  a_bits += (rand & 0x01) ? 1 : -1;
  std::memcpy(&a, &a_bits, sizeof(T));
  return a;
}

template <typename T> T ud_add(T a, T b) { return ud_round(a + b); }
template <typename T> T ud_sub(T a, T b) { return ud_add(a, -b); }
template <typename T> T ud_mul(T a, T b) { return ud_round(a * b); }
template <typename T> T ud_div(T a, T b) { return ud_round(a / b); }

float ud_add_float(float a, float b) { return ud_add<float>(a, b); }
float ud_sub_float(float a, float b) { return ud_sub<float>(a, b); }
float ud_mul_float(float a, float b) { return ud_mul<float>(a, b); }
float ud_div_float(float a, float b) { return ud_div<float>(a, b); }

double ud_add_double(double a, double b) { return ud_add<double>(a, b); }
double ud_sub_double(double a, double b) { return ud_sub<double>(a, b); }
double ud_mul_double(double a, double b) { return ud_mul<double>(a, b); }
double ud_div_double(double a, double b) { return ud_div<double>(a, b); }

#endif // __UD_HPP__