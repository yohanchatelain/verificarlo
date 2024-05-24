#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

// #include "interflop/common/float_utils.h"
#ifndef __FLOAT_STRUCT_H_
#define __FLOAT_STRUCT_H_

#include "interflop/common/float_const.h"
#include <stdint.h>
#include <stdlib.h>

/* import from <quadmath-imp.h> */

/* Frankly, if you have __float128, you have 64-bit integers, right?  */
#ifndef UINT64_C
#error "No way!"
#endif

/* Main union type we use to manipulate the floating-point type.  */

typedef union {
  __float128 f128;
  __uint128_t u128;
  __int128_t i128;

  /* Generic fields */
  __float128 type;
  __int128_t i;
  __uint128_t u;

  struct
#ifdef __MINGW32__
      /* On mingw targets the ms-bitfields option is active by default.
         Therefore enforce gnu-bitfield style.  */
      __attribute__((gcc_struct))
#endif
  {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    unsigned sign : QUAD_SIGN_SIZE;
    unsigned exponent : QUAD_EXP_SIZE;
    uint64_t mant_high : QUAD_HX_PMAN_SIZE;
    uint64_t mant_low : QUAD_LX_PMAN_SIZE;
#else
    uint64_t mant_low : QUAD_LX_PMAN_SIZE;
    uint64_t mant_high : QUAD_HX_PMAN_SIZE;
    unsigned exponent : QUAD_EXP_SIZE;
    unsigned sign : QUAD_SIGN_SIZE;
#endif
  } ieee;

  struct
#ifdef __MINGW32__
      /* On mingw targets the ms-bitfields option is active by default.
         Therefore enforce gnu-bitfield style.  */
      __attribute__((gcc_struct))
#endif
  {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    __uint128_t sign : QUAD_SIGN_SIZE;
    __uint128_t exponent : QUAD_EXP_SIZE;
    __uint128_t mantissa : QUAD_PMAN_SIZE;
#else
    __uint128_t mantissa : QUAD_PMAN_SIZE;
    __uint128_t exponent : QUAD_EXP_SIZE;
    __uint128_t sign : QUAD_SIGN_SIZE;
#endif
  } ieee128;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint64_t high;
    uint64_t low;
#else
    uint64_t low;
    uint64_t high;
#endif
  } words64;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint32_t w0;
    uint32_t w1;
    uint32_t w2;
    uint32_t w3;
#else
    uint32_t w3;
    uint32_t w2;
    uint32_t w1;
    uint32_t w0;
#endif
  } words32;

  struct
#ifdef __MINGW32__
      /* Make sure we are using gnu-style bitfield handling.  */
      __attribute__((gcc_struct))
#endif
  {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint64_t sign : QUAD_SIGN_SIZE;
    uint64_t exponent : QUAD_EXP_SIZE;
    uint64_t quiet_nan : QUAD_QUIET_NAN_SIZE;
    uint64_t mant_high : QUAD_HX_PMAN_QNAN_SIZE;
    uint64_t mant_low : QUAD_LX_PMAN_QNAN_SIZE;
#else
    uint64_t mant_low : QUAD_LX_PMAN_QNAN_SIZE;
    uint64_t mant_high : QUAD_HX_PMAN_QNAN_SIZE;
    uint64_t quiet_nan : QUAD_QUIET_NAN_SIZE;
    uint64_t exponent : QUAD_EXP_SIZE;
    uint64_t sign : QUAD_SIGN_SIZE;
#endif
  } nan;

} binary128;

typedef union {

  double f64;
  uint64_t u64;
  int64_t s64;
  uint32_t u32[2];

  /* Generic fields */
  double type;
  uint64_t u;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint64_t sign : DOUBLE_SIGN_SIZE;
    uint64_t exponent : DOUBLE_EXP_SIZE;
    uint64_t mantissa : DOUBLE_PMAN_SIZE;
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint64_t mantissa : DOUBLE_PMAN_SIZE;
    uint64_t exponent : DOUBLE_EXP_SIZE;
    uint64_t sign : DOUBLE_SIGN_SIZE;
#endif
  } ieee;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint32_t sign : DOUBLE_SIGN_SIZE;
    uint32_t exponent : DOUBLE_EXP_SIZE;
    uint32_t mantissa_high : DOUBLE_PMAN_HIGH_SIZE;
    uint32_t mantissa_low : DOUBLE_PMAN_LOW_SIZE;
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#if __FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__
    uint32_t mantissa_high : DOUBLE_PMAN_HIGH_SIZE;
    uint32_t exponent : DOUBLE_EXP_SIZE;
    uint32_t sign : DOUBLE_SIGN_SIZE;
    uint32_t mantissa_low : DOUBLE_PMAN_LOW_SIZE;
