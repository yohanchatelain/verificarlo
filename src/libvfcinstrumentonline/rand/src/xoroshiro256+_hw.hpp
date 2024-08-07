#ifndef __VERIFICARLO_SRLIB_RAND_XOROSHIRO256P_HPP__
#define __VERIFICARLO_SRLIB_RAND_XOROSHIRO256P_HPP__

#include <stdio.h>

#include <array>
#include <immintrin.h>
#include <stdint.h>

#include "vector_types.hpp"

#include "hwy/highway.h"

HWY_BEFORE_NAMESPACE(); // at file scope
namespace HWY_NAMESPACE {
namespace xoroshiro256plus {

namespace scalar {

using D = ScalableTag<uint64_t>;
const D d;

struct state {
  hn::Vec<D> s[4];
};

static inline hn::Vec<D> rotl(hn::Vec<D> x, int k) {
  return hn::Or(hn::ShiftLeft<D>()(x, k), hn::ShiftRight<D>()(x, 64 - k));
}

static inline hn::Vec<D> next(state &s) {
  hn::Vec<D> result = hn::Add(s.s[0], s.s[3]);
  hn::Vec<D> t = hn::ShiftLeft<D>()(s.s[1], 17);

  s.s[2] = hn::Xor(s.s[2], s.s[0]);
  s.s[3] = hn::Xor(s.s[3], s.s[1]);
  s.s[1] = hn::Xor(s.s[1], s.s[2]);
  s.s[0] = hn::Xor(s.s[0], s.s[3]);
  s.s[2] = hn::Xor(s.s[2], t);
  s.s[3] = rotl(s.s[3], 45);

  return result;
}

void init(state &s, const hn::Vec<D> &seed) {
  s.s[0] = seed;
  s.s[1] = hn::Zero(d);
  s.s[2] = hn::Zero(d);
  s.s[3] = hn::Zero(d);
}
} // namespace scalar

} // namespace xoroshiro256plus

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

#endif // __VERIFICARLO_SRLIB_RAND_XOROSHIRO256P_HPP__