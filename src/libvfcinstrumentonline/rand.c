#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "float_const.h"
#include "float_struct.h"
#include "shishua.h"
#include "vector_types.h"

#ifdef DEBUG
#include <stdio.h>
#define debug_print(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__);
#else
#define debug_print(fmt, ...)
#endif

#ifdef XOROSHIRO
#define __INTERNAL_RNG_STATE xoroshiro_state
typedef uint64_t xoroshiro_state[2];
static __thread xoroshiro_state rng_state;
#elif defined(SHISHUA)
static __thread prng_state rng_state;
static int shishua_buffer_index = 0;
static uint8_t buf[SHISHUA_RNG_BUFFER_SIZE];
#else
#error "No PRNG defined"
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

__attribute__((noinline)) uint64_t _get_rand_uint64(void) {
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
  uint64_t rand = 0;
  const int bytes = sizeof(uint64_t);
  if (shishua_buffer_index + bytes > SHISHUA_RNG_BUFFER_SIZE) {
    prng_gen(&rng_state, buf);
    shishua_buffer_index = 0;
  }
  memcpy(&rand, buf + shishua_buffer_index * bytes, bytes);
  shishua_buffer_index += bytes;
  return rand;
#else
#error "No PRNG defined"
#endif
}

#ifdef XOROSHIRO
#define DEFINE_GET_RAND_UINTN(N)                                               \
  __attribute__((noinline)) uint##N##_t get_rand_uint##N(void) {               \
    return _get_rand_uint64() & UINT64_C(0xFFFFFFFFFFFFFFFF) >> (64 - N);      \
  }
#elif defined(SHISHUA)
#define DEFINE_GET_RAND_UINTN(N)                                               \
  __attribute__((noinline)) uint##N##_t get_rand_uint##N(void) {               \
    uint##N##_t rand = 0;                                                      \
    const int bytes = sizeof(rand);                                            \
    if (shishua_buffer_index + bytes > SHISHUA_RNG_BUFFER_SIZE) {              \
      prng_gen(&rng_state, buf);                                               \
      shishua_buffer_index = 0;                                                \
    }                                                                          \
    memcpy(&rand, buf + shishua_buffer_index * bytes, bytes);                  \
    shishua_buffer_index += bytes;                                             \
    return rand;                                                               \
  }
#else
#error "No PRNG defined"
#endif

__attribute__((noinline)) uint8_t _get_rand_uint8(void) {
#ifdef XOROSHIRO
  return get_rand_uint64() & 0xFF;
#elif defined(SHISHUA)
  if (shishua_buffer_index + 1 > SHISHUA_RNG_BUFFER_SIZE) {
    prng_gen(&rng_state, buf);
    shishua_buffer_index = 0;
  }
  uint8_t rand = buf[shishua_buffer_index];
  shishua_buffer_index++;
  return rand;
#else
#error "No PRNG defined"
#endif
}

DEFINE_GET_RAND_UINTN(8);
DEFINE_GET_RAND_UINTN(16);
DEFINE_GET_RAND_UINTN(32);
DEFINE_GET_RAND_UINTN(64);

// __attribute__((noinline)) uint16_t get_rand_uint16(void) {
// #ifdef XOROSHIRO
//   return get_rand_uint64() & 0xFFFF;
// #elif defined(SHISHUA)
//   uint16_t rand = 0;
//   const int bytes = sizeof(rand);
//   if (shishua_buffer_index + bytes > SHISHUA_RNG_BUFFER_SIZE) {
//     prng_gen(&rng_state, buf);
//     shishua_buffer_index = 0;
//   }
//   memcpy(&rand, shishua_buffer_index + i * bytes, bytes);
//   i += bytes;
//   return rand;
// #else
// #error "No PRNG defined"
// #endif
// }

