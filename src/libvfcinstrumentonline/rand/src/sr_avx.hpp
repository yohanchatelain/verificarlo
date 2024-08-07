#include <cmath>
#include <cstdint>
#include <immintrin.h>
#include <type_traits>

#include "rand.hpp"
#include "utils.hpp"

#ifndef __AVX__
#error "AVX is required for this file"
#endif

namespace avx {

typedef __m128d double2;
typedef __m128i short8;
typedef __m128i int4;
typedef __m128i long2;
typedef __m128 float4;

typedef __m256d double4;
typedef __m256i short16;
typedef __m256i long4;
typedef __m256i int8;
typedef __m256 float8;

void debug_m256d(const char msg[], __m256d a) {
  std::cout << msg << ": ";
  for (int i = 0; i < sizeof(a) / sizeof(double); i++)
    std::cout << a[i] << " ";
  std::cout << std::endl;
}

namespace x2double {

static inline double2 get_rand_double01() {
  return xoroshiro256plus::sse4_2::get_rand_double01(rng_state);
}

static inline double2 get_predecessor_abs(double2 a) {
  const __m128d phi = _mm_set1_pd(1.0 - 0x1.0p-53);
  return _mm_mul_pd(a, phi);
}

static inline long2 get_exponent(double2 a) {
  const double2 zero = _mm_setzero_pd();
  const long2 exp_mask = _mm_set1_epi64x(0x7ff0000000000000ULL);
  const long2 bias = _mm_set1_pd(1023);

  double2 is_zero = _mm_cmp_pd(a, zero, _CMP_EQ_OQ);

  // Extract exponent using floating-point operations
  long2 a_bits = _mm_castpd_si128(a);
  long2 exp_bits = _mm_and_pd(a_bits, exp_mask);
  exp_bits = _mm_srli_epi64(exp_bits, 52);

  // Subtract bias
  exp_bits = _mm_sub_epi64(exp_bits, bias);

  // Blend result with zero for zero inputs
  return _mm_blendv_pd(exp_bits, _mm_setzero_pd(), is_zero);
}

static inline double2 isnumber(double2 a, double2 b) {
  const long2 zero = _mm_setzero_si128();
  const long2 naninf_mask = _mm_set1_epi64x(0x7ff0000000000000);

  long2 a_int = _mm_castpd_si128(a);
  long2 b_int = _mm_castpd_si128(b);
  long2 a_not_zero = ~_mm_cmpeq_epi64(a_int, zero);
  long2 b_not_zero = ~_mm_cmpeq_epi64(b_int, zero);
  long2 a_not_naninf =
      ~_mm_cmpeq_epi64(_mm_and_si128(a, naninf_mask), naninf_mask);
  long2 b_not_naninf =
      ~_mm_cmpeq_epi64(_mm_and_si128(b, naninf_mask), naninf_mask);

  return _mm_and_si128(_mm_and_si128(a_not_zero, a_not_naninf),
                       _mm_and_si128(b_not_zero, b_not_naninf));
}

static inline long2 cmpgt(long2 a, long2 b) { return _mm_cmpgt_epi64(a, b); }

static inline double2 pow2(long2 n) {

  const int _mantissa = sr::utils::IEEE754<double>::mantissa;
  const int _min_exponent = sr::utils::IEEE754<double>::min_exponent;

  long2 min_exp_vec = _mm_set1_epi64x(_min_exponent);
  long2 one = _mm_set1_epi64x(1);
  long2 mantissa_vec = _mm_set1_epi64x(_mantissa);

  long2 is_subnormal = cmpgt(min_exp_vec, n);
  long2 precision_loss = _mm_sub_epi64(min_exp_vec, n - 1);
  precision_loss = _mm_and_si128(precision_loss, is_subnormal);

  long2 n_adjusted = _mm_blendv_epi8(n, one, is_subnormal);
  double2 res = _mm_blendv_pd(_mm_set1_pd(1.0), _mm_set1_pd(0.0),
                              _mm_castsi128_pd(is_subnormal));

  long2 i = _mm_castpd_si128(res);
  long2 shift = _mm_sub_epi64(mantissa_vec, precision_loss);
  long2 to_add = _mm_sllv_epi64(n_adjusted, shift);
  i = _mm_add_epi64(i, to_add);
  return _mm_castsi128_pd(i);
}

static inline double2 abs(double2 a) {
  const double2 sign_mask = _mm_set1_pd(-0.0);
  return _mm_andnot_pd(sign_mask, a);
}

static inline double2 sr_round(double2 sigma, double2 tau, double2 z) {
  const int _mantissa = sr::utils::IEEE754<double>::mantissa;
  const int _min_exponent = sr::utils::IEEE754<double>::min_exponent;

  const double2 zero = _mm_setzero_pd();
  const long2 mantissa = _mm_set1_epi64x(_mantissa);

  double2 tau_is_zero = _mm_cmp_pd(tau, zero, _CMP_EQ_OQ);
  double2 sign_tau = _mm_cmp_pd(tau, zero, _CMP_LT_OQ);
  double2 sign_sigma = _mm_cmp_pd(sigma, zero, _CMP_LT_OQ);
  double2 sign_diff = _mm_xor_pd(sign_tau, sign_sigma);

  double2 pred_sigma = get_predecessor_abs(sigma);
  long2 eta = get_exponent(sigma);
  long2 pred_eta = get_exponent(pred_sigma);
  eta = _mm_blendv_pd(eta, pred_eta, sign_diff);

  double2 ulp = pow2(_mm_sub_epi64(eta, mantissa));
  ulp = _mm_blendv_pd(ulp, _mm_sub_pd(zero, ulp), sign_tau);

  double2 pi = _mm_mul_pd(ulp, z);
  double2 abs_tau_plus_pi = abs(_mm_add_pd(tau, pi));
  double2 abs_ulp = abs(ulp);
  double2 round = _mm_blendv_pd(
      zero, ulp, _mm_cmp_pd(abs_tau_plus_pi, abs_ulp, _CMP_GE_OQ));

  return _mm_blendv_pd(round, sigma, tau_is_zero);
}

static inline double2 sr_add(double2 a, double2 b) {
  double2 is_number = isnumber(a, b);
  double2 normal_sum = _mm_add_pd(a, b);

  double2 z = get_rand_double01();
  double2 sigma, tau;

  double2 tmp = a;
  a = _mm_min_pd(a, b);
  b = _mm_max_pd(tmp, b);

  sigma = normal_sum;
  double2 v = _mm_sub_pd(sigma, a);
  tau = _mm_sub_pd(b, v);

  double2 round = sr_round(sigma, tau, z);
  double2 stochastic_sum = _mm_add_pd(sigma, round);

  return _mm_blendv_pd(normal_sum, stochastic_sum, is_number);
}

} // namespace x2double

namespace x4double {

static const double4 zero = _mm256_setzero_pd();
static const double4 one = _mm256_set1_pd(1.0);
static const long4 naninf_mask = _mm256_set1_epi64x(0x7ff0000000000000);
static const int _mantissa = sr::utils::IEEE754<double>::mantissa;
static const int _min_exponent = sr::utils::IEEE754<double>::min_exponent;

static inline double4 get_rand_double01() {
  return xoroshiro256plus::avx::get_rand_double01(rng_state);
}

static inline double4 get_predecessor_abs(double4 a) {
  const double4 phi = _mm256_set1_epi64x(1.0 - 0x1.0p-53);
  return _mm256_mul_pd(a, phi);
}

static inline long4 get_exponent(double4 a) {
  const long4 exp_mask = _mm256_set1_epi64x(0x7ff0000000000000ULL);
  const long4 bias = _mm256_set1_pd(1023);

  double4 is_zero = _mm256_cmp_pd(a, zero, _CMP_EQ_OQ);

  // Extract exponent using floating-point operations
  long4 a_bits = _mm256_castpd_si256(a);
  long4 exp_bits = _mm256_and_pd(a_bits, exp_mask);
  exp_bits = _mm256_srli_epi64(exp_bits, 52);

  // Subtract bias
  exp_bits = _mm256_sub_epi64(exp_bits, bias);

  // Blend result with zero for zero inputs
  return _mm256_blendv_pd(exp_bits, zero, is_zero);
}

static inline double4 isnumber(double4 a, double4 b) {

  long4 a_int = _mm256_castpd_si256(a);
  long4 b_int = _mm256_castpd_si256(b);
  long4 a_not_zero = ~_mm256_cmpeq_epi64(a_int, zero);
  long4 b_not_zero = ~_mm256_cmpeq_epi64(b_int, zero);
  long4 a_not_naninf =
      ~_mm256_cmpeq_epi64(_mm256_and_si256(a, naninf_mask), naninf_mask);
  long4 b_not_naninf =
      ~_mm256_cmpeq_epi64(_mm256_and_si256(b, naninf_mask), naninf_mask);

  return _mm256_and_si256(_mm256_and_si256(a_not_zero, a_not_naninf),
                          _mm256_and_si256(b_not_zero, b_not_naninf));
}

static inline long4 cmpgt(long4 a, long4 b) { return _mm256_cmpgt_epi64(a, b); }

static inline double4 pow2(long4 n) {

  long4 min_exp_vec = _mm256_set1_epi64x(_min_exponent);
  long4 one = _mm256_set1_epi64x(1);
  long4 mantissa_vec = _mm256_set1_epi64x(_mantissa);

  long4 is_subnormal = cmpgt(min_exp_vec, n);
  long4 precision_loss = _mm256_sub_epi64(min_exp_vec, n - 1);
  precision_loss = _mm256_and_si256(precision_loss, is_subnormal);

  long4 n_adjusted = _mm256_blendv_epi8(n, one, is_subnormal);
  double4 res = _mm256_blendv_pd(one, zero, _mm256_castsi256_pd(is_subnormal));

  long4 i = _mm256_castpd_si128(res);
  long4 shift = _mm_sub_epi64(mantissa_vec, precision_loss);
  long4 to_add = _mm_sllv_epi64(n_adjusted, shift);
  i = _mm_add_epi64(i, to_add);
  return _mm_castsi128_pd(i);
}

static inline double2 abs(double2 a) {
  const double2 sign_mask = _mm_set1_pd(-0.0);
  return _mm_andnot_pd(sign_mask, a);
}

static inline double2 sr_round(double2 sigma, double2 tau, double2 z) {
  const int _mantissa = sr::utils::IEEE754<double>::mantissa;
  const int _min_exponent = sr::utils::IEEE754<double>::min_exponent;

  const double2 zero = _mm_setzero_pd();
  const long2 mantissa = _mm_set1_epi64x(_mantissa);

  double2 tau_is_zero = _mm_cmp_pd(tau, zero, _CMP_EQ_OQ);
  double2 sign_tau = _mm_cmp_pd(tau, zero, _CMP_LT_OQ);
  double2 sign_sigma = _mm_cmp_pd(sigma, zero, _CMP_LT_OQ);
  double2 sign_diff = _mm_xor_pd(sign_tau, sign_sigma);

  double2 pred_sigma = get_predecessor_abs(sigma);
  long2 eta = get_exponent(sigma);
  long2 pred_eta = get_exponent(pred_sigma);
  eta = _mm_blendv_pd(eta, pred_eta, sign_diff);

  double2 ulp = pow2(_mm_sub_epi64(eta, mantissa));
  ulp = _mm_blendv_pd(ulp, _mm_sub_pd(zero, ulp), sign_tau);

  double2 pi = _mm_mul_pd(ulp, z);
  double2 abs_tau_plus_pi = abs(_mm_add_pd(tau, pi));
  double2 abs_ulp = abs(ulp);
  double2 round = _mm_blendv_pd(
      zero, ulp, _mm_cmp_pd(abs_tau_plus_pi, abs_ulp, _CMP_GE_OQ));

  return _mm_blendv_pd(round, sigma, tau_is_zero);
}

static inline double2 sr_add(double2 a, double2 b) {
  double2 is_number = isnumber(a, b);
  double2 normal_sum = _mm_add_pd(a, b);

  double2 z = get_rand_double01();
  double2 sigma, tau;

  double2 tmp = a;
  a = _mm_min_pd(a, b);
  b = _mm_max_pd(tmp, b);

  sigma = normal_sum;
  double2 v = _mm_sub_pd(sigma, a);
  tau = _mm_sub_pd(b, v);

  double2 round = sr_round(sigma, tau, z);
  double2 stochastic_sum = _mm_add_pd(sigma, round);

  return _mm_blendv_pd(normal_sum, stochastic_sum, is_number);
}

} // namespace x4double

// Helper function to get random doubles between 0 and 1
__m256d get_rand_double01_avx() {
  // Implementation depends on your random number generator
  // This is a placeholder
  return xoroshiro256plus::avx::get_rand_double01(rng_state_x4);
}

__m256d get_predecessor_abs_avx(__m256d a) {
  const __m256d phi = _mm256_set1_pd(1.0 - 0x1.0p-53);
  return _mm256_mul_pd(a, phi);
}

__m256d get_exponent_avx(__m256d a) {
  const __m256d zero = _mm256_setzero_pd();
  const __m256d exp_mask = _mm256_set1_epi64x(0x7ff0000000000000ULL);
  const __m256d bias = _mm256_set1_pd(1023.0);

  __m256d is_zero = _mm256_cmp_pd(a, zero, _CMP_EQ_OQ);

  // Extract exponent using floating-point operations
  __m256d exp_bits = _mm256_and_pd(a, exp_mask);

  // Use multiplication to effectively right-shift by 52 bits
  const __m256d shift_const = _mm256_set1_pd(1.0 / (1ULL << 52));
  __m256d exp = _mm256_mul_pd(exp_bits, shift_const);

  // Subtract bias
  exp = _mm256_sub_pd(exp, bias);

  // Blend result with zero for zero inputs
  return _mm256_blendv_pd(exp, zero, is_zero);
}

__m256d isnumber_avx(__m256d a, __m256d b) {
  const __m256d zero = _mm256_setzero_pd();
  const __m256d naninf_mask = _mm256_set1_pd(0x7ff0000000000000);

  __m256d a_not_zero = _mm256_cmp_pd(a, zero, _CMP_NEQ_UQ);
  __m256d b_not_zero = _mm256_cmp_pd(b, zero, _CMP_NEQ_UQ);
  __m256d a_not_naninf =
      _mm256_cmp_pd(_mm256_and_pd(a, naninf_mask), naninf_mask, _CMP_NEQ_UQ);
  __m256d b_not_naninf =
      _mm256_cmp_pd(_mm256_and_pd(b, naninf_mask), naninf_mask, _CMP_NEQ_UQ);

  return _mm256_and_pd(_mm256_and_pd(a_not_zero, a_not_naninf),
                       _mm256_and_pd(b_not_zero, b_not_naninf));
}

__m256i cmpgt(__m256d a, __m256d b) {
  return _mm256_castpd_si256(_mm256_cmp_pd(a, b, _CMP_GT_OQ));
}

__m256d pow2_avx(__m256i n) {
  const int mantissa = sr::utils::IEEE754<double>::mantissa;
  const int min_exponent = sr::utils::IEEE754<double>::min_exponent;

  __m256i min_exp_vec = _mm256_set1_epi64x(min_exponent);
  __m256i one = _mm256_set1_epi64x(1);
  __m256i mantissa_vec = _mm256_set1_epi64x(mantissa);

  __m256i is_subnormal = cmpgt(min_exp_vec, n);
  __m256i precision_loss = min_exp_vec - n;
  precision_loss = precision_loss - one;
  precision_loss = precision_loss & is_subnormal;

  __m256i n_adjusted = _mm256_blendv_pd(n, one, is_subnormal);

  __m256d res = _mm256_blendv_pd(_mm256_set1_pd(1.0), _mm256_set1_pd(0.0),
                                 _mm256_castsi256_pd(is_subnormal));

  __m256i i = _mm256_castpd_si256(res);
  __m256i shift = mantissa_vec - precision_loss;
  __m256i to_add = n_adjusted << shift;
  i += to_add;

  return _mm256_castsi256_pd(i);
}

__m256d abs_pd(__m256d a) {
  const __m256d sign_mask = _mm256_set1_pd(-0.0);
  return _mm256_andnot_pd(sign_mask, a);
}

__m256d sr_round_avx(__m256d sigma, __m256d tau, __m256d z) {
  const __m256d zero = _mm256_setzero_pd();
  const __m256d one = _mm256_set1_pd(1.0);
  const __m256d mantissa = _mm256_set1_pd(52);

  __m256d tau_is_zero = _mm256_cmp_pd(tau, zero, _CMP_EQ_OQ);
  __m256d sign_tau = _mm256_cmp_pd(tau, zero, _CMP_LT_OQ);
  __m256d sign_sigma = _mm256_cmp_pd(sigma, zero, _CMP_LT_OQ);
  __m256d sign_diff = _mm256_xor_pd(sign_tau, sign_sigma);

  __m256d pred_sigma = get_predecessor_abs_avx(sigma);
  __m256d eta = _mm256_blendv_pd(get_exponent_avx(sigma),
                                 get_exponent_avx(pred_sigma), sign_diff);

  __m256d ulp = pow2_avx(_mm256_sub_pd(eta, mantissa));
  ulp = _mm256_blendv_pd(ulp, _mm256_sub_pd(zero, ulp), sign_tau);

  __m256d pi = _mm256_mul_pd(ulp, z);
  __m256d abs_tau_plus_pi = abs_pd(_mm256_add_pd(tau, pi));
  __m256d abs_ulp = abs_pd(ulp);
  __m256d round = _mm256_blendv_pd(
      zero, ulp, _mm256_cmp_pd(abs_tau_plus_pi, abs_ulp, _CMP_GE_OQ));

  return _mm256_blendv_pd(round, sigma, tau_is_zero);
}

__m256d sr_add_avx(__m256d a, __m256d b) {
  __m256d is_number = isnumber_avx(a, b);
  __m256d normal_sum = _mm256_add_pd(a, b);

  __m256d z = get_rand_double01_avx();
  debug_m256d("z", z);
  __m256d sigma, tau;

  // Implement twosum using AVX instructions
  sigma = _mm256_add_pd(a, b);
  __m256d v = _mm256_sub_pd(sigma, a);
  tau = _mm256_sub_pd(b, v);

  __m256d round = sr_round_avx(sigma, tau, z);
  __m256d stochastic_sum = _mm256_add_pd(sigma, round);

  return _mm256_blendv_pd(normal_sum, stochastic_sum, is_number);
}

} // namespace avx
