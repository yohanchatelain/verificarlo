#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "interflop/common/float_utils.h"

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

double twosum_b64(double a, double b, double *tau, double *sigma) {
  // Compute tau and sigma
  *sigma = a + b;
  double z = *sigma - a;
  *tau = (a - (*sigma - z)) + (b - z);
}

float twosum_b32(float a, float b, float *tau, float *sigma) {
  // Compute tau and sigma
  *sigma = a + b;
  float z = *sigma - a;
  *tau = (a - (*sigma - z)) + (b - z);
}

double twoprodfma_b64(double a, double b, double *tau, double *sigma) {
  // Compute tau and sigma
  *sigma = a * b;
  *tau = __builtin_fma(a, b, -(*sigma));
}

float twoprodfma_b32(float a, float b, float *tau, float *sigma) {
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