// __attribute__((noinline)) uint32_t get_rand_uint32(void) {
// #ifdef XOROSHIRO
//   return get_rand_uint64() & 0xFFFFFFFF;
// #elif defined(SHISHUA)
//   uint32_t rand = 0;
//   const int bytes = sizeof(uint32_t);
//   if (shishua_buffer_index + bytes > SHISHUA_RNG_BUFFER_SIZE) {
//     prng_gen(&rng_state, buf);
//     shishua_buffer_index = 0;
//   }
//   memcpy(&rand, shishua_buffer_index + i * bytes, bytes);
//   i += bytes;
//   return rand;
// #else
// #error "No PRNG defined"
// #endif
// }

// __attribute__((noinline)) uint64_t get_rand_uintN(int nb) {
// #ifdef XOROSHIRO
//   return get_rand_uint64();
// #elif defined(SHISHUA)
//   static int i = 0;
//   static uint8_t buf[SHISHUA_RNG_BUFFER_SIZE];
//   uint32_t rand = 0;
//   if (i + nb > SHISHUA_RNG_BUFFER_SIZE) {
//     prng_gen(&rng_state, buf);
//     i = 0;
//   }
//   memcpy(&rand, buf + i * nb, nb);
//   i += nb;

//   return rand;
// #else
// #error "No PRNG defined"
// #endif
// }

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
  if (a == 0) {
    return a;
  }
  uint32_t a_bits = *(uint32_t *)&a;
  uint64_t rand = get_rand_uint64();
  a_bits += (rand & 0x01) ? 1 : -1;
  return *(float *)&a_bits;
}

float2 ud_round_b32_2x(float2 a) {
  if (a[0] == 0 && a[1] == 0) {
    return a;
  }
  uint2 a_bits = *(uint2 *)&a;
  uint64_t rand = get_rand_uint64();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x01) ? 1 : -1;
  a_bits[1] += (rand & 0x02) ? 1 : -1;
  return *(float2 *)&a_bits;
}

float4 ud_round_b32_4x(float4 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0) {
    return a;
  }
  uint4 a_bits = *(uint4 *)&a;
  uint64_t rand = get_rand_uint64();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x01) ? 1 : -1;
  a_bits[1] += (rand & 0x02) ? 1 : -1;
  a_bits[2] += (rand & 0x04) ? 1 : -1;
  a_bits[3] += (rand & 0x08) ? 1 : -1;
  return *(float4 *)&a_bits;
}

float8 ud_round_b32_8x(float8 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0 &&
      a[5] == 0 && a[6] == 0 && a[7] == 0) {
    return a;
  }
  uint8 a_bits = *(uint8 *)&a;
  uint64_t rand = get_rand_uint64();
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
      a[15] == 0) {
    return a;
  }
  uint16 a_bits = *(uint16 *)&a;
  uint64_t rand = get_rand_uint64();
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

