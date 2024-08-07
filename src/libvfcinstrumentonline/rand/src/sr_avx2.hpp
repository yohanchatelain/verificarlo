#include <cmath>
#include <cstdint>
#include <immintrin.h>
#include <type_traits>

#include "rand.hpp"
#include "utils.hpp"

#ifndef __AVX2__
#error "AVX2 is required for this file"
#endif

typedef __m256 float8;
typedef __m256i short16;
typedef __m256i int8;
typedef __m256i long4;
typedef __m256d double4;

static inline void debug(const char msg[]) { std::cerr << msg << std::endl; }

static inline void debug_m256d(const char msg[], double4 a) {
  std::cerr << std::hexfloat;
  std::cerr << msg << ": ";
  for (int i = 0; i < sizeof(a) / sizeof(double); i++)
    std::cerr << a[i] << " ";
  std::cerr << std::endl;
  std::cerr << std::defaultfloat;
}

static inline void debug_m256(const char msg[], float8 a) {
  std::cerr << std::hexfloat;
  std::cerr << msg << ": ";
  for (int i = 0; i < sizeof(a) / sizeof(float); i++)
    std::cerr << a[i] << " ";
  std::cerr << std::endl;
  std::cerr << std::defaultfloat;
}

static inline void debug_m256i(const char msg[], __m256i a, bool hex = true) {
  if (hex)
    std::cerr << std::hex;
  else
    std::cerr << std::dec;
  std::cerr << msg << ": ";
  for (int i = 0; i < sizeof(a) / sizeof(double); i++)
    std::cerr << a[i] << " ";
  std::cerr << std::endl;
  std::cerr << std::defaultfloat;
}
// Helper function to get random doubles between 0 and 1
static inline double4 get_rand_double01_avx2() {
  // Implementation depends on your random number generator
  // This is a placeholder
  return xoroshiro256plus::avx2::get_rand_double01(rng_state_x4);
}

static inline double4 get_predecessor_abs_avx2(double4 a) {
  const double4 phi = _mm256_set1_pd(1.0 - 0x1.0p-53);
  return _mm256_mul_pd(a, phi);
}

static inline float8 get_predecessor_abs_avx2f(float8 a) {
  const float8 phi = _mm256_set1_ps(1.0f - 0x1.0p-24f);
  return _mm256_mul_ps(a, phi);
}

static inline double4 get_exponent_avx2(double4 a) {
  debug(" [get_exponent]\tstart");
  const double4 zero = _mm256_setzero_pd();
  const __m256i exp_mask = _mm256_set1_epi64x(0x7ff0000000000000ULL);
  const __m256i bias = _mm256_set1_epi64x(1023);

  double4 is_zero = _mm256_cmp_pd(a, zero, _CMP_EQ_OQ);

  debug_m256d("\t\ta", a);
  __m256i a_bits = _mm256_castpd_si256(a);
  debug_m256i("\t\ta_bits", a_bits);
  __m256i exp_bits = _mm256_and_si256(a_bits, exp_mask);
  debug_m256i("\t\texp_bits", exp_bits);
  exp_bits = _mm256_srli_epi64(exp_bits, 52);
  debug_m256i("\t\texp_bits", exp_bits, false);

  exp_bits = _mm256_sub_epi64(exp_bits, bias);
  debug_m256i("\t\texp", exp_bits, false);

  debug(" [get_exponent]\tend");
  return _mm256_blendv_pd(exp_bits, zero, is_zero);
}

static inline float8 get_exponent_avx2f(float8 a) {
  debug(" [get_exponent]\tstart");
  const float8 zero = _mm256_setzero_ps();
  const __m256i exp_mask = _mm256_set1_epi32(0x7f800000);
  const __m256i bias = _mm256_set1_epi32(127);

  float8 is_zero = _mm256_cmp_ps(a, zero, _CMP_EQ_OQ);

  debug_m256("\t\ta", a);
  __m256i a_bits = _mm256_castps_si256(a);
  debug_m256i("\t\ta_bits", a_bits);
  __m256i exp_bits = _mm256_and_si256(a_bits, exp_mask);
  debug_m256i("\t\texp_bits", exp_bits);
  exp_bits = _mm256_srli_epi32(exp_bits, 23);
  debug_m256i("\t\texp_bits", exp_bits, false);

  exp_bits = exp_bits - bias;
  debug_m256i("\t\texp", exp_bits, false);

  debug(" [get_exponent]\tend");
  return _mm256_blendv_ps(exp_bits, zero, is_zero);
}

