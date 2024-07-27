#ifndef SHISHUA_AVX2_H
#define SHISHUA_AVX2_H

#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct prng_state {
  __m256i state[4];
  __m256i output[4];
  __m256i counter;
} prng_state;

uint8_t prng_buf[1024] __attribute__((aligned(32)));

static inline void prng_gen(struct prng_state *s, uint8_t *buf) {
  __m256i o0 = s->output[0], o1 = s->output[1], o2 = s->output[2],
          o3 = s->output[3];
  __m256i s0 = s->state[0], s1 = s->state[1], s2 = s->state[2],
          s3 = s->state[3];
  __m256i counter = s->counter;

  const __m256i shu0 = _mm256_set_epi32(4, 3, 2, 1, 0, 7, 6, 5);
  const __m256i shu1 = _mm256_set_epi32(2, 1, 0, 7, 6, 5, 4, 3);
  const __m256i increment = _mm256_set_epi64x(1, 3, 5, 7);

  size_t i = 0;
  for (; i + 128 <= 1024; i += 128) {
    _mm256_store_si256((__m256i *)&buf[i], o0);
    _mm256_store_si256((__m256i *)&buf[i + 32], o1);
    _mm256_store_si256((__m256i *)&buf[i + 64], o2);
    _mm256_store_si256((__m256i *)&buf[i + 96], o3);

    s1 = _mm256_add_epi64(s1, counter);
    s3 = _mm256_add_epi64(s3, counter);
    counter = _mm256_add_epi64(counter, increment);

    __m256i u0 = _mm256_srli_epi64(s0, 1);
    __m256i u1 = _mm256_srli_epi64(s1, 3);
    __m256i u2 = _mm256_srli_epi64(s2, 1);
    __m256i u3 = _mm256_srli_epi64(s3, 3);

    __m256i t0 = _mm256_permutevar8x32_epi32(s0, shu0);
    __m256i t1 = _mm256_permutevar8x32_epi32(s1, shu1);
    __m256i t2 = _mm256_permutevar8x32_epi32(s2, shu0);
    __m256i t3 = _mm256_permutevar8x32_epi32(s3, shu1);

    s0 = _mm256_add_epi64(t0, u0);
    s1 = _mm256_add_epi64(t1, u1);
    s2 = _mm256_add_epi64(t2, u2);
    s3 = _mm256_add_epi64(t3, u3);

    o0 = _mm256_xor_si256(u0, t1);
    o1 = _mm256_xor_si256(u2, t3);
    o2 = _mm256_xor_si256(s0, s3);
    o3 = _mm256_xor_si256(s2, s1);
  }

  s->output[0] = o0;
  s->output[1] = o1;
  s->output[2] = o2;
  s->output[3] = o3;
  s->state[0] = s0;
  s->state[1] = s1;
  s->state[2] = s2;
  s->state[3] = s3;
  s->counter = counter;

  if (i < 1024) {
    uint8_t temp[128] __attribute__((aligned(32)));
    _mm256_store_si256((__m256i *)&temp[0], o0);
    _mm256_store_si256((__m256i *)&temp[32], o1);
    _mm256_store_si256((__m256i *)&temp[64], o2);
    _mm256_store_si256((__m256i *)&temp[96], o3);
    memcpy(buf + i, temp, 1024 - i);
  }
}

uint64_t prng_uint64(struct prng_state *s) {
  uint64_t result;
  prng_gen(s, prng_buf);
  memcpy(&result, prng_buf, sizeof(result));
  return result;
}

void prng_init(struct prng_state *s, const uint64_t seed[4]) {
  static const uint64_t phi[16] __attribute__((aligned(32))) = {
      0x9E3779B97F4A7C15, 0xF39CC0605CEDC834, 0x1082276BF3A27251,
      0xF86C6A11D0C18E95, 0x2767F0B153D27B7F, 0x0347045B5BF1827F,
      0x01886F0928403002, 0xC1D64BA40F335E36, 0xF06AD7AE9717877E,
      0x85839D6EFFBD7DC6, 0x64D325D1C5371682, 0xCADD0CCCFDFFBBE1,
      0x626E33B8D04B4331, 0xBBF73C790D94F79D, 0x471C4AB3ED3D82A5,
      0xFEC507705E4AE6E5,
  };

  s->state[0] = _mm256_xor_si256(_mm256_load_si256((__m256i *)&phi[0]),
                                 _mm256_set_epi64x(0, seed[1], 0, seed[0]));
  s->state[1] = _mm256_xor_si256(_mm256_load_si256((__m256i *)&phi[4]),
                                 _mm256_set_epi64x(0, seed[3], 0, seed[2]));
  s->state[2] = _mm256_xor_si256(_mm256_load_si256((__m256i *)&phi[8]),
                                 _mm256_set_epi64x(0, seed[3], 0, seed[2]));
  s->state[3] = _mm256_xor_si256(_mm256_load_si256((__m256i *)&phi[12]),
                                 _mm256_set_epi64x(0, seed[1], 0, seed[0]));

  s->counter = _mm256_setzero_si256();

  for (int i = 0; i < 13; ++i) {
    prng_gen(s, prng_buf);
    s->state[0] = s->output[3];
    s->state[1] = s->output[2];
    s->state[2] = s->output[1];
    s->state[3] = s->output[0];
  }
}

#endif