#ifndef __VERIFICARLO_SRLIB_RAND_XOROSHIRO256P_HPP__
#define __VERIFICARLO_SRLIB_RAND_XOROSHIRO256P_HPP__

#include <stdio.h>

#include <array>
#include <immintrin.h>
#include <stdint.h>

#include "vector_types.hpp"

namespace xoroshiro256plus {

namespace scalar {
typedef uint64_t rn_uint64;

/* The current state of the generators. */
typedef struct {
  uint64_t s[4];
} state;

static inline uint64_t rotl(uint64_t x, int k) {
  return (x << k) | (x >> (64 - k));
}

static inline uint64_t next(state &s) {
  uint64_t result = s.s[0] + s.s[3];
  uint64_t t = s.s[1] << 17;

  s.s[2] ^= s.s[0];
  s.s[3] ^= s.s[1];
  s.s[1] ^= s.s[2];
  s.s[0] ^= s.s[3];
  s.s[2] ^= t;
  s.s[3] = rotl(s.s[3], 45);

  return result;
}

void init(state &s, const std::array<uint64_t, 1> &seeds) {
  s.s[0] = seeds[0];
  s.s[1] = 0;
  s.s[2] = 0;
  s.s[3] = 0;
}

} // namespace scalar

namespace sse4_2 {
/* The current state of the generators. */
typedef struct {
  __m128i s[4];
} state;

__attribute__((target("sse4.2"))) static __inline __m128i rotl(__m128i x,
                                                               int k) {
  return _mm_or_si128(_mm_slli_epi64(x, k), _mm_srli_epi64(x, 64 - k));
}

__attribute__((target("sse4.2"))) static inline __m128i next(state &s) {
  // result[i] = s[0][i] + s[3][i];
  __m128i result = _mm_add_epi64(s.s[0], s.s[3]);

  // t[i] = s[1][i] << 17;
  __m128i t = _mm_slli_epi64(s.s[1], 17);

  // s[2][i] ^= s[0][i];
  s.s[2] = _mm_xor_si128(s.s[2], s.s[0]);

  // s[3][i] ^= s[1][i];
  s.s[3] = _mm_xor_si128(s.s[3], s.s[1]);

  // s[1][i] ^= s[2][i];
  s.s[1] = _mm_xor_si128(s.s[1], s.s[2]);

  // s[0][i] ^= s[3][i];
  s.s[0] = _mm_xor_si128(s.s[0], s.s[3]);

  // s[2][i] ^= t[i];
  s.s[2] = _mm_xor_si128(s.s[2], t);

  // s[3][i] = rotl(s[3][i], 45);
  s.s[3] = rotl(s.s[3], 45);

  return result;
}

__attribute__((target("sse4.2"))) static inline __m128d
get_rand_double01(state &s) {
  __m128i rng = next(s);
  __m128i shift = _mm_srli_epi64(rng, 11);
  __m128d shiftd = _mm_cvtepi64_pd(shift) * 0x1.0p-53;
  return shiftd;
}

__attribute__((target("sse4.2"))) void
init(state &s, const std::array<uint64_t, 2> &seeds) {
  s.s[0] = _mm_set_epi64x(seeds[0], seeds[1]);
  s.s[1] = _mm_setzero_si128();
  s.s[2] = _mm_setzero_si128();
  s.s[3] = _mm_setzero_si128();
}

} // namespace sse4_2

namespace avx {

void debug_m256i(const char msg[], __m256i a = _mm256_setzero_si256(),
                 bool novalue = false) {
  printf("%s: ", msg);
  if (novalue) {
    printf("\n");
    return;
  }
  for (int i = 0; i < 4; i++) {
    printf("%016lx ", (uint64_t)a[i]);
  }
  printf("\n");
}

void debug_m256d(const char msg[], __m256d a = _mm256_setzero_pd(),
                 bool novalue = false) {
  printf("%s: ", msg);
  if (novalue) {
    printf("\n");
    return;
  }
  for (int i = 0; i < 4; i++) {
    printf("%+.13a ", a[i]);
  }
  printf("\n");
}

void debug(const char msg[]) { debug_m256i(msg, _mm256_setzero_si256(), true); }

/* The current state of the generators. */
typedef struct {
  __m256i s[4];
} state;

__attribute__((target("avx"))) static __inline __m256i rotl(__m256i x, int k) {
  return x << k | (x >> (64 - k));
}

__attribute__((target("avx"))) static inline __m256i next(state &s) {
  debug_m256i("state[0]", s.s[0]);
  debug_m256i("state[1]", s.s[1]);
  debug_m256i("state[2]", s.s[2]);
  debug_m256i("state[3]", s.s[3]);
  debug("----\n");
  // result[i] = s[0][i] + s[3][i];
  __m256i result = s.s[0] + s.s[3];
  debug("result[i] = s[0][i] + s[3][i];");
  debug_m256i("result  ", result);

  // t[i] = s[1][i] << 17;
  __m256i t = s.s[1] << 17;
  debug("t[i] = s[1][i] << 17;");
  debug_m256i("t       ", t);

  // s[2][i] ^= s[0][i];
  s.s[2] = s.s[2] ^ s.s[0];
  debug("s[2][i] ^= s[0][i];");
  debug_m256i("s[2]    ", s.s[2]);

  // s[3][i] ^= s[1][i];
  s.s[3] = s.s[3] ^ s.s[1];
  debug("s[3][i] ^= s[1][i];");
  debug_m256i("s[3]    ", s.s[3]);

  // s[1][i] ^= s[2][i];
  s.s[1] = s.s[1] ^ s.s[2];
  debug("s[1][i] ^= s[2][i];");
  debug_m256i("s[1]    ", s.s[1]);

  // s[0][i] ^= s[3][i];
  s.s[0] = s.s[0] ^ s.s[3];
  debug("s[0][i] ^= s[3][i];");
  debug_m256i("s[0]    ", s.s[0]);

  // s[2][i] ^= t[i];
  s.s[2] = s.s[2] ^ t;
  debug("s[2][i] ^= t[i];");
  debug_m256i("s[2]    ", s.s[2]);

  // s[3][i] = rotl(s[3][i], 45);
  s.s[3] = rotl(s.s[3], 45);
  debug("s[3][i] = rotl(s[3][i], 45);");
  debug_m256i("s[3]    ", s.s[3]);

  return result;
}

__attribute__((target("avx"))) static inline __m256d
get_rand_double01(state &s) {
  __m256i rng = next(s);
  __m128i rng_lo = _mm256_extractf128_si256(rng, 0);
  __m128i rng_hi = _mm256_extractf128_si256(rng, 1);
  debug_m256i("next:", rng);
  debug("----\n");
  __m128i shift_lo = _mm_srli_epi64(rng_lo, 11);
  __m128i shift_hi = _mm_srli_epi64(rng_hi, 11);
  debug_m256i("shift", _mm256_set_m128i(shift_hi, shift_lo));
  debug_m256d("shift", _mm256_set_m128d(_mm_cvtepi64_pd(shift_hi),
                                        _mm_cvtepi64_pd(shift_lo)));
  __m128d shiftd_lo = _mm_cvtepi64_pd(shift_lo) * 0x1.0p-53;
  __m128d shiftd_hi = _mm_cvtepi64_pd(shift_hi) * 0x1.0p-53;
  __m256d shiftd = _mm256_set_m128d(shiftd_hi, shiftd_lo);
  debug_m256d("shiftd", shiftd);
  return shiftd;
}

__attribute__((target("avx"))) void init(state &s,
                                         const std::array<uint64_t, 4> &seeds) {
  s.s[0] = _mm256_set_epi64x(seeds[0], seeds[1], seeds[2], seeds[3]);
  s.s[1] = _mm256_setzero_si256();
  s.s[2] = _mm256_setzero_si256();
  s.s[3] = _mm256_setzero_si256();
}

} // namespace avx

namespace avx2 {

#define RET_DEBUG
#ifdef RET_DEGUG
#define RET return;
#else
#define RET
#endif

void debug_m128i(const char msg[], __m128i a = _mm_setzero_si128(),
                 bool novalue = false) {
  return;
  printf("%s: ", msg);
  if (novalue) {
    printf("\n");
    return;
  }
  for (int i = 0; i < 2; i++) {
    printf("%016lx ", (uint64_t)a[i]);
  }
  printf("\n");
}

void debug_m256i(const char msg[], __m256i a = _mm256_setzero_si256(),
                 bool novalue = false) {
  return;
  printf("%s: ", msg);
  if (novalue) {
    printf("\n");
    return;
  }
  for (int i = 0; i < 4; i++) {
    printf("%016lx ", (uint64_t)a[i]);
  }
  printf("\n");
}

void debug_m256d(const char msg[], __m256d a = _mm256_setzero_pd(),
                 bool novalue = false) {
  return;
  printf("%s: ", msg);
  if (novalue) {
    printf("\n");
    return;
  }
  for (int i = 0; i < 4; i++) {
    printf("%+.13a ", a[i]);
  }
  printf("\n");
}

void debug_m128d(const char msg[], __m128d a = _mm_setzero_pd(),
                 bool novalue = false) {
  return;
  printf("%s: ", msg);
  if (novalue) {
    printf("\n");
    return;
  }
  for (int i = 0; i < 2; i++) {
    printf("%+.13a ", a[i]);
  }
  printf("\n");
}

void debug(const char msg[]) { debug_m256i(msg, _mm256_setzero_si256(), true); }

/* The current state of the generators. */
typedef struct {
  __m256i s[4];
} state;

__attribute__((target("avx2"))) static __inline __m256i rotl(__m256i x, int k) {
  return _mm256_or_si256(_mm256_slli_epi64(x, k), _mm256_srli_epi64(x, 64 - k));
}

__attribute__((target("avx2"))) static inline __m256i next(state &s) {
  // result[i] = s[0][i] + s[3][i];
  __m256i result = _mm256_add_epi64(s.s[0], s.s[3]);

  // t[i] = s[1][i] << 17;
  __m256i t = _mm256_slli_epi64(s.s[1], 17);

  // s[2][i] ^= s[0][i];
  s.s[2] = _mm256_xor_si256(s.s[2], s.s[0]);

  // s[3][i] ^= s[1][i];
  s.s[3] = _mm256_xor_si256(s.s[3], s.s[1]);

  // s[1][i] ^= s[2][i];
  s.s[1] = _mm256_xor_si256(s.s[1], s.s[2]);

  // s[0][i] ^= s[3][i];
  s.s[0] = _mm256_xor_si256(s.s[0], s.s[3]);

  // s[2][i] ^= t[i];
  s.s[2] = _mm256_xor_si256(s.s[2], t);

  // s[3][i] = rotl(s[3][i], 45);
  s.s[3] = rotl(s.s[3], 45);

  return result;
}

__m256d get_rand_double01(state &s) {
  __m256i rng = next(s);
  debug_m256i("rng", rng);
  __m256i shift = _mm256_srli_epi64(rng, 11);
  __m128i shift_lo = _mm256_extractf128_si256(shift, 0);
  __m128i shift_hi = _mm256_extractf128_si256(shift, 1);
  // convert to double
  debug_m128i("shift lo", shift_lo);
  debug_m128i("shift hi", shift_hi);
  debug_m128d("shiftd lo", _mm_castsi128_pd(shift_lo));
  debug_m128d("shiftd hi", _mm_castsi128_pd(shift_hi));
  __m256d x = _mm256_set_pd(shift_hi[1], shift_hi[0], shift_lo[1], shift_lo[0]);
  debug_m256d("x", x * 0x1.0p-53);
  return x * 0x1.0p-53;
}

__attribute__((target("avx2"))) void
init(state &s, const std::array<uint64_t, 4> &seeds) {
  s.s[0] = _mm256_set_epi64x(seeds[0], seeds[1], seeds[2], seeds[3]);
  s.s[1] = _mm256_setzero_si256();
  s.s[2] = _mm256_setzero_si256();
  s.s[3] = _mm256_setzero_si256();
}

} // namespace avx2

namespace avx512 {
typedef __m512i rn_uint64_x8;

/* The current state of the generators. */
typedef struct {
  __m512i s[4];
} state;

__attribute__((target("avx512f"))) static __inline __m512i rotl(__m512i x,
                                                                int k) {
  return _mm512_or_si512(_mm512_slli_epi64(x, k), _mm512_srli_epi64(x, 64 - k));
}

__attribute__((target("avx512f"))) static inline __m512i next(state &s) {
  // result[i] = s[0][i] + s[3][i];
  __m512i result = _mm512_add_epi64(s.s[0], s.s[3]);

  // t[i] = s[1][i] << 17;
  __m512i t = _mm512_slli_epi64(s.s[1], 17);

  // s[2][i] ^= s[0][i];
  s.s[2] = _mm512_xor_si512(s.s[2], s.s[0]);

  // s[3][i] ^= s[1][i];
  s.s[3] = _mm512_xor_si512(s.s[3], s.s[1]);

  // s[1][i] ^= s[2][i];
  s.s[1] = _mm512_xor_si512(s.s[1], s.s[2]);

  // s[0][i] ^= s[3][i];
  s.s[0] = _mm512_xor_si512(s.s[0], s.s[3]);

  // s[2][i] ^= t[i];
  s.s[2] = _mm512_xor_si512(s.s[2], t);

  // s[3][i] = rotl(s[3][i], 45);
  s.s[3] = rotl(s.s[3], 45);

  return result;
}

__attribute__((target("avx512f"))) void
init(state &s, const std::array<uint64_t, 8> &seeds) {
  s.s[0] = _mm512_set_epi64(seeds[0], seeds[1], seeds[2], seeds[3], seeds[4],
                            seeds[5], seeds[6], seeds[7]);
  s.s[1] = _mm512_setzero_si512();
  s.s[2] = _mm512_setzero_si512();
  s.s[3] = _mm512_setzero_si512();
}

} // namespace avx512

} // namespace xoroshiro256plus
#endif // __VERIFICARLO_SRLIB_RAND_XOROSHIRO256P_HPP__