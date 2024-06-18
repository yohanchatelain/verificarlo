#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "float_const.h"
#include "float_struct.h"
#include "shishua.h"
#include <stdint.h>
#include <stdlib.h>

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

#ifdef XOROSHIRO
#define __INTERNAL_RNG_STATE xoroshiro_state
typedef uint64_t xoroshiro_state[2];
static __thread xoroshiro_state rng_state;
#elif defined(SHISHUA)
static __thread prng_state rng_state;
#else
#error "No PRNG defined"
#endif

#ifdef __GNUC__
typedef float float2 __attribute__((vector_size(8)));
typedef float float4 __attribute__((vector_size(16)));
typedef float float8 __attribute__((vector_size(32)));
typedef float float16 __attribute__((vector_size(64)));
typedef double double2 __attribute__((vector_size(16)));
typedef double double4 __attribute__((vector_size(32)));
typedef double double8 __attribute__((vector_size(64)));
typedef double double16 __attribute__((vector_size(128)));
typedef int32_t int2 __attribute__((vector_size(8)));
typedef int32_t int4 __attribute__((vector_size(16)));
typedef int32_t int8 __attribute__((vector_size(32)));
typedef int32_t int16 __attribute__((vector_size(64)));
typedef uint32_t uint2 __attribute__((vector_size(8)));
typedef uint32_t uint4 __attribute__((vector_size(16)));
typedef uint32_t uint8 __attribute__((vector_size(32)));
typedef uint32_t uint16 __attribute__((vector_size(64)));
typedef int64_t long2 __attribute__((vector_size(8)));
typedef int64_t long4 __attribute__((vector_size(16)));
typedef int64_t long8 __attribute__((vector_size(32)));
typedef int64_t long16 __attribute__((vector_size(64)));
typedef uint64_t ulong2 __attribute__((vector_size(8)));
typedef uint64_t ulong4 __attribute__((vector_size(16)));
typedef uint64_t ulong8 __attribute__((vector_size(32)));
typedef uint64_t ulong16 __attribute__((vector_size(64)));
#elif __clang__
typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef double double8 __attribute__((ext_vector_type(8)));
typedef double double16 __attribute__((ext_vector_type(16)));
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef float float8 __attribute__((ext_vector_type(8)));
typedef float float16 __attribute__((ext_vector_type(16)));
typedef int32_t int2 __attribute__((ext_vector_type(2)));
typedef int32_t int4 __attribute__((ext_vector_type(4)));
typedef int32_t int8 __attribute__((ext_vector_type(8)));
typedef int32_t int16 __attribute__((ext_vector_type(16)));
typedef uint32_t uint2 __attribute__((ext_vector_type(2)));
typedef uint32_t uint4 __attribute__((ext_vector_type(4)));
typedef uint32_t uint8 __attribute__((ext_vector_type(8)));
typedef uint32_t uint16 __attribute__((ext_vector_type(16)));
typedef int64_t long2 __attribute__((ext_vector_type(2)));
typedef int64_t long4 __attribute__((ext_vector_type(4)));
typedef int64_t long8 __attribute__((ext_vector_type(8)));
typedef int64_t long16 __attribute__((ext_vector_type(16)));
typedef uint64_t ulong2 __attribute__((ext_vector_type(2)));
typedef uint64_t ulong4 __attribute__((ext_vector_type(4)));
typedef uint64_t ulong8 __attribute__((ext_vector_type(8)));
typedef uint64_t ulong16 __attribute__((ext_vector_type(16)));
#else
#error "Compiler must be gcc or clang"
#endif

static pid_t global_tid = 0;
static bool already_initialized = 0;

