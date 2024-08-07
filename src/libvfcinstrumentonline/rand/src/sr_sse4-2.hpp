#include <cmath>
#include <cstdint>
#include <immintrin.h>
#include <type_traits>

#include "rand.hpp"
#include "utils.hpp"
#include "xoroshiro256+.hpp"

#ifndef __SSE4_2__
#error "SSE4.2 is required for this file"
#endif

namespace sse4_2 {

typedef __m128d double2;
typedef __m128i short8;
typedef __m128i int4;
typedef __m128i long2;
typedef __m128 float4;

void debug_m128d(const char msg[], __m128d a) {
  std::cout << msg << ": ";
  for (int i = 0; i < sizeof(a) / sizeof(double); i++)
    std::cout << a[i] << " ";
  std::cout << std::endl;
}

void debug_m128i(const char msg[], __m128i a) {
  std::cout << msg << ": ";
  for (int i = 0; i < sizeof(a) / sizeof(int); i++)
    std::cout << a[i] << " ";
  std::cout << std::endl;
}

namespace doublex2 {

static inline double2 get_rand_double01() {
  return xoroshiro256plus::sse4_2::get_rand_double01(rng_state_x2);
}

static inline double2 get_predecessor_abs(double2 a) {
  const double2 phi = _mm_set1_pd(1.0 - 0x1.0p-53);
  return _mm_mul_pd(a, phi);
}

static inline double2 get_exponent(double2 a) {
  const double2 zero = _mm_setzero_pd();
  const long2 exp_mask = _mm_set1_epi64x(0x7ff0000000000000ULL);
  const long2 bias = _mm_set1_pd(1023);

  double2 is_zero = _mm_cmp_pd(a, zero, _CMP_EQ_OQ);

  // Extract exponent using floating-point operations
  long2 a_bits = _mm_castpd_si128(a);
  long2 exp_bits = _mm_and_pd(a_bits, exp_mask);
  exp_bits = _mm_srli_epi64(exp_bits, 52);

  // Subtract bias
  exp_bits = _mm_sub_pd(exp_bits, bias);

  // Blend result with zero for zero inputs
  return _mm_blendv_pd(exp_bits, zero, is_zero);
}

static inline double2 isnumber(double2 a, double2 b) {
  const double2 zero = _mm_setzero_pd();
  const double2 naninf_mask = _mm_set1_pd(0x7ff0000000000000);

  double2 a_not_zero = _mm_cmp_pd(a, zero, _CMP_NEQ_UQ);
  double2 b_not_zero = _mm_cmp_pd(b, zero, _CMP_NEQ_UQ);
  double2 a_not_naninf =
      _mm_cmp_pd(_mm_and_pd(a, naninf_mask), naninf_mask, _CMP_NEQ_UQ);
  double2 b_not_naninf =
      _mm_cmp_pd(_mm_and_pd(b, naninf_mask), naninf_mask, _CMP_NEQ_UQ);

  return _mm_and_pd(_mm_and_pd(a_not_zero, a_not_naninf),
                    _mm_and_pd(b_not_zero, b_not_naninf));
}

static inline int4 cmpgt(double2 a, double2 b) {
  return _mm_castpd_si128(_mm_cmp_pd(a, b, _CMP_GT_OQ));
}

static inline double2 pow2(long2 n) {
  const int mantissa = sr::utils::IEEE754<double>::mantissa;
  const int min_exponent = sr::utils::IEEE754<double>::min_exponent;

  const double2 zero = _mm_setzero_pd();
  const double2 one = _mm_set1_pd(1.0);

  long2 min_exp_vec = _mm_set1_epi64x(min_exponent);
  long2 one = _mm_set1_epi64x(1);
  long2 mantissa_vec = _mm_set1_epi64x(mantissa);

  long2 is_subnormal = _mm_cmpeq_epi64(min_exp_vec, n);
  long2 precision_loss = _mm_sub_epi64(min_exp_vec, n - 1);
  precision_loss = _mm_and_si128(precision_loss, is_subnormal);

  long2 n_adjusted = _mm_blendv_epi8(n, one, is_subnormal);
  double2 res = _mm_blendv_pd(one, zero, _mm_castsi128_pd(is_subnormal));

  long2 i = _mm_castpd_si128(res);
  long2 shift = _mm_sub_epi64(mantissa_vec, precision_loss);
  long2 to_add = _mm_sllv_epi64(n_adjusted, shift);
  i = _mm_add_epi64(i, to_add);
  return _mm_castsi128_pd(i);
}

static inline double2 abs(double2 a) {
  const long2 sign_mask = _mm_set1_epi64x(0x8000000000000000ULL);
  return _mm_andnot_pd(_mm_castsi128_pd(sign_mask), a);
}

static inline double2 sr_round(double2 sigma, double2 tau, double2 z) {
  const int _mantissa = sr::utils::IEEE754<double>::mantissa;
  const int _min_exponent = sr::utils::IEEE754<double>::min_exponent;

  const double2 zero = _mm_setzero_pd();
  const long2 mantissa = _mm_set1_epi64x(_mantissa);

  double2 tau_is_zero = _mm_cmp_pd(tau, zero, _CMP_EQ_OQ);
  double2 sign_tau = _mm_and_pd(tau, _mm_set1_pd(-0.0));
  double2 sign_sigma = _mm_and_pd(sigma, _mm_set1_pd(-0.0));
  double2 sign_diff = _mm_xor_pd(sign_tau, sign_sigma);

  double2 pred_sigma = get_predecessor_abs(sigma);
  long2 eta =
      _mm_blendv_pd(get_exponent(sigma), get_exponent(pred_sigma), sign_diff);

  double2 ulp = pow2(_mm_sub_epi64(eta, mantissa));
  ulp = _mm_blendv_pd(ulp, -ulp, tau_is_zero);
  double2 pi = ulp * z;
  double2 abs_ulp = abs(ulp);
  double2 abs_tau_plus_pi = abs(tau + pi);
  double2 round = _mm_blendv_pd(
      zero, sign_diff, _mm_cmp_pd(abs_tau_plus_pi, abs_ulp, _CMP_GE_OQ));
  return _mm_blendv_pd(round, sigma, tau_is_zero);
}

static inline double2 sr_add(double2 a, double2 b) {
  double2 is_number = isnumber(a, b);
  double2 normal_sum = a + b;
  double2 z = get_rand_double01();
  double2 tau, sigma, round;

  double2 tmp = a;
  a = _mm_min_pd(a, b);
  b = _mm_max_pd(tmp, b);

  sigma = normal_sum;
  double2 v = sigma - a;
  tau = b - v;

  round = sr_round(sigma, tau, z);
  return sigma + round;
}

static inline double2 sr_sub(double2 a, double2 b) { return sr_add(a, -b); }

static inline double2 sr_mul(double2 a, double2 b) {
  double2 is_number = isnumber(a, b);
  double2 normal_sum = a + b;
  double2 z = get_rand_double01();
  double2 tau, sigma, round;

  double2 z = get_rand_double01();
  double2 tau, sigma, round;
  twoproduct(a, b, sigma, tau);
  round = sr_round(sigma, tau, z);
  return sigma + round;
}

static inline double2 sr_div(double2 a, double2 b) {
  if (not isnumber(a, b)) {
    return a / b;
  }
  double2 z = get_rand_double01();
  double2 tau, sigma, round;
  twodiv(a, b, sigma, tau);
  round = sr_round(sigma, tau, z);
  return sigma + round;
}

} // namespace doublex2
} // namespace sse4_2
