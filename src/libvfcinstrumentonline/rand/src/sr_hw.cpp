// Generates code for every target that this compiler can support.
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "src/sr_hw.cpp" // this file
#include "hwy/foreach_target.h"            // must come before highway.h

// clang-format off
#include "hwy/highway.h"
#include "src/sr_hw-inl.h"
// clang-format on

// Optional, can instead add HWY_ATTR to all functions.
HWY_BEFORE_NAMESPACE();

namespace sr {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

namespace internal {

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
V _add_fp_xN(V a, V b) {
  auto res = sr_add<D>(a, b);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
V _sub_fp_xN(V a, V b) {
  auto res = sr_sub<D>(a, b);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
V _mul_fp_xN(V a, V b) {
  auto res = sr_mul<D>(a, b);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
V _div_fp_xN(V a, V b) {
  auto res = sr_div<D>(a, b);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
V _sqrt_fp_xN(V a) {
  auto res = sr_sqrt<D>(a);
  return res;
}

/* 32-bits */
#if HWY_MAX_BYTES >= 4
using f32x1_t = hn::FixedTag<float, 1>;
using f32x1 = hn::Vec<f32x1_t>;

f32x1 _add_f32_x1(f32x1 a, f32x1 b) { return _add_fp_xN<float, 1>(a, b); }
f32x1 _sub_f32_x1(f32x1 a, f32x1 b) { return _sub_fp_xN<float, 1>(a, b); }
f32x1 _mul_f32_x1(f32x1 a, f32x1 b) { return _mul_fp_xN<float, 1>(a, b); }
f32x1 _div_f32_x1(f32x1 a, f32x1 b) { return _div_fp_xN<float, 1>(a, b); }
f32x1 _sqrt_f32_x1(f32x1 a) { return _sqrt_fp_xN<float, 1>(a); }
#endif

/* 64-bits */
#if HWY_MAX_BYTES >= 8
using f64x1_t = hn::FixedTag<double, 1>;
using f64x1 = hn::Vec<f64x1_t>;
using f32x2_t = hn::FixedTag<float, 2>;
using f32x2 = hn::Vec<f32x2_t>;

f64x1 _add_f64_x1(f64x1 a, f64x1 b) { return _add_fp_xN<double, 1>(a, b); }
f64x1 _sub_f64_x1(f64x1 a, f64x1 b) { return _sub_fp_xN<double, 1>(a, b); }
f64x1 _mul_f64_x1(f64x1 a, f64x1 b) { return _mul_fp_xN<double, 1>(a, b); }
f64x1 _div_f64_x1(f64x1 a, f64x1 b) { return _div_fp_xN<double, 1>(a, b); }
f64x1 _sqrt_f64_x1(f64x1 a) { return _sqrt_fp_xN<double, 1>(a); }

f32x2 _add_f32_x2(f32x2 a, f32x2 b) { return _add_fp_xN<float, 2>(a, b); }
f32x2 _sub_f32_x2(f32x2 a, f32x2 b) { return _sub_fp_xN<float, 2>(a, b); }
f32x2 _mul_f32_x2(f32x2 a, f32x2 b) { return _mul_fp_xN<float, 2>(a, b); }
f32x2 _div_f32_x2(f32x2 a, f32x2 b) { return _div_fp_xN<float, 2>(a, b); }
f32x2 _sqrt_f32_x2(f32x2 a) { return _sqrt_fp_xN<float, 2>(a); }
#endif

/* 128-bits */
#if HWY_MAX_BYTES >= 16
using f64x2_t = hn::FixedTag<double, 2>;
using f64x2 = hn::Vec<f64x2_t>;
using f32x4_t = hn::FixedTag<float, 4>;
using f32x4 = hn::Vec<f32x4_t>;

f64x2 _add_f64_x2(f64x2 a, f64x2 b) { return _add_fp_xN<double, 2>(a, b); }
f64x2 _sub_f64_x2(f64x2 a, f64x2 b) { return _sub_fp_xN<double, 2>(a, b); }
f64x2 _mul_f64_x2(f64x2 a, f64x2 b) { return _mul_fp_xN<double, 2>(a, b); }
f64x2 _div_f64_x2(f64x2 a, f64x2 b) { return _div_fp_xN<double, 2>(a, b); }
f64x2 _sqrt_f64_x2(f64x2 a) { return _sqrt_fp_xN<double, 2>(a); }

f32x4 _add_f32_x4(f32x4 a, f32x4 b) { return _add_fp_xN<float, 4>(a, b); }
f32x4 _sub_f32_x4(f32x4 a, f32x4 b) { return _sub_fp_xN<float, 4>(a, b); }
f32x4 _mul_f32_x4(f32x4 a, f32x4 b) { return _mul_fp_xN<float, 4>(a, b); }
f32x4 _div_f32_x4(f32x4 a, f32x4 b) { return _div_fp_xN<float, 4>(a, b); }
f32x4 _sqrt_f32_x4(f32x4 a) { return _sqrt_fp_xN<float, 4>(a); }

#endif

/* 256-bits */
#if HWY_MAX_BYTES >= 32
using f64x4_t = hn::FixedTag<double, 4>;
using f64x4 = hn::Vec<f64x4_t>;
using f32x8_t = hn::FixedTag<float, 8>;
using f32x8 = hn::Vec<f32x8_t>;

f64x4 _add_f64_x4(f64x4 a, f64x4 b) { return _add_fp_xN<double, 4>(a, b); }
f64x4 _sub_f64_x4(f64x4 a, f64x4 b) { return _sub_fp_xN<double, 4>(a, b); }
f64x4 _mul_f64_x4(f64x4 a, f64x4 b) { return _mul_fp_xN<double, 4>(a, b); }
f64x4 _div_f64_x4(f64x4 a, f64x4 b) { return _div_fp_xN<double, 4>(a, b); }
f64x4 _sqrt_f64_x4(f64x4 a) { return _sqrt_fp_xN<double, 4>(a); }

f32x8 _add_f32_x8(f32x8 a, f32x8 b) { return _add_fp_xN<float, 8>(a, b); }
f32x8 _sub_f32_x8(f32x8 a, f32x8 b) { return _sub_fp_xN<float, 8>(a, b); }
f32x8 _mul_f32_x8(f32x8 a, f32x8 b) { return _mul_fp_xN<float, 8>(a, b); }
f32x8 _div_f32_x8(f32x8 a, f32x8 b) { return _div_fp_xN<float, 8>(a, b); }
f32x8 _sqrt_f32_x8(f32x8 a) { return _sqrt_fp_xN<float, 8>(a); }
#endif

/* 512-bits */
#if HWY_MAX_BYTES >= 64
using f64x8_t = hn::FixedTag<double, 8>;
using f64x8 = hn::Vec<f64x8_t>;
using f32x16_t = hn::FixedTag<float, 16>;
using f32x16 = hn::Vec<f32x16_t>;

f64x8 _add_f64_x8(f64x8 a, f64x8 b) { return _add_fp_xN<double, 8>(a, b); }
f64x8 _sub_f64_x8(f64x8 a, f64x8 b) { return _sub_fp_xN<double, 8>(a, b); }
f64x8 _mul_f64_x8(f64x8 a, f64x8 b) { return _mul_fp_xN<double, 8>(a, b); }
f64x8 _div_f64_x8(f64x8 a, f64x8 b) { return _div_fp_xN<double, 8>(a, b); }
f64x8 _sqrt_f64_x8(f64x8 a) { return _sqrt_fp_xN<double, 8>(a); }

f32x16 _add_f32_x16(f32x16 a, f32x16 b) { return _add_fp_xN<float, 16>(a, b); }
f32x16 _sub_f32_x16(f32x16 a, f32x16 b) { return _sub_fp_xN<float, 16>(a, b); }
f32x16 _mul_f32_x16(f32x16 a, f32x16 b) { return _mul_fp_xN<float, 16>(a, b); }
f32x16 _div_f32_x16(f32x16 a, f32x16 b) { return _div_fp_xN<float, 16>(a, b); }
f32x16 _sqrt_f32_x16(f32x16 a) { return _sqrt_fp_xN<float, 16>(a); }
#endif

} // namespace internal

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _addxN(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
            T *HWY_RESTRICT c) {
  const D d;
  const auto va = hn::Load(d, a);
  const auto vb = hn::Load(d, b);
  auto res = internal::_add_fp_xN<T, N>(va, vb);
  hn::Store(res, d, c);
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _subxN(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
            T *HWY_RESTRICT c) {
  const D d;
  const auto va = hn::Load(d, a);
  const auto vb = hn::Load(d, b);
  auto res = internal::_sub_fp_xN<T, N>(va, vb);
  hn::Store(res, d, c);
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _mulxN(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
            T *HWY_RESTRICT c) {
  const D d;
  const auto va = hn::Load(d, a);
  const auto vb = hn::Load(d, b);
  auto res = internal::_mul_fp_xN<T, N>(va, vb);
  hn::Store(res, d, c);
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _divxN(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
            T *HWY_RESTRICT c) {
  const D d;
  const auto va = hn::Load(d, a);
  const auto vb = hn::Load(d, b);
  auto res = internal::_div_fp_xN<T, N>(va, vb);
  hn::Store(res, d, c);
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _sqrtxN(const T *HWY_RESTRICT a, T *HWY_RESTRICT c) {
  const D d;
  const auto va = hn::Load(d, a);
  auto res = internal::_sqrt_fp_xN<T, N>(va);
  hn::Store(res, d, c);
}

#define define_fp_xN_unary_op(type, name, size, op)                            \
  void _##op##x##size##_##name(const type *HWY_RESTRICT a,                     \
                               type *HWY_RESTRICT c) {                         \
    _##op##xN<type, size>(a, c);                                               \
  }

#define define_fp_xN_bin_op(type, name, size, op)                              \
  void _##op##x##size##_##name(const type *HWY_RESTRICT a,                     \
                               const type *HWY_RESTRICT b,                     \
                               type *HWY_RESTRICT c) {                         \
    _##op##xN<type, size>(a, b, c);                                            \
  }

/* 32-bits */
#if HWY_MAX_BYTES >= 4
define_fp_xN_bin_op(float, f32, 1, add);
define_fp_xN_bin_op(float, f32, 1, sub);
define_fp_xN_bin_op(float, f32, 1, mul);
define_fp_xN_bin_op(float, f32, 1, div);
define_fp_xN_unary_op(float, f32, 1, sqrt);
#endif

/* 64-bits */
#if HWY_MAX_BYTES >= 8
define_fp_xN_bin_op(double, f64, 1, add);
define_fp_xN_bin_op(double, f64, 1, sub);
define_fp_xN_bin_op(double, f64, 1, mul);
define_fp_xN_bin_op(double, f64, 1, div);
define_fp_xN_unary_op(double, f64, 1, sqrt);

define_fp_xN_bin_op(float, f32, 2, add);
define_fp_xN_bin_op(float, f32, 2, sub);
define_fp_xN_bin_op(float, f32, 2, mul);
define_fp_xN_bin_op(float, f32, 2, div);
define_fp_xN_unary_op(float, f32, 2, sqrt);
#endif

/* 128-bits */
#if HWY_MAX_BYTES >= 16
define_fp_xN_bin_op(double, f64, 2, add);
define_fp_xN_bin_op(double, f64, 2, sub);
define_fp_xN_bin_op(double, f64, 2, mul);
define_fp_xN_bin_op(double, f64, 2, div);
define_fp_xN_unary_op(double, f64, 2, sqrt);

define_fp_xN_bin_op(float, f32, 4, add);
define_fp_xN_bin_op(float, f32, 4, sub);
define_fp_xN_bin_op(float, f32, 4, mul);
define_fp_xN_bin_op(float, f32, 4, div);
define_fp_xN_unary_op(float, f32, 4, sqrt);
#endif

/* 256-bits */
#if HWY_MAX_BYTES >= 32
define_fp_xN_bin_op(double, f64, 4, add);
define_fp_xN_bin_op(double, f64, 4, sub);
define_fp_xN_bin_op(double, f64, 4, mul);
define_fp_xN_bin_op(double, f64, 4, div);
define_fp_xN_unary_op(double, f64, 4, sqrt);

define_fp_xN_bin_op(float, f32, 8, add);
define_fp_xN_bin_op(float, f32, 8, sub);
define_fp_xN_bin_op(float, f32, 8, mul);
define_fp_xN_bin_op(float, f32, 8, div);
define_fp_xN_unary_op(float, f32, 8, sqrt);
#endif

/* 512-bits */
#if HWY_MAX_BYTES >= 64
define_fp_xN_bin_op(double, f64, 8, add);
define_fp_xN_bin_op(double, f64, 8, sub);
define_fp_xN_bin_op(double, f64, 8, mul);
define_fp_xN_bin_op(double, f64, 8, div);
define_fp_xN_unary_op(double, f64, 8, sqrt);

define_fp_xN_bin_op(float, f32, 16, add);
define_fp_xN_bin_op(float, f32, 16, sub);
define_fp_xN_bin_op(float, f32, 16, mul);
define_fp_xN_bin_op(float, f32, 16, div);
define_fp_xN_unary_op(float, f32, 16, sqrt);
#endif

template <typename T>
void _round(const T *HWY_RESTRICT sigma, const T *HWY_RESTRICT tau,
            T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const D d;
  const size_t N = hn::Lanes(d);
  for (size_t i = 0; i + N <= count; i += N) {
    auto sigma_vec = hn::Load(d, sigma + i);
    auto tau_vec = hn::Load(d, tau + i);
    auto res = sr_round<D>(sigma_vec, tau_vec);
    hn::Store(res, d, result + i);
  }
}

template <typename T>
HWY_NOINLINE void _add(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const D d;
  const size_t N = hn::Lanes(d);
  size_t i = 0;
  for (; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto b_vec = hn::Load(d, b + i);
    auto res = sr_add<D>(a_vec, b_vec);
    hn::Store(res, d, result + i);
  }
  using C = hn::CappedTag<T, 1>;
  const C c;
  for (; i < count; ++i) {
    auto a_vec = hn::Load(c, a + i);
    auto b_vec = hn::Load(c, b + i);
    auto res = sr_add<C>(a_vec, b_vec);
    hn::Store(res, c, result + i);
  }
}

template <typename T>
HWY_NOINLINE void _sub(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const D d;
  const size_t N = hn::Lanes(d);
  for (size_t i = 0; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto b_vec = hn::Load(d, b + i);
    auto res = sr_sub<D>(a_vec, b_vec);
    hn::Store(res, d, result + i);
  }
}

template <typename T>
HWY_NOINLINE void _mul(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);
  for (size_t i = 0; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto b_vec = hn::Load(d, b + i);
    auto res = sr_mul<D>(a_vec, b_vec);
    hn::Store(res, d, result + i);
  }
}

template <typename T>
HWY_NOINLINE void _div(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);
  for (size_t i = 0; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto b_vec = hn::Load(d, b + i);
    auto res = sr_div<D>(a_vec, b_vec);
    hn::Store(res, d, result + i);
  }
}

template <typename T>
HWY_NOINLINE void _sqrt(const T *HWY_RESTRICT a, T *HWY_RESTRICT result,
                        const size_t count) {
  using D = hn::ScalableTag<T>;
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);
  for (size_t i = 0; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto res = sr_sqrt<D>(a_vec);
    hn::Store(res, d, result + i);
  }
}

void _round_f32(const float *HWY_RESTRICT sigma, const float *HWY_RESTRICT tau,
                float *HWY_RESTRICT result, const size_t count) {
  _round<float>(sigma, tau, result, count);
}

void _add_f32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result, const size_t count) {
  _add<float>(a, b, result, count);
}
void _sub_f32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result, const size_t count) {
  _sub<float>(a, b, result, count);
}

void _mul_f32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result, const size_t count) {
  _mul<float>(a, b, result, count);
}

void _div_f32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result, const size_t count) {
  _div<float>(a, b, result, count);
}