uint64_t next_seed(uint64_t seed_state) {
  uint64_t z = (seed_state += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

#define rotl(x, k) ((x) << (k)) | ((x) >> (64 - (k)))

__attribute__((noinline)) uint64_t get_rand_uint64(void) {
#ifdef XOROSHIRO
  // XOROSHIRO128++
  const uint64_t s0 = rng_state[0];
  uint64_t s1 = rng_state[1];
  const uint64_t result = rotl(s0 + s1, 17) + s0;

  s1 ^= s0;
  rng_state[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21); // a, b
  rng_state[1] = rotl(s1, 28);                   // c

  return result;
#elif defined(SHISHUA)
  static int i = 0;
  static uint8_t buf[32];
  uint64_t rand = 0;
  if (i % 4 == 0) {
    prng_gen(&rng_state, buf);
    i = 0;
  }
  memcpy(&rand, buf + i * 8, sizeof(uint64_t));
  i++;

  return rand;
#else
#error "No PRNG defined"
#endif
}

__attribute__((noinline)) uint8_t get_rand_uint8(void) {
#ifdef XOROSHIRO
  return get_rand_uint64() & 0xFF;
#elif defined(SHISHUA)
  static int i = 0;
  static uint8_t buf[32];
  if (i % 32 == 0) {
    prng_gen(&rng_state, buf);
    i = 0;
  }
  i++;
  return buf[i];
#else
#error "No PRNG defined"
#endif
}

__attribute__((noinline)) uint16_t get_rand_uint16(void) {
#ifdef XOROSHIRO
  return get_rand_uint64() & 0xFFFF;
#elif defined(SHISHUA)
  static int i = 0;
  static uint8_t buf[32];
  uint16_t rand = 0;
  if (i % 16 == 0) {
    prng_gen(&rng_state, buf);
    i = 0;
  }
  memcpy(&rand, buf + i * 2, sizeof(uint16_t));
  i++;

  return rand;
#else
#error "No PRNG defined"
#endif
}

__attribute__((noinline)) uint32_t get_rand_uint32(void) {
#ifdef XOROSHIRO
  return get_rand_uint64() & 0xFFFFFFFF;
#elif defined(SHISHUA)
  static int i = 0;
  static uint8_t buf[32];
  uint32_t rand = 0;
  if (i % 8 == 0) {
    prng_gen(&rng_state, buf);
    i = 0;
  }
  memcpy(&rand, buf + i * 4, sizeof(uint32_t));
  i++;

  return rand;
#else
#error "No PRNG defined"
#endif
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
  uint64_t seed = 1;
  struct timeval t1;
  gettimeofday(&t1, NULL);
  seed = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);
#ifdef XOROSHIRO
  rng_state[0] = next_seed(seed);
  rng_state[1] = next_seed(seed);
#elif defined(SHISHUA)
  uint64_t seed_state[4] = {next_seed(seed), next_seed(seed), next_seed(seed),
                            next_seed(seed)};
  prng_init(&rng_state, seed_state);
#else
#error "No PRNG defined"
#endif
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

float ud_round_b32(float a) {
  if (a == 0)
    return a;
  uint32_t a_bits = *(uint32_t *)&a;
  uint32_t rand = get_rand_uint32();
  a_bits += (rand & 0x01) ? 1 : -1;
  return *(float *)&a_bits;
}

float2 ud_round_b32_2x(float2 a) {
  if (a[0] == 0 && a[1] == 0)
    return a;
  uint2 a_bits = *(uint2 *)&a;
  uint32_t rand = get_rand_uint32();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x01) ? 1 : -1;
  a_bits[1] += (rand & 0x02) ? 1 : -1;
  return *(float2 *)&a_bits;
}

float4 ud_round_b32_4x(float4 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0)
    return a;
  uint4 a_bits = *(uint4 *)&a;
  uint32_t rand = get_rand_uint32();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x01) ? 1 : -1;
  a_bits[1] += (rand & 0x02) ? 1 : -1;
  a_bits[2] += (rand & 0x04) ? 1 : -1;
  a_bits[3] += (rand & 0x08) ? 1 : -1;
  return *(float4 *)&a_bits;
}

float8 ud_round_b32_8x(float8 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0 &&
      a[5] == 0 && a[6] == 0 && a[7] == 0)
    return a;
  uint8 a_bits = *(uint8 *)&a;
  uint32_t rand = get_rand_uint32();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x01) ? 1 : -1;
  a_bits[1] += (rand & 0x02) ? 1 : -1;
  a_bits[2] += (rand & 0x04) ? 1 : -1;
  a_bits[3] += (rand & 0x08) ? 1 : -1;
  a_bits[4] += (rand & 0x10) ? 1 : -1;
  a_bits[5] += (rand & 0x20) ? 1 : -1;
  a_bits[6] += (rand & 0x40) ? 1 : -1;
  a_bits[7] += (rand & 0x80) ? 1 : -1;

  return *(float8 *)&a_bits;
}

float16 ud_round_b32_16x(float16 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0 &&
      a[5] == 0 && a[6] == 0 && a[7] == 0 && a[8] == 0 && a[9] == 0 &&
      a[10] == 0 && a[11] == 0 && a[12] == 0 && a[13] == 0 && a[14] == 0 &&
      a[15] == 0)
    return a;
  uint16 a_bits = *(uint16 *)&a;
  uint32_t rand = get_rand_uint32();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x0001) ? 1 : -1;
  a_bits[1] += (rand & 0x0002) ? 1 : -1;
  a_bits[2] += (rand & 0x0004) ? 1 : -1;
  a_bits[3] += (rand & 0x0008) ? 1 : -1;
  a_bits[4] += (rand & 0x0010) ? 1 : -1;
  a_bits[5] += (rand & 0x0020) ? 1 : -1;
  a_bits[6] += (rand & 0x0040) ? 1 : -1;
  a_bits[7] += (rand & 0x0080) ? 1 : -1;
  a_bits[8] += (rand & 0x0100) ? 1 : -1;
  a_bits[9] += (rand & 0x0200) ? 1 : -1;
  a_bits[10] += (rand & 0x0400) ? 1 : -1;
  a_bits[11] += (rand & 0x0800) ? 1 : -1;
  a_bits[12] += (rand & 0x1000) ? 1 : -1;
  a_bits[13] += (rand & 0x2000) ? 1 : -1;
  a_bits[14] += (rand & 0x4000) ? 1 : -1;
  a_bits[15] += (rand & 0x8000) ? 1 : -1;
  return *(float16 *)&a_bits;
}