#else
    uint32_t mantissa_low : DOUBLE_PMAN_LOW_SIZE;
    uint32_t mantissa_high : DOUBLE_PMAN_HIGH_SIZE;
    uint32_t exponent : DOUBLE_EXP_SIZE;
    uint32_t sign : DOUBLE_SIGN_SIZE;
#endif
#endif
  } ieee32;

} binary64;

typedef union {

  float f32;
  uint32_t u32;
  int32_t s32;

  /* Generic fields */
  float type;
  uint32_t u;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint32_t sign : FLOAT_SIGN_SIZE;
    uint32_t exponent : FLOAT_EXP_SIZE;
    uint32_t mantissa : FLOAT_PMAN_SIZE;
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint32_t mantissa : FLOAT_PMAN_SIZE;
    uint32_t exponent : FLOAT_EXP_SIZE;
    uint32_t sign : FLOAT_SIGN_SIZE;
#endif
  } ieee;

} binary32;

#define QUADFP_NAN 0
#define QUADFP_INFINITE 1
#define QUADFP_ZERO 2
#define QUADFP_SUBNORMAL 3
#define QUADFP_NORMAL 4
#define fpclassifyq(x)                                                         \
  __builtin_fpclassify(QUADFP_NAN, QUADFP_INFINITE, QUADFP_NORMAL,             \
                       QUADFP_SUBNORMAL, QUADFP_ZERO, x)

#endif /* __FLOAT_STRUCT_H_ */

#ifdef DEBUG
#include <stdio.h>
#define debug_print(fmt, ...)                                                  \
  do {                                                                         \
    fprintf(stderr, fmt, __VA_ARGS__);                                         \
  } while (0)
#else
#define debug_print(fmt, ...)                                                  \
  do {                                                                         \
  } while (0)
#endif

#define __INTERNAL_RNG_STATE xoroshiro_state

typedef uint64_t xoroshiro_state[2];

static __thread xoroshiro_state rng_state;
static pid_t global_tid = 0;
static bool already_initialized = 0;