void _sqrt_f32(const float *HWY_RESTRICT a, float *HWY_RESTRICT result,
               const size_t count) {
  _sqrt<float>(a, result, count);
}

void _round_f64(const double *HWY_RESTRICT sigma,
                const double *HWY_RESTRICT tau, double *HWY_RESTRICT result,
                const size_t count) {
  _round<double>(sigma, tau, result, count);
}

void _add_f64(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result, const size_t count) {
  _add<double>(a, b, result, count);
}

void _sub_f64(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result, const size_t count) {
  _sub<double>(a, b, result, count);
}

void _mul_f64(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result, const size_t count) {
  _mul<double>(a, b, result, count);
}

void _div_f64(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result, const size_t count) {
  _div<double>(a, b, result, count);
}

void _sqrt_f64(const double *HWY_RESTRICT a, double *HWY_RESTRICT result,
               const size_t count) {
  _sqrt<double>(a, result, count);
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
} // namespace sr
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace sr {

HWY_EXPORT(_round_f32);
HWY_EXPORT(_round_f64);
HWY_EXPORT(_add_f32);
HWY_EXPORT(_add_f64);
HWY_EXPORT(_sub_f32);
HWY_EXPORT(_sub_f64);
HWY_EXPORT(_mul_f32);
HWY_EXPORT(_mul_f64);
HWY_EXPORT(_div_f32);
HWY_EXPORT(_div_f64);
HWY_EXPORT(_sqrt_f32);
HWY_EXPORT(_sqrt_f64);

/* 32-bits */
#if HWY_MAX_BYTES >= 4
HWY_EXPORT(_addx1_f32);
HWY_EXPORT(_subx1_f32);
HWY_EXPORT(_mulx1_f32);
HWY_EXPORT(_divx1_f32);
HWY_EXPORT(_sqrtx1_f32);
#endif

/* 64-bits */
#if HWY_MAX_BYTES >= 8
HWY_EXPORT(_addx1_f64);
HWY_EXPORT(_subx1_f64);
HWY_EXPORT(_mulx1_f64);
HWY_EXPORT(_divx1_f64);
HWY_EXPORT(_sqrtx1_f64);
HWY_EXPORT(_addx2_f32);
HWY_EXPORT(_subx2_f32);
HWY_EXPORT(_mulx2_f32);
HWY_EXPORT(_divx2_f32);
HWY_EXPORT(_sqrtx2_f32);
#endif

/* 128-bits */
#if HWY_MAX_BYTES >= 16
HWY_EXPORT(_addx2_f64);
HWY_EXPORT(_subx2_f64);
HWY_EXPORT(_mulx2_f64);
HWY_EXPORT(_divx2_f64);
HWY_EXPORT(_sqrtx2_f64);
HWY_EXPORT(_addx4_f32);
HWY_EXPORT(_subx4_f32);
HWY_EXPORT(_mulx4_f32);
HWY_EXPORT(_divx4_f32);
HWY_EXPORT(_sqrtx4_f32);
#endif

/* 256-bits */
#if HWY_MAX_BYTES >= 32
HWY_EXPORT(_addx4_f64);
HWY_EXPORT(_subx4_f64);
HWY_EXPORT(_mulx4_f64);
HWY_EXPORT(_divx4_f64);
HWY_EXPORT(_sqrtx4_f64);
HWY_EXPORT(_addx8_f32);
HWY_EXPORT(_subx8_f32);
HWY_EXPORT(_mulx8_f32);
HWY_EXPORT(_divx8_f32);
HWY_EXPORT(_sqrtx8_f32);
#endif

/* 512-bits */
#if HWY_MAX_BYTES >= 64
HWY_EXPORT(_addx8_f64);
HWY_EXPORT(_subx8_f64);
HWY_EXPORT(_mulx8_f64);
HWY_EXPORT(_divx8_f64);
HWY_EXPORT(_sqrtx8_f64);
HWY_EXPORT(_addx16_f32);
HWY_EXPORT(_subx16_f32);
HWY_EXPORT(_mulx16_f32);
HWY_EXPORT(_divx16_f32);
HWY_EXPORT(_sqrtx16_f32);
#endif

template <typename T>
void round(const T *HWY_RESTRICT sigma, const T *HWY_RESTRICT tau,
           T *HWY_RESTRICT result, const size_t count) {
  if constexpr (std::is_same_v<T, float>) {
    return HWY_DYNAMIC_DISPATCH(_round_f32)(sigma, tau, result, count);
  } else if constexpr (std::is_same_v<T, double>) {
    return HWY_DYNAMIC_DISPATCH(_round_f64)(sigma, tau, result, count);
  }
}

template <typename T>
void add(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
         T *HWY_RESTRICT result, const size_t count) {
  if constexpr (std::is_same_v<T, float>) {
    return HWY_DYNAMIC_DISPATCH(_add_f32)(a, b, result, count);
  } else if constexpr (std::is_same_v<T, double>) {
    return HWY_DYNAMIC_DISPATCH(_add_f64)(a, b, result, count);
  }
}

template <typename T>
void sub(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
         T *HWY_RESTRICT result, const size_t count) {
  if constexpr (std::is_same_v<T, float>) {
    return HWY_DYNAMIC_DISPATCH(_sub_f32)(a, b, result, count);
  } else if constexpr (std::is_same_v<T, double>) {
    return HWY_DYNAMIC_DISPATCH(_sub_f64)(a, b, result, count);
  }
}

template <typename T>
HWY_DLLEXPORT void mul(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  if constexpr (std::is_same_v<T, float>) {
    return HWY_DYNAMIC_DISPATCH(_mul_f32)(a, b, result, count);
  } else if constexpr (std::is_same_v<T, double>) {
    return HWY_DYNAMIC_DISPATCH(_mul_f64)(a, b, result, count);
  }
}

template <typename T>
HWY_DLLEXPORT void div(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  if constexpr (std::is_same_v<T, float>) {
    return HWY_DYNAMIC_DISPATCH(_div_f32)(a, b, result, count);
  } else if constexpr (std::is_same_v<T, double>) {
    return HWY_DYNAMIC_DISPATCH(_div_f64)(a, b, result, count);
  }
}

template <typename T>
HWY_DLLEXPORT void sqrt(const T *HWY_RESTRICT a, T *HWY_RESTRICT result,
                        const size_t count) {
  if constexpr (std::is_same_v<T, float>) {
    return HWY_DYNAMIC_DISPATCH(_sqrt_f32)(a, result, count);
  } else if constexpr (std::is_same_v<T, double>) {
    return HWY_DYNAMIC_DISPATCH(_sqrt_f64)(a, result, count);
  }
}

/* Array functions */

void addf32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
            float *HWY_RESTRICT result, const size_t count) {
  return add(a, b, result, count);
}

