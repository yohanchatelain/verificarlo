#ifndef __VERIFICARLO_SRLIB_UD_HPP__
#define __VERIFICARLO_SRLIB_UD_HPP__

#include "debug.hpp"
#include "eft.hpp"
#include "rand.hpp"
#include "utils.hpp"

template <typename T> T ud_round(T a) {
  debug_start();
  if (a == 0) {
    debug_end();
    return a;
  }
  debug_print("a = %.13a\n", a);
  using I = typename sr::utils::IEEE754<T>::I;
  I a_bits = *reinterpret_cast<I *>(&a);
  uint8_t rand = get_rand_uint1();
  debug_print("rand = 0x%02x\n", rand);
  a_bits += 1 - (rand * 2);
  debug_print("ud_round(%.13a)", a);
  a = *reinterpret_cast<T *>(&a_bits);
  debug_print(" = %.13a\n", a);
  debug_end();
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