float32 ud_round_b32_32x(float32 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0 &&
      a[5] == 0 && a[6] == 0 && a[7] == 0 && a[8] == 0 && a[9] == 0 &&
      a[10] == 0 && a[11] == 0 && a[12] == 0 && a[13] == 0 && a[14] == 0 &&
      a[15] == 0 && a[16] == 0 && a[17] == 0 && a[18] == 0 && a[19] == 0 &&
      a[20] == 0 && a[21] == 0 && a[22] == 0 && a[23] == 0 && a[24] == 0 &&
      a[25] == 0 && a[26] == 0 && a[27] == 0 && a[28] == 0 && a[29] == 0 &&
      a[30] == 0 && a[31] == 0) {
    return a;
  }
  uint32 a_bits = *(uint32 *)&a;
  uint64_t rand = get_rand_uint64();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x00000001) ? 1 : -1;
  a_bits[1] += (rand & 0x00000002) ? 1 : -1;
  a_bits[2] += (rand & 0x00000040) ? 1 : -1;
  a_bits[3] += (rand & 0x00000080) ? 1 : -1;
  a_bits[4] += (rand & 0x00000100) ? 1 : -1;
  a_bits[5] += (rand & 0x00000200) ? 1 : -1;
  a_bits[6] += (rand & 0x00000400) ? 1 : -1;
  a_bits[7] += (rand & 0x00000800) ? 1 : -1;
  a_bits[8] += (rand & 0x00000100) ? 1 : -1;
  a_bits[9] += (rand & 0x00000200) ? 1 : -1;
  a_bits[10] += (rand & 0x00000400) ? 1 : -1;
  a_bits[11] += (rand & 0x00000800) ? 1 : -1;
  a_bits[12] += (rand & 0x00001000) ? 1 : -1;
  a_bits[13] += (rand & 0x00002000) ? 1 : -1;
  a_bits[14] += (rand & 0x00004000) ? 1 : -1;
  a_bits[15] += (rand & 0x00008000) ? 1 : -1;
  a_bits[16] += (rand & 0x00010000) ? 1 : -1;
  a_bits[17] += (rand & 0x00020000) ? 1 : -1;
  a_bits[18] += (rand & 0x00040000) ? 1 : -1;
  a_bits[19] += (rand & 0x00080000) ? 1 : -1;
  a_bits[20] += (rand & 0x00100000) ? 1 : -1;
  a_bits[21] += (rand & 0x00200000) ? 1 : -1;
  a_bits[22] += (rand & 0x00400000) ? 1 : -1;
  a_bits[23] += (rand & 0x00800000) ? 1 : -1;
  a_bits[24] += (rand & 0x01000000) ? 1 : -1;
  a_bits[25] += (rand & 0x02000000) ? 1 : -1;
  a_bits[26] += (rand & 0x04000000) ? 1 : -1;
  a_bits[27] += (rand & 0x08000000) ? 1 : -1;
  a_bits[28] += (rand & 0x10000000) ? 1 : -1;
  a_bits[29] += (rand & 0x20000000) ? 1 : -1;
  a_bits[30] += (rand & 0x40000000) ? 1 : -1;
  a_bits[31] += (rand & 0x80000000) ? 1 : -1;
  return *(float32 *)&a_bits;
}

