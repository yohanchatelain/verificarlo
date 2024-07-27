#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATE_SIZE 2

typedef struct {
  __m256i s[STATE_SIZE];
} xoroshiro128plus_avx2_state;

#define __INTERNAL_RNG_STATE xoroshiro128plus_avx2_state

// Function to rotate left (vectorized)
static inline __m256i rotl_avx2(__m256i x, int k) {
  return _mm256_or_si256(_mm256_slli_epi64(x, k), _mm256_srli_epi64(x, 64 - k));
}

// XOROSHIRO128++ next function (vectorized)
__m256i xoroshiro128plus_avx2_next(xoroshiro128plus_avx2_state *state) {
  const __m256i s0 = state->s[0];
  __m256i s1 = state->s[1];
  const __m256i result =
      _mm256_add_epi64(rotl_avx2(_mm256_add_epi64(s0, s1), 17), s0);

  s1 = _mm256_xor_si256(s1, s0);
  state->s[0] = _mm256_xor_si256(_mm256_xor_si256(rotl_avx2(s0, 49), s1),
                                 _mm256_slli_epi64(s1, 21));
  state->s[1] = rotl_avx2(s1, 28);

  return result;
}

// Function to initialize the state
void xoroshiro128plus_avx2_init(xoroshiro128plus_avx2_state *state,
                                uint64_t seed) {
  uint64_t temp_state[4][2];
  for (int i = 0; i < 4; i++) {
    temp_state[i][0] = seed + i;
    temp_state[i][1] = (seed + i) ^ 0x1234567890abcdefULL;
  }

  state->s[0] = _mm256_loadu_si256((__m256i *)&temp_state[0][0]);
  state->s[1] = _mm256_loadu_si256((__m256i *)&temp_state[0][1]);

  // Warm up the state
  for (int i = 0; i < 100; i++) {
    xoroshiro128plus_avx2_next(state);
  }
}

// Generate random numbers using AVX2
void generate_random_numbers_avx2(uint64_t *output, size_t count) {
  xoroshiro128plus_avx2_state state;
  xoroshiro128plus_avx2_init(&state, 12345);

  size_t i = 0;
  for (; i + 4 <= count; i += 4) {
    __m256i result = xoroshiro128plus_avx2_next(&state);
    _mm256_storeu_si256((__m256i *)&output[i], result);
  }

  // Handle any remaining elements
  if (i < count) {
    __m256i result = xoroshiro128plus_avx2_next(&state);
    uint64_t temp[4];
    _mm256_storeu_si256((__m256i *)temp, result);
    memcpy(&output[i], temp, (count - i) * sizeof(uint64_t));
  }
}

#ifdef XOROSHIRO_TEST
int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <count>\n", argv[0]);
    return 1;
  }

  // read count from input
  const size_t count = atoi(argv[1]);

  uint64_t *random_numbers =
      (uint64_t *)aligned_alloc(32, count * sizeof(uint64_t));

  if (!random_numbers) {
    fprintf(stderr, "Memory allocation failed\n");
    return 1;
  }

  generate_random_numbers_avx2(random_numbers, count);

  printf("Generated %zu random numbers\n", count);

  // Print first few numbers as a sample
  for (int i = 0; i < 10; i++) {
    printf("%lu ", random_numbers[i]);
  }
  printf("\n");

  free(random_numbers);
  return 0;
}
#endif