static inline double4 isnumber_avx2(double4 a, double4 b) {
  const double4 zero = _mm256_setzero_pd();
  const double4 naninf_mask = _mm256_set1_pd(0x7ff0000000000000);

  double4 a_not_zero = _mm256_cmp_pd(a, zero, _CMP_NEQ_UQ);
  double4 b_not_zero = _mm256_cmp_pd(b, zero, _CMP_NEQ_UQ);
  double4 a_not_naninf =
      _mm256_cmp_pd(_mm256_and_pd(a, naninf_mask), naninf_mask, _CMP_NEQ_UQ);
  double4 b_not_naninf =
      _mm256_cmp_pd(_mm256_and_pd(b, naninf_mask), naninf_mask, _CMP_NEQ_UQ);

  return _mm256_and_pd(_mm256_and_pd(a_not_zero, a_not_naninf),
                       _mm256_and_pd(b_not_zero, b_not_naninf));
}

static inline float8 isnumber_avx2f(float8 a, float8 b) {
  const float8 zero = _mm256_setzero_ps();
  const float8 naninf_mask = _mm256_set1_ps(0x7f800000);

  float8 a_not_zero = _mm256_cmp_ps(a, zero, _CMP_NEQ_UQ);
  float8 b_not_zero = _mm256_cmp_ps(b, zero, _CMP_NEQ_UQ);
  float8 a_not_naninf =
      _mm256_cmp_ps(_mm256_and_ps(a, naninf_mask), naninf_mask, _CMP_NEQ_UQ);
  float8 b_not_naninf =
      _mm256_cmp_ps(_mm256_and_ps(b, naninf_mask), naninf_mask, _CMP_NEQ_UQ);

  return _mm256_and_ps(_mm256_and_ps(a_not_zero, a_not_naninf),
                       _mm256_and_ps(b_not_zero, b_not_naninf));
}

// AVX2 version of cmpgt
static inline __m256i cmpgt(__m256i a, __m256i b) {
  return _mm256_cmpgt_epi64(a, b);
}

static inline __m256i cmpgtf(float8 a, float8 b) {
  return _mm256_cmpgt_epi32(a, b);
}

static inline double4 pow2_avx2(__m256i n) {
  debug(" [POW2]\tstart");
  debug_m256i("\t\tn", n, false);
  const int mantissa = sr::utils::IEEE754<double>::mantissa;
  const int min_exponent = sr::utils::IEEE754<double>::min_exponent;

  __m256i min_exp_vec = _mm256_set1_epi64x(min_exponent);
  __m256i one = _mm256_set1_epi64x(1);
  __m256i mantissa_vec = _mm256_set1_epi64x(mantissa);

  __m256i is_subnormal = cmpgt(min_exp_vec, n);
  __m256i precision_loss = _mm256_sub_epi64(min_exp_vec, n - 1);
  precision_loss = _mm256_and_si256(precision_loss, is_subnormal);
  debug_m256i("\t\tprecision_loss", precision_loss, false);

  __m256i n_adjusted = _mm256_blendv_epi8(n, one, is_subnormal);

  double4 res = _mm256_blendv_pd(_mm256_set1_pd(1.0), _mm256_set1_pd(0.0),
                                 _mm256_castsi256_pd(is_subnormal));

  __m256i i = _mm256_castpd_si256(res);
  __m256i shift = _mm256_sub_epi64(mantissa_vec, precision_loss);
  __m256i to_add = _mm256_sllv_epi64(n_adjusted, shift);
  debug_m256i("\t\tto_add", to_add);
  i = _mm256_add_epi64(i, to_add);

  debug_m256d("\t\tres", _mm256_castsi256_pd(i));
  debug(" [POW2]\tend");
  return _mm256_castsi256_pd(i);
}