float64 ud_round_b32_64x(float64 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0 &&
      a[5] == 0 && a[6] == 0 && a[7] == 0 && a[8] == 0 && a[9] == 0 &&
      a[10] == 0 && a[11] == 0 && a[12] == 0 && a[13] == 0 && a[14] == 0 &&
      a[15] == 0 && a[16] == 0 && a[17] == 0 && a[18] == 0 && a[19] == 0 &&
      a[20] == 0 && a[21] == 0 && a[22] == 0 && a[23] == 0 && a[24] == 0 &&
      a[25] == 0 && a[26] == 0 && a[27] == 0 && a[28] == 0 && a[29] == 0 &&
      a[30] == 0 && a[31] == 0 && a[32] == 0 && a[33] == 0 && a[34] == 0 &&
      a[35] == 0 && a[36] == 0 && a[37] == 0 && a[38] == 0 && a[39] == 0 &&
      a[40] == 0 && a[41] == 0 && a[42] == 0 && a[43] == 0 && a[44] == 0 &&
      a[45] == 0 && a[46] == 0 && a[47] == 0 && a[48] == 0 && a[49] == 0 &&
      a[50] == 0 && a[51] == 0 && a[52] == 0 && a[53] == 0 && a[54] == 0 &&
      a[55] == 0 && a[56] == 0 && a[57] == 0 && a[58] == 0 && a[59] == 0 &&
      a[60] == 0 && a[61] == 0 && a[62] == 0 && a[63] == 0) {
    return a;
  }
  uint64 a_bits = *(uint64 *)&a;
  uint64_t rand = get_rand_uint64();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x0000000000000001) ? 1 : -1;
  a_bits[1] += (rand & 0x0000000000000002) ? 1 : -1;
  a_bits[2] += (rand & 0x0000000000000004) ? 1 : -1;
  a_bits[3] += (rand & 0x0000000000000008) ? 1 : -1;
  a_bits[4] += (rand & 0x0000000000000010) ? 1 : -1;
  a_bits[5] += (rand & 0x0000000000000020) ? 1 : -1;
  a_bits[6] += (rand & 0x0000000000000040) ? 1 : -1;
  a_bits[7] += (rand & 0x0000000000000080) ? 1 : -1;
  a_bits[8] += (rand & 0x0000000000000100) ? 1 : -1;
  a_bits[9] += (rand & 0x0000000000000200) ? 1 : -1;
  a_bits[10] += (rand & 0x0000000000000400) ? 1 : -1;
  a_bits[11] += (rand & 0x0000000000000800) ? 1 : -1;
  a_bits[12] += (rand & 0x0000000000001000) ? 1 : -1;
  a_bits[13] += (rand & 0x0000000000002000) ? 1 : -1;
  a_bits[14] += (rand & 0x0000000000004000) ? 1 : -1;
  a_bits[15] += (rand & 0x0000000000008000) ? 1 : -1;
  a_bits[16] += (rand & 0x0000000000010000) ? 1 : -1;
  a_bits[17] += (rand & 0x0000000000020000) ? 1 : -1;
  a_bits[18] += (rand & 0x0000000000040000) ? 1 : -1;
  a_bits[19] += (rand & 0x0000000000080000) ? 1 : -1;
  a_bits[20] += (rand & 0x0000000000100000) ? 1 : -1;
  a_bits[21] += (rand & 0x0000000000200000) ? 1 : -1;
  a_bits[22] += (rand & 0x0000000000400000) ? 1 : -1;
  a_bits[23] += (rand & 0x0000000000800000) ? 1 : -1;
  a_bits[24] += (rand & 0x0000000001000000) ? 1 : -1;
  a_bits[25] += (rand & 0x0000000002000000) ? 1 : -1;
  a_bits[26] += (rand & 0x0000000004000000) ? 1 : -1;
  a_bits[27] += (rand & 0x0000000008000000) ? 1 : -1;
  a_bits[28] += (rand & 0x0000000010000000) ? 1 : -1;
  a_bits[29] += (rand & 0x0000000020000000) ? 1 : -1;
  a_bits[30] += (rand & 0x0000000040000000) ? 1 : -1;
  a_bits[31] += (rand & 0x0000000080000000) ? 1 : -1;
  a_bits[32] += (rand & 0x0000000100000000) ? 1 : -1;
  a_bits[33] += (rand & 0x0000000200000000) ? 1 : -1;
  a_bits[34] += (rand & 0x0000000400000000) ? 1 : -1;
  a_bits[35] += (rand & 0x0000000800000000) ? 1 : -1;
  a_bits[36] += (rand & 0x0000001000000000) ? 1 : -1;
  a_bits[37] += (rand & 0x0000002000000000) ? 1 : -1;
  a_bits[38] += (rand & 0x0000004000000000) ? 1 : -1;
  a_bits[39] += (rand & 0x0000008000000000) ? 1 : -1;
  a_bits[40] += (rand & 0x0000010000000000) ? 1 : -1;
  a_bits[41] += (rand & 0x0000020000000000) ? 1 : -1;
  a_bits[42] += (rand & 0x0000040000000000) ? 1 : -1;
  a_bits[43] += (rand & 0x0000080000000000) ? 1 : -1;
  a_bits[44] += (rand & 0x0000100000000000) ? 1 : -1;
  a_bits[45] += (rand & 0x0000200000000000) ? 1 : -1;
  a_bits[46] += (rand & 0x0000400000000000) ? 1 : -1;
  a_bits[47] += (rand & 0x0000800000000000) ? 1 : -1;
  a_bits[48] += (rand & 0x0001000000000000) ? 1 : -1;
  a_bits[49] += (rand & 0x0002000000000000) ? 1 : -1;
  a_bits[50] += (rand & 0x0004000000000000) ? 1 : -1;
  a_bits[51] += (rand & 0x0008000000000000) ? 1 : -1;
  a_bits[52] += (rand & 0x0010000000000000) ? 1 : -1;
  a_bits[53] += (rand & 0x0020000000000000) ? 1 : -1;
  a_bits[54] += (rand & 0x0040000000000000) ? 1 : -1;
  a_bits[55] += (rand & 0x0080000000000000) ? 1 : -1;
  a_bits[56] += (rand & 0x0100000000000000) ? 1 : -1;
  a_bits[57] += (rand & 0x0200000000000000) ? 1 : -1;
  a_bits[58] += (rand & 0x0400000000000000) ? 1 : -1;
  a_bits[59] += (rand & 0x0800000000000000) ? 1 : -1;
  a_bits[60] += (rand & 0x1000000000000000) ? 1 : -1;
  a_bits[61] += (rand & 0x2000000000000000) ? 1 : -1;
  a_bits[62] += (rand & 0x4000000000000000) ? 1 : -1;
  a_bits[63] += (rand & 0x8000000000000000) ? 1 : -1;
  return *(float64 *)&a_bits;
}