void subf32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
            float *HWY_RESTRICT result, const size_t count) {
  return sub(a, b, result, count);
}

void mulf32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
            float *HWY_RESTRICT result, const size_t count) {
  return mul(a, b, result, count);
}

void divf32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
            float *HWY_RESTRICT result, const size_t count) {
  return div(a, b, result, count);
}

void sqrtf32(const float *HWY_RESTRICT a, float *HWY_RESTRICT result,
             const size_t count) {
  return sqrt(a, result, count);
}

void addf64(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
            double *HWY_RESTRICT result, const size_t count) {
  return add(a, b, result, count);
}

void subf64(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
            double *HWY_RESTRICT result, const size_t count) {
  return sub(a, b, result, count);
}

void mulf64(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
            double *HWY_RESTRICT result, const size_t count) {
  return mul(a, b, result, count);
}

void divf64(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
            double *HWY_RESTRICT result, const size_t count) {
  return div(a, b, result, count);
}

void sqrtf64(const double *HWY_RESTRICT a, double *HWY_RESTRICT result,
             const size_t count) {
  return sqrt(a, result, count);
}

/* 32-bits functions */
void addf32x1(const float a, const float b, float *result) {
  HWY_DYNAMIC_DISPATCH(_addx1_f32)
  (&a, &b, result);
}

