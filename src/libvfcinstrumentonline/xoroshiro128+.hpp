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

void xoroshiro128plus_avx2_init(xoroshiro128plus_avx2_state *state,
                                uint64_t seed);
__m256i xoroshiro128plus_avx2_next(xoroshiro128plus_avx2_state *state);