double2 ud_round_b64_2x(double2 a) {
  if (a[0] == 0 && a[1] == 0) {
    return a;
  }
  ulong2 a_bits = *(ulong2 *)&a;
  uint64_t rand = get_rand_uint64();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x01) ? 1 : -1;
  a_bits[1] += (rand & 0x02) ? 1 : -1;
  return *(double2 *)&a_bits;
}

double4 ud_round_b64_4x(double4 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0) {
    return a;
  }
  ulong4 a_bits = *(ulong4 *)&a;
  uint64_t rand = get_rand_uint64();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x1) ? 1 : -1;
  a_bits[1] += (rand & 0x2) ? 1 : -1;
  a_bits[2] += (rand & 0x4) ? 1 : -1;
  a_bits[3] += (rand & 0x8) ? 1 : -1;
  return *(double4 *)&a_bits;
}

double8 ud_round_b64_8x(double8 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0 &&
      a[5] == 0 && a[6] == 0 && a[7] == 0) {
    return a;
  }
  ulong8 a_bits = *(ulong8 *)&a;
  uint64_t rand = get_rand_uint64();
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
      a[15] == 0) {
    return a;
  }
  ulong16 a_bits = *(ulong16 *)&a;
  uint64_t rand = get_rand_uint64();
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

double32 ud_round_b64_32x(double32 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0 &&
      a[5] == 0 && a[6] == 0 && a[7] == 0 && a[8] == 0 && a[9] == 0 &&
      a[10] == 0 && a[11] == 0 && a[12] == 0 && a[13] == 0 && a[14] == 0 &&
      a[15] == 0 && a[16] == 0 && a[17] == 0 && a[18] == 0 && a[19] == 0 &&
      a[20] == 0 && a[21] == 0 && a[22] == 0 && a[23] == 0 && a[24] == 0 &&
      a[25] == 0 && a[26] == 0 && a[27] == 0 && a[28] == 0 && a[29] == 0 &&
      a[30] == 0 && a[31] == 0) {
    return a;
  }
  ulong32 a_bits = *(ulong32 *)&a;
  uint64_t rand = get_rand_uint64();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x00000001) ? 1 : -1;
  a_bits[1] += (rand & 0x00000002) ? 1 : -1;
  a_bits[2] += (rand & 0x00000004) ? 1 : -1;
  a_bits[3] += (rand & 0x00000008) ? 1 : -1;
  a_bits[4] += (rand & 0x00000010) ? 1 : -1;
  a_bits[5] += (rand & 0x00000020) ? 1 : -1;
  a_bits[6] += (rand & 0x00000040) ? 1 : -1;
  a_bits[7] += (rand & 0x00000080) ? 1 : -1;
  a_bits[8] += (rand & 0x00000100) ? 1 : -1;
  a_bits[9] += (rand & 0x00000200) ? 1 : -1;
  a_bits[10] += (rand & 0x00000400) ? 1 : -1;
  a_bits[11] += (rand & 0x00000800) ? 1 : -1;
  a_bits[12] += (rand & 0x00001000) ? 1 : -1;
  a_bits[13] += (rand & 0x00002000) ? 1 : -1;
  a_bits[14] += (rand & 0x00004000) ? 1 : -1;
  a_bits[15] += (rand & 0x00008000) ? 1 : -1;
  a_bits[16] += (rand & 0x00010000) ? 1 : -1;
  a_bits[17] += (rand & 0x00020000) ? 1 : -1;
  a_bits[18] += (rand & 0x00040000) ? 1 : -1;
  a_bits[19] += (rand & 0x00080000) ? 1 : -1;
  a_bits[20] += (rand & 0x00100000) ? 1 : -1;
  a_bits[21] += (rand & 0x00200000) ? 1 : -1;
  a_bits[22] += (rand & 0x00400000) ? 1 : -1;
  a_bits[23] += (rand & 0x00800000) ? 1 : -1;
  a_bits[24] += (rand & 0x01000000) ? 1 : -1;
  a_bits[25] += (rand & 0x02000000) ? 1 : -1;
  a_bits[26] += (rand & 0x04000000) ? 1 : -1;
  a_bits[27] += (rand & 0x08000000) ? 1 : -1;
  a_bits[28] += (rand & 0x10000000) ? 1 : -1;
  a_bits[29] += (rand & 0x20000000) ? 1 : -1;
  a_bits[30] += (rand & 0x40000000) ? 1 : -1;
  a_bits[31] += (rand & 0x80000000) ? 1 : -1;
  return *(double32 *)&a_bits;
}

