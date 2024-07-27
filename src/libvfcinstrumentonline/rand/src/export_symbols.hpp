#ifndef __VERIFICARLO_SRLIB_EXPORT_SYMBOLS_HPP__
#define __VERIFICARLO_SRLIB_EXPORT_SYMBOLS_HPP__

#include <cstdint>
#include <type_traits>

// RNG functions
uint64_t _get_rand_uint64();
template <typename T> T get_rand_uint();
uint32_t get_rand_uint32_t();
uint64_t get_rand_uint64_t();

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, int64_t>::type
get_rand_float(T a);
float get_rand_float01();
double get_rand_double01();

// stochastic rounding functions
template <typename T> T sr_round(T sigma, T tau, T z);

template <typename T> T sr_add(T a, T b);
template <typename T> T sr_sub(T a, T b);
template <typename T> T sr_mul(T a, T b);
template <typename T> T __attribute__((target("fma"))) sr_div(T a, T b);
template <typename T> T __attribute__((target("fma,sse2"))) sr_sqrt(T a);

float sr_add_float(float a, float b);
float sr_sub_float(float a, float b);
float sr_mul_float(float a, float b);
float sr_div_float(float a, float b);
float sr_sqrt_float(float a);

double sr_add_double(double a, double b);
double sr_sub_double(double a, double b);
double sr_mul_double(double a, double b);
double sr_div_double(double a, double b);
double sr_sqrt_double(double a);

// up-down functions
template <typename T> T ud_round(T a);

template <typename T> T ud_add(T a, T b);
template <typename T> T ud_sub(T a, T b);
template <typename T> T ud_mul(T a, T b);
template <typename T> T ud_div(T a, T b);

float ud_add_float(float a, float b);
float ud_sub_float(float a, float b);
float ud_mul_float(float a, float b);
float ud_div_float(float a, float b);

double ud_add_double(double a, double b);
double ud_sub_double(double a, double b);
double ud_mul_double(double a, double b);
double ud_div_double(double a, double b);

// utils functions
template <typename T> T predecessor(T a);
template <typename T> int32_t get_exponent(T a);
template <typename T, typename I> T pow2(I n);
template <typename T> T add_round_odd(T a, T b);

// eft functions
template <typename T> void twosum(T a, T b, T *tau, T *sigma);
template <typename T>
__attribute__((target("fma"))) void twoprodfma(T a, T b, T *tau, T *sigma);

#endif // __VERIFICARLO_SRLIB_EXPORT_SYMBOLS_HPP__