static inline float8 pow2_avx2f(__m256i n) {
  debug(" [POW2]\tstart");
  debug_m256i("\t\tn", n, false);
  const int mantissa = sr::utils::IEEE754<float>::mantissa;
  const int min_exponent = sr::utils::IEEE754<float>::min_exponent;

  __m256i min_exp_vec = _mm256_set1_epi32(min_exponent);
  __m256i one = _mm256_set1_epi32(1);
  __m256i mantissa_vec = _mm256_set1_epi32(mantissa);

  __m256i is_subnormal = cmpgtf(min_exp_vec, n);
  __m256i precision_loss = _mm256_sub_epi32(min_exp_vec, n - 1);
  precision_loss = _mm256_and_si256(precision_loss, is_subnormal);
  debug_m256i("\t\tprecision_loss", precision_loss, false);

  __m256i n_adjusted = _mm256_blendv_epi8(n, one, is_subnormal);

  float8 res = _mm256_blendv_ps(_mm256_set1_ps(1.0), _mm256_set1_ps(0.0),
                                _mm256_castsi256_ps(is_subnormal));

  __m256i i = _mm256_castps_si256(res);
  __m256i shift = _mm256_sub_epi32(mantissa_vec, precision_loss);
  __m256i to_add = _mm256_sllv_epi32(n_adjusted, shift);
  debug_m256i("\t\tto_add", to_add);
  i = _mm256_add_epi32(i, to_add);

  debug_m256("\t\tres", _mm256_castsi256_ps(i));
  debug(" [POW2]\tend");
  return _mm256_castsi256_ps(i);
}

static inline double4 abs_pd(double4 a) {
  const double4 sign_mask = _mm256_set1_pd(-0.0);
  return _mm256_andnot_pd(sign_mask, a);
}

static inline float8 abs_ps(float8 a) {
  const float8 sign_mask = _mm256_set1_ps(-0.0f);
  return _mm256_andnot_ps(sign_mask, a);
}

static inline double4 sr_round_avx2(double4 sigma, double4 tau, double4 z) {
  debug(" [SR_ROUND]\tstart");
  debug_m256d("\tsigma ", sigma);
  debug_m256d("\ttau   ", tau);

  const int _mantissa = sr::utils::IEEE754<double>::mantissa;
  const int _min_exponent = sr::utils::IEEE754<double>::min_exponent;

  const double4 zero = _mm256_setzero_pd();
  const __m256i mantissa = _mm256_set1_epi64x(_mantissa);

  double4 tau_is_zero = _mm256_cmp_pd(tau, zero, _CMP_EQ_OQ);
  double4 sign_tau = _mm256_cmp_pd(tau, zero, _CMP_LT_OQ);
  double4 sign_sigma = _mm256_cmp_pd(sigma, zero, _CMP_LT_OQ);
  double4 sign_diff = _mm256_xor_pd(sign_tau, sign_sigma);

  double4 pred_sigma = get_predecessor_abs_avx2(sigma);
  __m256i eta = _mm256_blendv_pd(get_exponent_avx2(sigma),
                                 get_exponent_avx2(pred_sigma), sign_diff);

  debug_m256i("eta-mantissa", eta - mantissa, false);
  double4 ulp = pow2_avx2(_mm256_sub_epi64(eta, mantissa));
  debug_m256d("\tulp   ", ulp);
  ulp = _mm256_blendv_pd(ulp, _mm256_sub_pd(zero, ulp), sign_tau);

  double4 pi = _mm256_mul_pd(ulp, z);
  double4 abs_tau_plus_pi = abs_pd(_mm256_add_pd(tau, pi));
  double4 abs_ulp = abs_pd(ulp);
  double4 round = _mm256_blendv_pd(
      zero, ulp, _mm256_cmp_pd(abs_tau_plus_pi, abs_ulp, _CMP_GE_OQ));

  debug_m256d("\tz     ", z);
  debug_m256i("\teta   ", eta, false);
  debug_m256d("\tpi    ", pi);
  debug_m256d("\ttau+pi", _mm256_add_pd(tau, pi));
  debug_m256d("\tulp   ", ulp);
  debug_m256d("\tround ", round);
  debug_m256d("\t--", _mm256_setzero_pd());
  debug(" [SR_ROUND]\tend");
  return _mm256_blendv_pd(round, sigma, tau_is_zero);
}