double64 ud_round_b64_64x(double64 a) {
  if (a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0 && a[4] == 0 &&
      a[5] == 0 && a[6] == 0 && a[7] == 0 && a[8] == 0 && a[9] == 0 &&
      a[10] == 0 && a[11] == 0 && a[12] == 0 && a[13] == 0 && a[14] == 0 &&
      a[15] == 0 && a[16] == 0 && a[17] == 0 && a[18] == 0 && a[19] == 0 &&
      a[20] == 0 && a[21] == 0 && a[22] == 0 && a[23] == 0 && a[24] == 0 &&
      a[25] == 0 && a[26] == 0 && a[27] == 0 && a[28] == 0 && a[29] == 0 &&
      a[30] == 0 && a[31] == 0 && a[32] == 0 && a[33] == 0 && a[34] == 0 &&
      a[35] == 0 && a[36] == 0 && a[37] == 0 && a[38] == 0 && a[39] == 0 &&
      a[40] == 0 && a[41] == 0 && a[42] == 0 && a[43] == 0 && a[44] == 0 &&
      a[45] == 0 && a[46] == 0 && a[47] == 0 && a[48] == 0 && a[49] == 0 &&
      a[50] == 0 && a[51] == 0 && a[52] == 0 && a[53] == 0 && a[54] == 0 &&
      a[55] == 0 && a[56] == 0 && a[57] == 0 && a[58] == 0 && a[59] == 0 &&
      a[60] == 0 && a[61] == 0 && a[62] == 0 && a[63] == 0) {
    return a;
  }
  ulong64 a_bits = *(ulong64 *)&a;
  uint64_t rand = get_rand_uint64();
  // use vectorized operations to add 1 or -1 to each element of the vector
  a_bits[0] += (rand & 0x0000000000000001) ? 1 : -1;
  a_bits[1] += (rand & 0x0000000000000002) ? 1 : -1;
  a_bits[2] += (rand & 0x0000000000000004) ? 1 : -1;
  a_bits[3] += (rand & 0x0000000000000008) ? 1 : -1;
  a_bits[4] += (rand & 0x0000000000000010) ? 1 : -1;
  a_bits[5] += (rand & 0x0000000000000020) ? 1 : -1;
  a_bits[6] += (rand & 0x0000000000000040) ? 1 : -1;
  a_bits[7] += (rand & 0x0000000000000080) ? 1 : -1;
  a_bits[8] += (rand & 0x0000000000000100) ? 1 : -1;
  a_bits[9] += (rand & 0x0000000000000200) ? 1 : -1;
  a_bits[10] += (rand & 0x0000000000000400) ? 1 : -1;
  a_bits[11] += (rand & 0x0000000000000800) ? 1 : -1;
  a_bits[12] += (rand & 0x0000000000001000) ? 1 : -1;
  a_bits[13] += (rand & 0x0000000000002000) ? 1 : -1;
  a_bits[14] += (rand & 0x0000000000004000) ? 1 : -1;
  a_bits[15] += (rand & 0x0000000000008000) ? 1 : -1;
  a_bits[16] += (rand & 0x0000000000010000) ? 1 : -1;
  a_bits[17] += (rand & 0x0000000000020000) ? 1 : -1;
  a_bits[18] += (rand & 0x0000000000040000) ? 1 : -1;
  a_bits[19] += (rand & 0x0000000000080000) ? 1 : -1;
  a_bits[20] += (rand & 0x0000000000100000) ? 1 : -1;
  a_bits[21] += (rand & 0x0000000000200000) ? 1 : -1;
  a_bits[22] += (rand & 0x0000000000400000) ? 1 : -1;
  a_bits[23] += (rand & 0x0000000000800000) ? 1 : -1;
  a_bits[24] += (rand & 0x0000000001000000) ? 1 : -1;
  a_bits[25] += (rand & 0x0000000002000000) ? 1 : -1;
  a_bits[26] += (rand & 0x0000000004000000) ? 1 : -1;
  a_bits[27] += (rand & 0x0000000008000000) ? 1 : -1;
  a_bits[28] += (rand & 0x0000000010000000) ? 1 : -1;
  a_bits[29] += (rand & 0x0000000020000000) ? 1 : -1;
  a_bits[30] += (rand & 0x0000000040000000) ? 1 : -1;
  a_bits[31] += (rand & 0x0000000080000000) ? 1 : -1;
  a_bits[32] += (rand & 0x0000000100000000) ? 1 : -1;
  a_bits[33] += (rand & 0x0000000200000000) ? 1 : -1;
  a_bits[34] += (rand & 0x0000000400000000) ? 1 : -1;
  a_bits[35] += (rand & 0x0000000800000000) ? 1 : -1;
  a_bits[36] += (rand & 0x0000001000000000) ? 1 : -1;
  a_bits[37] += (rand & 0x0000002000000000) ? 1 : -1;
  a_bits[38] += (rand & 0x0000004000000000) ? 1 : -1;
  a_bits[39] += (rand & 0x0000008000000000) ? 1 : -1;
  a_bits[40] += (rand & 0x0000010000000000) ? 1 : -1;
  a_bits[41] += (rand & 0x0000020000000000) ? 1 : -1;
  a_bits[42] += (rand & 0x0000040000000000) ? 1 : -1;
  a_bits[43] += (rand & 0x0000080000000000) ? 1 : -1;
  a_bits[44] += (rand & 0x0000100000000000) ? 1 : -1;
  a_bits[45] += (rand & 0x0000200000000000) ? 1 : -1;
  a_bits[46] += (rand & 0x0000400000000000) ? 1 : -1;
  a_bits[47] += (rand & 0x0000800000000000) ? 1 : -1;
  a_bits[48] += (rand & 0x0001000000000000) ? 1 : -1;
  a_bits[49] += (rand & 0x0002000000000000) ? 1 : -1;
  a_bits[50] += (rand & 0x0004000000000000) ? 1 : -1;
  a_bits[51] += (rand & 0x0008000000000000) ? 1 : -1;
  a_bits[52] += (rand & 0x0010000000000000) ? 1 : -1;
  a_bits[53] += (rand & 0x0020000000000000) ? 1 : -1;
  a_bits[54] += (rand & 0x0040000000000000) ? 1 : -1;
  a_bits[55] += (rand & 0x0080000000000000) ? 1 : -1;
  a_bits[56] += (rand & 0x0100000000000000) ? 1 : -1;
  a_bits[57] += (rand & 0x0200000000000000) ? 1 : -1;
  a_bits[58] += (rand & 0x0400000000000000) ? 1 : -1;
  a_bits[59] += (rand & 0x0800000000000000) ? 1 : -1;
  a_bits[60] += (rand & 0x1000000000000000) ? 1 : -1;
  a_bits[61] += (rand & 0x2000000000000000) ? 1 : -1;
  a_bits[62] += (rand & 0x4000000000000000) ? 1 : -1;
  a_bits[63] += (rand & 0x8000000000000000) ? 1 : -1;
  return *(double64 *)&a_bits;
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