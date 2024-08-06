#ifndef __VERIFICARLO_SRLIB_RAND_VECTOR_TYPES_HPP__
#define __VERIFICARLO_SRLIB_RAND_VECTOR_TYPES_HPP__

#include <immintrin.h>
#include <stdint.h>

namespace scalar {
typedef union {
  float f[2];
  double d;
  uint64_t u64;
  int64_t i64;
  uint32_t u32[2];
  int32_t i32[2];
} double_t;
typedef double_t floatx2_t;
typedef double_t uint64_t;
typedef double_t int64_t;
typedef double_t uint32x2_t;
typedef double_t int32x2_t;
typedef double_t boolx2_t;

floatx2_t add(floatx2_t a,
              floatx2_t b){return {.f = {a.f[0] + b.f[0], a.f[1] + b.f[1]}}};
boolx2_t cmplt(floatx2_t a, floatx2_t b) {
  return {.u32 = {a.f[0] < b.f[0], a.f[1] < b.f[1]}};
}
floatx2_t blendv(floatx2_t a, floatx2_t b, boolx2_t mask) {
  return {.f = {mask.u32[0] ? b.f[0] : a.f[0], mask.u32[1] ? b.f[1] : a.f[1]}};
}
boolx2_t bitwise_xor(boolx2_t a, boolx2_t b) {
  return {.u32 = {a.u32[0] ^ b.u32[0], a.u32[1] ^ b.u32[1]}};
}

floatx2_t mul(floatx2_t a, floatx2_t b) {
  return {.f = {a.f[0] * b.f[0], a.f[1] * b.f[1]}};

} // namespace scalar

namespace sse4_2 {
typedef __m128i boolx2_t;
typedef __m128i int64x2_t;
typedef __m128i uint64x2_t;
typedef __m128i int32x4_t;
typedef __m128i uint32x4_t;
typedef __m128 floatx4_t;
typedef __m128d doublex2_t;

constexpr floatx4_t floatx4_0 = {0.0f, 0.0f, 0.0f, 0.0f};
constexpr doublex2_t doublex2_0 = {0.0, 0.0};

floatx4_t bitwise_and(floatx4_t a, floatx4_t b) { return _mm_and_ps(a, b); };
floatx4_t bitwise_or(floatx4_t a, floatx4_t b) { return _mm_or_ps(a, b); };
floatx4_t bitwise_xor(floatx4_t a, floatx4_t b) { return _mm_xor_ps(a, b); };
floatx4_t bitwise_not(floatx4_t a) {
  return _mm_xor_ps(a, _mm_castsi128_ps(_mm_set1_epi32(-1)));
};
floatx4_t bitwise_andnot(floatx4_t a, floatx4_t b) {
  return _mm_andnot_ps(a, b);
};

int32x4_t eq(floatx4_t a, floatx4_t b) { return _mm_cmpeq_ps(a, b); };
int64x2_t eq(doublex2_t a, doublex2_t b) { return _mm_cmpeq_pd(a, b); };
int32x4_t eq0(floatx4_t a) { return _mm_cmpeq_ps(a, floatx4_0); };
int64x2_t eq0(doublex2_t a) { return _mm_cmpeq_pd(a, doublex2_0); };

int32x4_t neqf(floatx4_t a, floatx4_t b) { return _mm_cmpneq_ps(a, b); };
int64x2_t neqd(doublex2_t a, doublex2_t b) { return _mm_cmpneq_pd(a, b); };
int32x4_t neq0f(floatx4_t a) { return _mm_cmpneq_ps(a, floatx4_0); };
int64x2_t neq0d(doublex2_t a) { return _mm_cmpneq_pd(a, doublex2_0); };

int not0(int64x2_t a) { return _mm_movemask_pd(a) == 0; };

doublex2_t abs(doublex2_t a) {
  return _mm_and_pd(a, _mm_castsi128_pd(_mm_set1_epi64x(0x7FFFFFFFFFFFFFFF)));
};

doublex2_t add(doublex2_t a, doublex2_t b) { return _mm_add_pd(a, b); };

} // namespace sse4_2

namespace avx {
typedef __m128i uint64x2_t;
typedef __m128i uint32x4_t;
typedef __m128 floatx4_t;
typedef __m128d doublex2_t;

/* The current state of the generators. */
typedef struct {
  __m128i s[4];
} state;

} // namespace avx

namespace avx2 {
typedef __m256i uint64x4_t;
typedef __m256i uint32x8_t;
typedef __m256 floatx8_t;
typedef __m256d doublex4_t;

/* The current state of the generators. */
typedef struct {
  __m256i s[4];
} state;

} // namespace avx2

namespace avx512 {
typedef __m512i uint64x8_t;
typedef __m512i uint32x16_t;
typedef __m512 floatx16_t;
typedef __m512d doublex8_t;

/* The current state of the generators. */
typedef struct {
  __m512i s[4];
} state;

} // namespace avx512

#endif // __VERIFICARLO_SRLIB_RAND_VECTOR_TYPES_HPP__