static inline float8 sr_round_avx2f(float8 sigma, float8 tau, float8 z) {
  debug(" [SR_ROUND]\tstart");
  debug_m256("\tsigma ", sigma);
  debug_m256("\ttau   ", tau);

  const int _mantissa = sr::utils::IEEE754<float>::mantissa;
  const int _min_exponent = sr::utils::IEEE754<float>::min_exponent;

  const float8 zero = _mm256_setzero_ps();
  const __m256i mantissa = _mm256_set1_epi32(_mantissa);

  float8 tau_is_zero = _mm256_cmp_ps(tau, zero, _CMP_EQ_OQ);
  float8 sign_tau = _mm256_cmp_ps(tau, zero, _CMP_LT_OQ);
  float8 sign_sigma = _mm256_cmp_ps(sigma, zero, _CMP_LT_OQ);
  float8 sign_diff = _mm256_xor_ps(sign_tau, sign_sigma);

  float8 pred_sigma = get_predecessor_abs_avx2f(sigma);
  __m256i eta = _mm256_blendv_ps(get_exponent_avx2f(sigma),
                                 get_exponent_avx2f(pred_sigma), sign_diff);

  debug_m256i("eta-mantissa", eta - mantissa, false);
  float8 ulp = pow2_avx2f(eta - mantissa);
  debug_m256("\tulp   ", ulp);
  ulp = _mm256_blendv_ps(ulp, _mm256_sub_ps(zero, ulp), sign_tau);

  float8 pi = _mm256_mul_ps(ulp, z);
  float8 abs_tau_plus_pi = abs_ps(_mm256_add_ps(tau, pi));
  float8 abs_ulp = abs_ps(ulp);
  float8 round = _mm256_blendv_ps(
      zero, ulp, _mm256_cmp_ps(abs_tau_plus_pi, abs_ulp, _CMP_GE_OQ));

  debug_m256("\tz     ", z);
  debug_m256i("\teta   ", eta, false);
  debug_m256("\tpi    ", pi);
  debug_m256("\ttau+pi", _mm256_add_ps(tau, pi));
  debug_m256("\tulp   ", ulp);
  debug_m256("\tround ", round);
  debug_m256("\t--", _mm256_setzero_ps());
  debug(" [SR_ROUND]\tend");
  return _mm256_blendv_ps(round, sigma, tau_is_zero);
}

static inline double4 sr_add_avx2(double4 a, double4 b) {
  debug(" [SR_ADD]\t\tstart");
  double4 is_number = isnumber_avx2(a, b);
  double4 normal_sum = _mm256_add_pd(a, b);

  double4 z = get_rand_double01_avx2();
  debug_m256d("\t\tz", z);
  double4 sigma, tau;

  // Implement twosum using AVX instructions
  // swap a and b if |a| < |b|
  double4 swap = _mm256_cmp_pd(abs_pd(a), abs_pd(b), _CMP_LT_OQ);
  double4 tmp = a;
  a = _mm256_blendv_pd(a, b, swap);
  b = _mm256_blendv_pd(b, tmp, swap);

  sigma = _mm256_add_pd(a, b);
  double4 v = _mm256_sub_pd(sigma, a);
  tau = _mm256_sub_pd(b, v);

  double4 round = sr_round_avx2(sigma, tau, z);
  double4 stochastic_sum = _mm256_add_pd(sigma, round);

  debug_m256d("\t\tsigma", sigma);
  debug_m256d("\t\ttau  ", tau);
  debug_m256d("\t\tstochastic_sum", stochastic_sum);
  debug(" [SR_ADD]\t\tend");

  return _mm256_blendv_pd(normal_sum, stochastic_sum, is_number);
}

static inline double4 sr_sub_avx2(double4 a, double4 b) {
  b = _mm256_xor_pd(b, _mm256_set1_pd(-0.0));
  return sr_add_avx2(a, b);
}

static inline double4 sr_mul_avx2(double4 a, double4 b) {
  debug(" [SR_MUL]\t\tstart");
  double4 is_number = isnumber_avx2(a, b);
  double4 normal_mul = _mm256_mul_pd(a, b);
  double4 z = get_rand_double01_avx2();
  double4 sigma = normal_mul;
  double4 tau = _mm256_fmsub_pd(a, b, sigma);
  double4 round = sr_round_avx2(sigma, tau, z);
  double4 stochastic_mul = _mm256_add_pd(sigma, round);

  debug(" [SR_MUL]\t\tend");
  return _mm256_blendv_pd(normal_mul, stochastic_mul, is_number);
}

static inline double4 sr_div_avx2(double4 a, double4 b) {
  debug(" [SR_DIV]\t\tstart");
  double4 is_number = isnumber_avx2(a, b);
  double4 normal_div = _mm256_div_pd(a, b);
  double4 z = get_rand_double01_avx2();
  double4 sigma = normal_div;
  double4 tau =
      _mm256_fmadd_pd(_mm256_xor_pd(sigma, _mm256_set1_pd(-0.0)), b, a);
  double4 round = sr_round_avx2(sigma, tau, z);
  double4 stochastic_div = _mm256_add_pd(sigma, round);

  debug(" [SR_DIV]\t\tend");
  return _mm256_blendv_pd(normal_div, stochastic_div, is_number);
}