void subf32x1(const float a, const float b, float *result) {
  HWY_DYNAMIC_DISPATCH(_subx1_f32)
  (&a, &b, result);
}

void mulf32x1(const float a, const float b, float *result) {
  HWY_DYNAMIC_DISPATCH(_mulx1_f32)
  (&a, &b, result);
}

void divf32x1(const float a, const float b, float *result) {
  HWY_DYNAMIC_DISPATCH(_divx1_f32)
  (&a, &b, result);
}

void sqrtf32x1(const float a, float *result) {
  HWY_DYNAMIC_DISPATCH(_sqrtx1_f32)(&a, result);
}

/* 64-bits functions */
void addf64x1(const double a, const double b, double *result) {
  HWY_DYNAMIC_DISPATCH(_addx1_f64)(&a, &b, result);
}

void subf64x1(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_subx1_f64)(a, b, result);
}

void mulf64x1(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_mulx1_f64)(a, b, result);
}

void divf64x1(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_divx1_f64)(a, b, result);
}

void sqrtf64x1(const double *HWY_RESTRICT a, double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_sqrtx1_f64)(a, result);
}

void addf32x2(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_addx2_f32)(a, b, result);
}

void subf32x2(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_subx2_f32)(a, b, result);
}

void mulf32x2(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_mulx2_f32)(a, b, result);
}