uint64_t next_seed(uint64_t seed_state) {
  uint64_t z = (seed_state += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

#define rotl(x, k) ((x) << (k)) | ((x) >> (64 - (k)))

uint64_t get_rand_uint64(void) {
  const uint64_t s0 = rng_state[0];
  uint64_t s1 = rng_state[1];
  const uint64_t result = rotl(s0 + s1, 17) + s0;

  s1 ^= s0;
  rng_state[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21); // a, b
  rng_state[1] = rotl(s1, 28);                   // c

  return result;
}

int32_t get_rand_float(float a) {
  const uint64_t half_max_int = (uint64_t)(UINT64_MAX / 2);
  if (a == 0)
    return 0;
  return (get_rand_uint64() < half_max_int) ? 1 : -1;
}

int64_t get_rand_double(double a) {
  const uint64_t half_max_int = (uint64_t)(UINT64_MAX / 2);
  if (a == 0)
    return 0;
  return (get_rand_uint64() < half_max_int) ? 1 : -1;
}

static inline double get_rand_double01() {
  uint64_t x = get_rand_uint64();
  const union {
    uint64_t i;
    double d;
  } u = {.i = UINT64_C(0x3FF) << 52 | x >> 12};
  return u.d - 1.0;
}

__attribute__((constructor)) static void init(void) {
  if (already_initialized == 0) {
    already_initialized = 1;
  } else {
    return;
  }
  uint64_t seed = 0;
  struct timeval t1;
  gettimeofday(&t1, NULL);
  seed = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);
  rng_state[0] = next_seed(seed);
  rng_state[1] = next_seed(seed);
}

/* Returns the unbiased exponent of the binary32 f */
static inline int32_t _get_exponent_binary32(const float f) {
  binary32 x = {.f32 = f};
  /* Substracts the bias */
  return x.ieee.exponent - FLOAT_EXP_COMP;
}

/* Returns the unbiased exponent of the binary64 d */
static inline int32_t _get_exponent_binary64(const double d) {
  binary64 x = {.f64 = d};
  /* Substracts the bias */
  return x.ieee.exponent - DOUBLE_EXP_COMP;
}

uint32_t get_exponent_b64(double x) {
  debug_print("%s\n", "=== get_exponent_b64 ===");
  debug_print("\tx: %+.13a\n", x);
  debug_print("\texp: %d\n", _get_exponent_binary64(x));
  debug_print("%s\n", "=== get_exponent_b64 ===");
  return _get_exponent_binary64(x);
}
uint32_t get_exponent_b32(float x) {
  debug_print("%s\n", "=== get_exponent_b32 ===");
  debug_print("\tx: %+.13a\n", x);
  debug_print("\texp: %d\n", _get_exponent_binary32(x));
  debug_print("%s\n", "=== get_exponent_b32 ===");

  return _get_exponent_binary32(x);
}

double predecessor_b64(double x) {
  debug_print("%s\n", "=== predecessor_b64 ===");
  debug_print("\tx:      %+.13a\n", x);
  uint64_t x_bits = *(uint64_t *)&x;
  x_bits--;
  debug_print("\tpre(x): %+.13a\n", *(double *)&x_bits);
  debug_print("%s\n", "=== predecessor_b64 ===");
  return *(double *)&x_bits;
}

double _predecessor_b64(double x) { return __builtin_nextafter(x, -INFINITY); }
float _predecessor_b32(float x) { return __builtin_nextafterf(x, -INFINITY); }

double _successor_b64(double x) { return __builtin_nextafter(x, INFINITY); }
float _successor_b32(float x) { return __builtin_nextafterf(x, INFINITY); }

float predecessor_b32(float x) {
  debug_print("%s\n", "=== predecessor_b32 ===");
  debug_print("\tx:      %+.13a\n", x);
  uint32_t x_bits = *(uint32_t *)&x;
  x_bits--;
  debug_print("\tpre(x): %+.13a\n", *(float *)&x_bits);
  debug_print("%s\n", "=== predecessor_b32 ===");
  return *(float *)&x_bits;
}

double abs_b64(double x) {
  debug_print("%s\n", "=== abs_b64 ===");
  debug_print("\tx:      %+.13a\n", x);
  uint64_t x_bits = *(uint64_t *)&x;
  x_bits &= 0x7FFFFFFFFFFFFFFF;
  debug_print("\tabs(x): %+.13a\n", *(double *)&x_bits);
  debug_print("%s\n", "=== abs_b64 ===");
  return *(double *)&x_bits;
}

float abs_b32(float x) {
  debug_print("%s\n", "=== abs_b32 ===");
  debug_print("\tx:      %+.13a\n", x);
  uint32_t x_bits = *(uint32_t *)&x;
  x_bits &= 0x7FFFFFFF;
  debug_print("\tabs(x): %+.13a\n", *(float *)&x_bits);
  debug_print("%s\n", "=== abs_b32 ===");
  return *(float *)&x_bits;
}

double pow2_b64(uint32_t n) {
  debug_print("%s\n", "=== pow2_b64 ===");
  debug_print("\tn: %d\n", n);
  uint64_t n_bits = (uint64_t)(n + 1023) << 52;
  debug_print("\tn_bits: %+.13a\n", *(double *)&n_bits);
  debug_print("%s\n", "=== get_exponent_b64 ===");
  return *(double *)&n_bits;
}

float pow2_b32(uint32_t n) {
  debug_print("%s\n", "=== pow2_b32 ===");
  debug_print("\tn: %d\n", n);
  uint32_t n_bits = (n + 127) << 23;
  debug_print("\tn_bits: %+.13a\n", *(float *)&n_bits);
  debug_print("%s\n", "=== pow_b32 ===");
  return *(float *)&n_bits;
}

double sr_round_b64(double sigma, double tau, double z) {
  // Compute round
  debug_print("%s\n", "=== sr_round_b64 ===");
  double eps = 0x1.0p-52;
  bool sign_tau, sign_sigma;
  sign_tau = tau < 0 ? -1 : 1;
  sign_sigma = sigma < 0 ? -1 : 1;
  uint32_t eta;
  if (sign_tau != sign_sigma) {
    eta = get_exponent_b64(predecessor_b64(abs_b64(sigma)));
  } else {
    eta = get_exponent_b64(sigma);
  }
  debug_print("\teta: %d\n", eta);
  double ulp = sign_tau * pow2_b64(eta) * eps;
  debug_print("\tulp: %.13a\n", ulp);
  double pi = ulp * z;
  debug_print("\tpi: %.13a\n", pi);
  double round;
  debug_print("\ttau + pi: %.13a\n", tau + pi);
  if (abs_b64(tau + pi) >= abs_b64(ulp)) {
    round = ulp;
  } else {
    round = 0;
  }
  debug_print("\tround: %.13a\n", round);
  debug_print("%s\n", "=== sr_round_b64 ===");
  return round;
}

float sr_round_b32(float sigma, float tau, float z) {
  // Compute round
  debug_print("%s\n", "=== sr_round_b32 ===");
  float eps = 0x1.0p-23;
  bool sign_tau, sign_sigma;
  sign_tau = tau < 0 ? -1 : 1;
  sign_sigma = sigma < 0 ? -1 : 1;
  uint32_t eta;
  if (sign_tau != sign_sigma) {
    eta = get_exponent_b32(predecessor_b32(abs_b32(sigma)));
  } else {
    eta = get_exponent_b32(sigma);
  }
  float ulp = sign_tau * pow2_b32(eta) * eps;
  float pi = ulp * z;
  float round;
  if (abs_b32(tau + pi) >= abs_b32(ulp)) {
    round = ulp;
  } else {
    round = 0;
  }
  return round;
}

void twosum_b64(double a, double b, double *tau, double *sigma) {
  // Compute tau and sigma
  *sigma = a + b;
  double z = *sigma - a;
  *tau = (a - (*sigma - z)) + (b - z);
}

void twosum_b32(float a, float b, float *tau, float *sigma) {
  // Compute tau and sigma
  *sigma = a + b;
  float z = *sigma - a;
  *tau = (a - (*sigma - z)) + (b - z);
}

void twoprodfma_b64(double a, double b, double *tau, double *sigma) {
  // Compute tau and sigma
  *sigma = a * b;
  *tau = __builtin_fma(a, b, -(*sigma));
}

void twoprodfma_b32(float a, float b, float *tau, float *sigma) {
  // Compute tau and sigma
  *sigma = a * b;
  *tau = __builtin_fmaf(a, b, -(*sigma));
}

double add2_double(double a, double b) {
  debug_print("%s\n", "=== add2_double ===");
  // compute SR(a+b)
  debug_print("\ta: %+.13a\n", a);
  debug_print("\tb: %+.13a\n", b);
  double z, tau, sigma, round;
  z = get_rand_double01(); // return float in [0,1)
  debug_print("\tz: %f\n", z);
  twosum_b64(a, b, &tau, &sigma);
  debug_print("\ttau: %+.13a, sigma: %+.13a\n", tau, sigma);
  round = sr_round_b64(sigma, tau, z);
  debug_print("\tround: %+.13a\n", round);
  debug_print("\tsigma + round: %+.13a\n", sigma + round);
  debug_print("%s\n", "=== add2_double ===");
  return sigma + round;
}

double sub2_double(double a, double b) { return add2_double(a, -b); }

double mul2_double(double a, double b) {
  // if a and b satisfy the condition (5.1), compute SR(a*b)
  double z, tau, sigma, round;
  z = get_rand_double01(); // return float in [0,1)
  twoprodfma_b64(a, b, &tau, &sigma);
  round = sr_round_b64(sigma, tau, z);
  return sigma + round;
}

double div2_double(double a, double b) {
  // compute SR(a/b)
  debug_print("%s\n", "=== div2_double ===");
  double z, sigma, tau, round;
  z = get_rand_double01(); // return float in [0,1)
  debug_print("\tz: %f\n", z);
  sigma = a / b;
  debug_print("\tsigma: %+.13a\n", sigma);
  tau = __builtin_fma(-sigma, b, a);
  debug_print("\ttau: %+.13a\n", tau);
  tau = tau / b;
  debug_print("\ttau: %+.13a\n", tau);
  round = sr_round_b64(sigma, tau, z);
  debug_print("\tround: %+.13a\n", round);
  debug_print("\tsigma + round: %+.13a\n", sigma + round);
  debug_print("%s\n", "=== div2_double ===");
  return sigma + round;
}

double sqrt2_double(double a) {
  // compute SR(sqrt(a))
  double z, sigma, tau, round;
  z = get_rand_double01(); // return float in [0,1)
  sigma = sqrt(a);
  tau = __builtin_fma(-sigma, sigma, a);
  tau = tau / (2 * sigma);
  round = sr_round_b64(sigma, tau, z);
  return sigma + round;
}

float add2_float(float a, float b) {
  // compute SR(a+b)
  float z, tau, sigma, round;
  z = get_rand_double01(); // return float in [0,1)
  twosum_b32(a, b, &tau, &sigma);
  round = sr_round_b32(sigma, tau, z);
  return sigma + round;
}

float sub2_float(float a, float b) { return add2_float(a, -b); }

float mul2_float(float a, float b) {
  // if a and b satisfy the condition (5.1), compute SR(a*b)
  float z, tau, sigma, round;
  z = get_rand_double01(); // return float in [0,1)
  twoprodfma_b32(a, b, &tau, &sigma);
  round = sr_round_b32(sigma, tau, z);
  return sigma + round;
}

float div2_float(float a, float b) {
  // compute SR(a/b)
  float z, sigma, tau, round;
  z = get_rand_double01(); // return float in [0,1)
  sigma = a / b;
  tau = __builtin_fmaf(-sigma, b, a);
  tau = tau / b;
  round = sr_round_b32(sigma, tau, z);
  return sigma + round;
}

float sqrt2_float(float a) {
  // compute SR(sqrt(a))
  float z, sigma, tau, round;
  z = get_rand_double01(); // return float in [0,1)
  sigma = sqrtf(a);
  tau = __builtin_fmaf(-sigma, sigma, a);
  tau = tau / (2 * sigma);
  round = sr_round_b32(sigma, tau, z);
  return sigma + round;
}