/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *                                                                           *\
 ****************************************************************************/
/*
 * Unbiased companions of the BITMASK 'rand' (XOR) operator.
 *
 * Notation: |x| = S * 2^E with S the integer significand (p bits for normal
 * numbers, fewer for subnormals), t the virtual precision counted from the
 * leading one, n = bits(S) - t the number of perturbed bits, N = 2^n.
 *
 *  rand (XOR)  : S' = H + (L xor R)        E[S'] = H + (N-1)/2   (biased)
 *  center      : S' = S + sext_k(R)*2^(n-k) + B*2^(n-k)
 *                                          E[S'] = S             (unbiased)
 *  center-t    : as center, B taken from a per-thread toggle: the
 *                expectation alternates +-2^(n-k-1); zero over any two
 *                consecutive operations (amortized)
 *  sr          : S' = H + N * carry(L + R) E[S'] = S             (unbiased)
 *                with k < n random bits:   E[S'] = S - (L mod 2^(n-k))
 *
 * k = min(n, random_bits) is the number of random bits per operation
 * (random_bits <= 0 means k = n). Random bits are drawn from a 64-bit
 * per-thread reservoir so that an operation consumes exactly the bits it
 * uses (k+1 for center, k for sr).
 */
#ifndef __BITMASK_UNBIASED_H__
#define __BITMASK_UNBIASED_H__

#include <stdint.h>

/* --- per-thread random-bit reservoir ---------------------------------- */
static TLS uint64_t _rb_pool = 0;
static TLS int _rb_left = 0;
static TLS uint64_t _rb_drawn_words = 0; /* statistics: 64-bit draws */
static TLS unsigned _toggle = 0;
static TLS uint64_t _rb_consumed = 0;  /* statistics: random bits consumed */

static inline uint64_t _rand_bits(int k) {
  /* returns k uniform random bits, 0 <= k <= 63 */
  if (k <= 0)
    return 0;
  if (_rb_left < k) {
    /* discard the remainder: keeps every draw independent and simple */
    _rb_pool = get_random_mask();
    _rb_left = 64;
    _rb_drawn_words++;
  }
  const uint64_t v = _rb_pool & ((1ULL << k) - 1);
  _rb_pool >>= k;
  _rb_left -= k;
  _rb_consumed += k;
  return v;
}


/* k <= 63 bits made available without consuming them (lazy samplers) */
static inline uint64_t _peek_bits(int k) {
  if (_rb_left < k) {
    _rb_pool = get_random_mask();
    _rb_left = 64;
    _rb_drawn_words++;
  }
  return _rb_pool & ((1ULL << k) - 1);
}
static inline void _consume_bits(int j) {
  _rb_pool >>= j;
  _rb_left -= j;
  _rb_consumed += j;
}