double2 ud_round_b64_2x(double2 a) {
  if (a[0] == 0 && a[1] == 0)
    return a;
  ulong2 a_bits = *(ulong2 *)&a;
  uint32_t rand = get_rand_uint32();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x01) ? 1 : -1;
  a_bits[1] += (rand & 0x02) ? 1 : -1;
  return *(double2 *)&a_bits;
}

double4 ud_round_b64_4x(double4 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0)
    return a;
  ulong4 a_bits = *(ulong4 *)&a;
  uint32_t rand = get_rand_uint32();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x01) ? 1 : -1;
  a_bits[1] += (rand & 0x02) ? 1 : -1;
  a_bits[2] += (rand & 0x04) ? 1 : -1;
  a_bits[3] += (rand & 0x08) ? 1 : -1;
  return *(double4 *)&a_bits;
}

double8 ud_round_b64_8x(double8 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0 &&
      a[5] == 0 && a[6] == 0 && a[7] == 0)
    return a;
  ulong8 a_bits = *(ulong8 *)&a;
  uint32_t rand = get_rand_uint32();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x01) ? 1 : -1;
  a_bits[1] += (rand & 0x02) ? 1 : -1;
  a_bits[2] += (rand & 0x04) ? 1 : -1;
  a_bits[3] += (rand & 0x08) ? 1 : -1;
  a_bits[4] += (rand & 0x10) ? 1 : -1;
  a_bits[5] += (rand & 0x20) ? 1 : -1;
  a_bits[6] += (rand & 0x40) ? 1 : -1;
  a_bits[7] += (rand & 0x80) ? 1 : -1;
  return *(double8 *)&a_bits;
}

double16 ud_round_b64_16x(double16 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0 &&
      a[5] == 0 && a[6] == 0 && a[7] == 0 && a[8] == 0 && a[9] == 0 &&
      a[10] == 0 && a[11] == 0 && a[12] == 0 && a[13] == 0 && a[14] == 0 &&
      a[15] == 0)
    return a;
  ulong16 a_bits = *(ulong16 *)&a;
  uint32_t rand = get_rand_uint32();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x0001) ? 1 : -1;
  a_bits[1] += (rand & 0x0002) ? 1 : -1;
  a_bits[2] += (rand & 0x0004) ? 1 : -1;
  a_bits[3] += (rand & 0x0008) ? 1 : -1;
  a_bits[4] += (rand & 0x0010) ? 1 : -1;
  a_bits[5] += (rand & 0x0020) ? 1 : -1;
  a_bits[6] += (rand & 0x0040) ? 1 : -1;
  a_bits[7] += (rand & 0x0080) ? 1 : -1;
  a_bits[8] += (rand & 0x0100) ? 1 : -1;
  a_bits[9] += (rand & 0x0200) ? 1 : -1;
  a_bits[10] += (rand & 0x0400) ? 1 : -1;
  a_bits[11] += (rand & 0x0800) ? 1 : -1;
  a_bits[12] += (rand & 0x1000) ? 1 : -1;
  a_bits[13] += (rand & 0x2000) ? 1 : -1;
  a_bits[14] += (rand & 0x4000) ? 1 : -1;
  a_bits[15] += (rand & 0x8000) ? 1 : -1;
  return *(double16 *)&a_bits;
}

double ud_round_b64(double a) {
  if (a == 0)
    return a;
  uint64_t a_bits = *(uint64_t *)&a;
  uint64_t rand = get_rand_uint8() & 1;
  a_bits += (rand) ? 1 : -1;
  return *(double *)&a_bits;
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

double addUD_double(double a, double b) {
  debug_print("%s\n", "=== addUD_double ===");
  // compute SR(a+b)
  debug_print("\ta: %+.13a\n", a);
  debug_print("\tb: %+.13a\n", b);
  double round = ud_round_b64(a + b);
  debug_print("\tround: %+.13a\n", round);
  debug_print("%s\n", "=== addUD_double ===");
  return round;
}

double subUD_double(double a, double b) { return addUD_double(a, -b); }

double mulUD_double(double a, double b) {
  // if a and b satisfy the condition (5.1), compute SR(a*b)
  double round = ud_round_b64(a * b);
  return round;
}

double divUD_double(double a, double b) {
  // compute SR(a/b)
  debug_print("%s\n", "=== divUD_double ===");
  double round = ud_round_b64(a / b);
  debug_print("\tround: %+.13a\n", round);
  debug_print("%s\n", "=== divUD_double ===");
  return round;
}

float addUD_float(float a, float b) {
  // compute SR(a+b)
  float round = ud_round_b32(a + b);
  return round;
}

float2 addUD_float_2x(float2 a, float2 b) {
  // compute SR(a+b)
  float2 round = ud_round_b32_2x(a + b);
  return round;
}

float subUD_float(float a, float b) { return addUD_float(a, -b); }

float mulUD_float(float a, float b) {
  // if a and b satisfy the condition (5.1), compute SR(a*b)
  float round = ud_round_b32(a * b);
  return round;
}

float divUD_float(float a, float b) {
  // compute SR(a/b)
  float round = ud_round_b32(a / b);
  return round;
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