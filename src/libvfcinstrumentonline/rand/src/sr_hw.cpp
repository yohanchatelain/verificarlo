
// clang-format off
#include "hwy/highway.h"
#include "src/sr_hw.h"
#include "src/sr_hw-inl.h"
// clang-format on

// Optional, can instead add HWY_ATTR to all functions.
HWY_BEFORE_NAMESPACE();

namespace sr {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

template <typename T>
void _round(const T *HWY_RESTRICT sigma, const T *HWY_RESTRICT tau,
            T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const D d;
  const size_t N = hn::Lanes(d);
  for (size_t i = 0; i + N <= count; i += N) {
    auto sigma_vec = hn::Load(d, sigma + i);
    auto tau_vec = hn::Load(d, tau + i);
    auto res = sr_round(sigma_vec, tau_vec);
    hn::Store(res, d, result + i);
  }
}

template <typename T>
void _add(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
          T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const D d;
  const size_t N = hn::Lanes(d);
  for (size_t i = 0; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto b_vec = hn::Load(d, b + i);
    auto res = sr_add<D>(a_vec, b_vec);
    hn::Store(res, d, result + i);
  }
}

template <typename T>
void _mul(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
          T *HWY_RESTRICT result, const size_t count) {
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);
  for (size_t i = 0; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto b_vec = hn::Load(d, b + i);
    auto res = sr_mul(a_vec, b_vec);
    hn::Store(res, d, result + i);
  }
}

template <typename T>
void _div(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
          T *HWY_RESTRICT result, const size_t count) {
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);
  for (size_t i = 0; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto b_vec = hn::Load(d, b + i);
    auto res = sr_div(a_vec, b_vec);
    hn::Store(res, d, result + i);
  }
}

template <typename T>
void _sqrt(const T *HWY_RESTRICT a, T *HWY_RESTRICT result,
           const size_t count) {
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);
  for (size_t i = 0; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto res = sr_sqrt(a_vec);
    hn::Store(res, d, result + i);
  }
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
} // namespace sr
HWY_AFTER_NAMESPACE();

namespace sr {

template <typename T>
HWY_DLLEXPORT void round(const T *HWY_RESTRICT sigma, const T *HWY_RESTRICT tau,
                         T *HWY_RESTRICT result, const size_t count) {
  return HWY_STATIC_DISPATCH(_round)(sigma, tau, result, count);
}

template <typename T>
HWY_DLLEXPORT void add(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  return HWY_STATIC_DISPATCH(_add)(a, b, result, count);
}

template <typename T>
HWY_DLLEXPORT void mul(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  return HWY_STATIC_DISPATCH(_mul)(a, b, result, count);
}

template <typename T>
HWY_DLLEXPORT void div(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  return HWY_STATIC_DISPATCH(_div)(a, b, result, count);
}

template <typename T>
HWY_DLLEXPORT void sqrt(const T *HWY_RESTRICT a, T *HWY_RESTRICT result,
                        const size_t count) {
  return HWY_STATIC_DISPATCH(_sqrt)(a, result);
}

} // namespace sr

int _main() {
  float a[] = {1.0f, 1.0f, 1.0f, 1.0f};
  float b[] = {0x1.0p-24f, 0x1.0p-24f, 0x1.0p-24f, 0x1.0p-24f};
  float c[4] = {0, 0, 0, 0};
  sr::add<float>(a, b, c, 4);
  for (int i = 0; i < 4; i++) {
    printf("%a\n", c[i]);
  }
  return 0;
}