void divf32x2(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_divx2_f32)(a, b, result);
}

/* 128-bits functions */

void addf64x2(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_addx2_f64)(a, b, result);
}

void subf64x2(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_subx2_f64)(a, b, result);
}

void mulf64x2(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_mulx2_f64)(a, b, result);
}

void divf64x2(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_divx2_f64)(a, b, result);
}

void sqrtf64x2(const double *HWY_RESTRICT a, double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_sqrtx2_f64)(a, result);
}

#if HWY_MAX_BYTES >= 16
void addf32x4(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_addx4_f32)(a, b, result);
}

void subf32x4(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_subx4_f32)(a, b, result);
}

void mulf32x4(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_mulx4_f32)(a, b, result);
}

void divf32x4(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_divx4_f32)(a, b, result);
}
#endif

#if HWY_MAX_BYTES >= 32
void addf32x8(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_addx8_f32)(a, b, result);
}

void subf32x8(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_subx8_f32)(a, b, result);
}

void mulf32x8(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_mulx8_f32)(a, b, result);
}

void divf32x8(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_divx8_f32)(a, b, result);
}

void addf64x4(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_addx4_f64)(a, b, result);
}

void subf64x4(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_subx4_f64)(a, b, result);
}

void mulf64x4(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_mulx4_f64)(a, b, result);
}

void divf64x4(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_divx4_f64)(a, b, result);
}
#endif

} // namespace sr

#endif // HWY_ONCE