/* bit-reverse the n low bits of v (n >= 1) */
static inline uint64_t _rev_n(uint64_t v, int n) {
  v = ((v >> 1) & 0x5555555555555555ULL) | ((v & 0x5555555555555555ULL) << 1);
  v = ((v >> 2) & 0x3333333333333333ULL) | ((v & 0x3333333333333333ULL) << 2);
  v = ((v >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((v & 0x0F0F0F0F0F0F0F0FULL) << 4);
  v = __builtin_bswap64(v);
  return v >> (64 - n);
}

/* sign-extend the k-bit field v (k >= 1) */
static inline int64_t _sext(uint64_t v, int k) {
  return (int64_t)v - (int64_t)((v >> (k - 1)) & 1ULL) * (int64_t)(1ULL << k);
}

/* Build the float sign * S * 2^E exactly. S > 0, S < 2^(PREC+1) and, when
 * S >= 2^PREC, S is even. E >= the subnormal quantum exponent, so the value
 * is always a multiple of the smallest subnormal: the result is exact unless
 * it overflows (then +-inf). */
#define DEFINE_COMPOSE(NAME, UT, PREC, BIAS_P, EMAXF)                         \
  static inline UT NAME(UT sign, int64_t S, int E) {                          \
    int nb = 64 - __builtin_clzll((uint64_t)S);                               \
    if (nb > (PREC)) { /* even by construction */                            \
      S >>= 1;                                                                \
      E += 1;                                                                 \
      nb -= 1;                                                                \
    }                                                                         \
    const int sh = (PREC)-nb;                                                 \
    S <<= sh;                                                                 \
    E -= sh;                                                                  \
    int be = E + (BIAS_P); /* biased exponent of the leading one */           \
    if (be >= (EMAXF))                                                        \
      return sign | ((UT)(EMAXF) << ((PREC)-1)); /* infinity */               \
    if (be <= 0) {                             /* subnormal: exact shift */   \
      S >>= (1 - be);                                                         \
      return sign | (UT)S;                                                    \
    }                                                                         \
    return sign | ((UT)be << ((PREC)-1)) |                                    \
           ((UT)S & (((UT)1 << ((PREC)-1)) - 1));                             \
  }
DEFINE_COMPOSE(_compose_b64, uint64_t, 53, 1075, 2047)
DEFINE_COMPOSE(_compose_b32, uint32_t, 24, 150, 255)

/* Decompose |x| (finite, nonzero) into S, E; returns bits(S). */
#define DEFINE_DECOMPOSE(NAME, UT, PMAN, EBIAS_P)                             \
  static inline int NAME(UT u, int64_t *S, int *E) {                          \
    const UT ef = (u >> (PMAN)) & (((UT)1 << (sizeof(UT) * 8 - 1 - (PMAN))) - \
                                   1);                                        \
    const UT mf = u & (((UT)1 << (PMAN)) - 1);                                \
    if (ef) {                                                                 \
      *S = (int64_t)(mf | ((UT)1 << (PMAN)));                                 \
      *E = (int)ef - (EBIAS_P);                                               \
      return (PMAN) + 1;                                                      \
    }                                                                         \
    *S = (int64_t)mf;                                                         \
    *E = 1 - (EBIAS_P);                                                       \
    return 64 - __builtin_clzll((uint64_t)mf);                                \
  }
DEFINE_DECOMPOSE(_decompose_b64, uint64_t, 52, 1075)
DEFINE_DECOMPOSE(_decompose_b32, uint32_t, 23, 150)

/* center: zero-mean dither of amplitude ulp_t/2 (discrete MCA-RR) */
#define DEFINE_CENTER(NAME, FT, UT, PMAN, DECOMP, COMPOSE, UNION, FIELD)      \
  static inline FT NAME(FT x, int t, int rbits, int toggle) {                 \
    UNION b = {.FIELD = x};                                                   \
    const UT sign = b.u & ((UT)1 << (sizeof(UT) * 8 - 1));                    \
    int64_t S;                                                                \
    int E;                                                                    \
    const int nb = DECOMP(b.u ^ sign, &S, &E);                                \
    const int n = nb - t;                                                     \
    if (n <= 0)                                                               \
      return x;                                                               \
    const int k = (rbits > 0 && rbits < n) ? rbits : n;                       \
    const uint64_t R = _rand_bits(toggle ? k : k + 1);                        \
    const int64_t B = toggle ? (int64_t)((_toggle++) & 1U)                    \
                             : (int64_t)((R >> k) & 1ULL);                    \
    const int64_t Z = (_sext(R & ((1ULL << k) - 1), k) + B) * (1LL << (n - k)); \
    int64_t S2 = S + Z;                                                       \
    const int64_t TOP = (int64_t)1 << (PMAN + 1);                             \
    if (S2 >= TOP && (S2 & 1)) /* crossing up: unbiased SR of the last bit */ \
      S2 += _rand_bits(1) ? 1 : -1;                                           \
    b.u = COMPOSE(sign, S2, E);                                               \
    return b.FIELD;                                                           \
  }
DEFINE_CENTER(_center_binary64, double, uint64_t, 52, _decompose_b64,
              _compose_b64, binary64, f64)
DEFINE_CENTER(_center_binary32, float, uint32_t, 23, _decompose_b32,
              _compose_b32, binary32, f32)

/* sr: stochastic rounding of |x| to t significant bits, by carry-out of
 * (L + R). Integer addition on the encoding carries correctly into the
 * exponent field (binade change, subnormal->normal, overflow to inf). */
#define DEFINE_SR(NAME, FT, UT, PMAN, UNION, FIELD)                           \
  static inline FT NAME(FT x, int t, int rbits) {                             \
    UNION b = {.FIELD = x};                                                   \
    const UT sign = b.u & ((UT)1 << (sizeof(UT) * 8 - 1));                    \
    UT u = b.u ^ sign;                                                        \
    const UT ef = u >> (PMAN);                                                \
    const int nb = ef ? (PMAN) + 1                                            \
                      : 64 - __builtin_clzll((uint64_t)(u));                  \
    const int n = nb - t;                                                     \
    if (n <= 0)                                                               \
      return x;                                                               \
    const UT lowmask = ((UT)1 << n) - 1;                                      \
    const UT L = u & lowmask;                                                 \
    UT carry;                                                                 \
    if (rbits > 0 && rbits < n) {                                             \
      const int k = rbits;                                                    \
      carry = ((L >> (n - k)) + (UT)_rand_bits(k)) >> k;                      \
    } else {                                                                  \
      carry = (L + (UT)_rand_bits(n)) >> n;                                   \
    }                                                                         \
    u = (u & ~lowmask) + (carry << n);                                        \
    b.u = sign | u;                                                           \
    return b.FIELD;                                                           \
  }
DEFINE_SR(_sr_binary64, double, uint64_t, 52, binary64, f64)
DEFINE_SR(_sr_binary32, float, uint32_t, 23, binary32, f32)

/* sr-lazy: exact stochastic rounding to t bits, consuming on average fewer
 * than 2 random bits.  Round up iff U < L for U uniform on n bits; U is
 * revealed MSB first and the comparison stops at the first bit where U and
 * L differ (Knuth-Yao style lazy sampling).  Bits after that point are not
 * examined and stay in the reservoir for the next operation, so the
 * decisions remain independent.  P(up) = L / 2^n exactly; expected bits
 * consumed = 2 - 2^(1-n). */
#define DEFINE_SR_LAZY(NAME, FT, UT, PMAN, UNION, FIELD)                      \
  static inline FT NAME(FT x, int t) {                                        \
    UNION b = {.FIELD = x};                                                   \
    const UT sign = b.u & ((UT)1 << (sizeof(UT) * 8 - 1));                    \
    UT u = b.u ^ sign;                                                        \
    const UT ef = u >> (PMAN);                                                \
    const int nb = ef ? (PMAN) + 1 : 64 - __builtin_clzll((uint64_t)(u));     \
    const int n = nb - t;                                                     \
    if (n <= 0)                                                               \
      return x;                                                               \
    const UT lowmask = ((UT)1 << n) - 1;                                      \
    const uint64_t L = (uint64_t)(u & lowmask);                               \
    if (L == 0)                                                               \
      return x;                                                               \
    const uint64_t Lr = _rev_n(L, n); /* MSB of L first */                    \
    const uint64_t d = (Lr ^ _peek_bits(n)) & ((1ULL << n) - 1);              \
    UT carry;                                                                 \
    if (d == 0) { /* U == L: not below, do not round up */                    \
      carry = 0;                                                              \
      _consume_bits(n);                                                       \
    } else {                                                                  \
      const int j = __builtin_ctzll(d); /* first differing bit */             \
      carry = (UT)((Lr >> j) & 1ULL);   /* L has 1, U has 0 -> U < L */       \
      _consume_bits(j + 1);                                                   \
    }                                                                         \
    u = (u & ~lowmask) + (carry << n);                                        \
    b.u = sign | u;                                                           \
    return b.FIELD;                                                           \
  }
DEFINE_SR_LAZY(_sr_lazy_binary64, double, uint64_t, 52, binary64, f64)
DEFINE_SR_LAZY(_sr_lazy_binary32, float, uint32_t, 23, binary32, f32)

/* exported for measurements: random bits consumed by this thread */
uint64_t bitmask_random_bits_consumed(void) { return _rb_consumed; }
uint64_t bitmask_random_words_drawn(void) { return _rb_drawn_words; }

#endif /* __BITMASK_UNBIASED_H__ */
