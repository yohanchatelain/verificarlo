
#include "src/debug.hpp"
#include "src/ud.h"

// Generates code for every target that this compiler can support.
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "src/ud_hw.cpp" // this file
#include "hwy/foreach_target.h"            // must come before highway.h

// clang-format off
#include "hwy/highway.h"
#include "src/ud_hw-inl.h"
#include "src/ud_hw.h"
#include "src/xoroshiro256+_hw-inl.hpp"
#include "src/debug_hwy-inl.h"
// clang-format on

// Optional, can instead add HWY_ATTR to all functions.
HWY_BEFORE_NAMESPACE();
namespace prism::ud::vector {

namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;
namespace dbg = prism::vector::HWY_NAMESPACE;

namespace internal {

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
V _add_fp_xN(V a, V b) {
  auto res = add<D>(a, b);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
V _sub_fp_xN(V a, V b) {
  auto res = sub<D>(a, b);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
V _mul_fp_xN(V a, V b) {
  auto res = mul<D>(a, b);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
V _div_fp_xN(V a, V b) {
  auto res = div<D>(a, b);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
V _sqrt_fp_xN(V a) {
  auto res = sqrt<D>(a);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
V _fma_fp_xN(V a, V b, V c) {
  auto res = fma<D>(a, b, c);
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
f32x1 _fma_f32_x1(f32x1 a, f32x1 b, f32x1 c) {
  return _fma_fp_xN<float, 1>(a, b, c);
}
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
f64x1 _fma_f64_x1(f64x1 a, f64x1 b, f64x1 c) {
  return _fma_fp_xN<double, 1>(a, b, c);
}

f32x2 _add_f32_x2(f32x2 a, f32x2 b) { return _add_fp_xN<float, 2>(a, b); }
f32x2 _sub_f32_x2(f32x2 a, f32x2 b) { return _sub_fp_xN<float, 2>(a, b); }
f32x2 _mul_f32_x2(f32x2 a, f32x2 b) { return _mul_fp_xN<float, 2>(a, b); }
f32x2 _div_f32_x2(f32x2 a, f32x2 b) { return _div_fp_xN<float, 2>(a, b); }
f32x2 _sqrt_f32_x2(f32x2 a) { return _sqrt_fp_xN<float, 2>(a); }
f32x2 _fma_f32_x2(f32x2 a, f32x2 b, f32x2 c) {
  return _fma_fp_xN<float, 2>(a, b, c);
}
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
f64x2 _fma_f64_x2(f64x2 a, f64x2 b, f64x2 c) {
  return _fma_fp_xN<double, 2>(a, b, c);
}

f32x4 _add_f32_x4(f32x4 a, f32x4 b) { return _add_fp_xN<float, 4>(a, b); }
f32x4 _sub_f32_x4(f32x4 a, f32x4 b) { return _sub_fp_xN<float, 4>(a, b); }
f32x4 _mul_f32_x4(f32x4 a, f32x4 b) { return _mul_fp_xN<float, 4>(a, b); }
f32x4 _div_f32_x4(f32x4 a, f32x4 b) { return _div_fp_xN<float, 4>(a, b); }
f32x4 _sqrt_f32_x4(f32x4 a) { return _sqrt_fp_xN<float, 4>(a); }
f32x4 _fma_f32_x4(f32x4 a, f32x4 b, f32x4 c) {
  return _fma_fp_xN<float, 4>(a, b, c);
}
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
f64x4 _fma_f64_x4(f64x4 a, f64x4 b, f64x4 c) {
  return _fma_fp_xN<double, 4>(a, b, c);
}

f32x8 _add_f32_x8(f32x8 a, f32x8 b) { return _add_fp_xN<float, 8>(a, b); }
f32x8 _sub_f32_x8(f32x8 a, f32x8 b) { return _sub_fp_xN<float, 8>(a, b); }
f32x8 _mul_f32_x8(f32x8 a, f32x8 b) { return _mul_fp_xN<float, 8>(a, b); }
f32x8 _div_f32_x8(f32x8 a, f32x8 b) { return _div_fp_xN<float, 8>(a, b); }
f32x8 _sqrt_f32_x8(f32x8 a) { return _sqrt_fp_xN<float, 8>(a); }
f32x8 _fma_f32_x8(f32x8 a, f32x8 b, f32x8 c) {
  return _fma_fp_xN<float, 8>(a, b, c);
}
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
f64x8 _fma_f64_x8(f64x8 a, f64x8 b, f64x8 c) {
  return _fma_fp_xN<double, 8>(a, b, c);
}

f32x16 _add_f32_x16(f32x16 a, f32x16 b) { return _add_fp_xN<float, 16>(a, b); }
f32x16 _sub_f32_x16(f32x16 a, f32x16 b) { return _sub_fp_xN<float, 16>(a, b); }
f32x16 _mul_f32_x16(f32x16 a, f32x16 b) { return _mul_fp_xN<float, 16>(a, b); }
f32x16 _div_f32_x16(f32x16 a, f32x16 b) { return _div_fp_xN<float, 16>(a, b); }
f32x16 _sqrt_f32_x16(f32x16 a) { return _sqrt_fp_xN<float, 16>(a); }
f32x16 _fma_f32_x16(f32x16 a, f32x16 b, f32x16 c) {
  return _fma_fp_xN<float, 16>(a, b, c);
}
#endif
} // namespace internal

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _addxN(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
            T *HWY_RESTRICT result) {
  const D d;
  const auto va = hn::Load(d, a);
  const auto vb = hn::Load(d, b);
  auto res = internal::_add_fp_xN<T, N>(va, vb);
  hn::Store(res, d, result);
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _subxN(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
            T *HWY_RESTRICT result) {
  const D d;
  const auto va = hn::Load(d, a);
  const auto vb = hn::Load(d, b);
  auto res = internal::_sub_fp_xN<T, N>(va, vb);
  hn::Store(res, d, result);
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _mulxN(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
            T *HWY_RESTRICT result) {
  const D d;
  const auto va = hn::Load(d, a);
  const auto vb = hn::Load(d, b);
  auto res = internal::_mul_fp_xN<T, N>(va, vb);
  hn::Store(res, d, result);
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _divxN(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
            T *HWY_RESTRICT result) {
  const D d;
  const auto va = hn::Load(d, a);
  const auto vb = hn::Load(d, b);
  auto res = internal::_div_fp_xN<T, N>(va, vb);
  hn::Store(res, d, result);
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _sqrtxN(const T *HWY_RESTRICT a, T *HWY_RESTRICT result) {
  const D d;
  const auto va = hn::Load(d, a);
  auto res = internal::_sqrt_fp_xN<T, N>(va);
  hn::Store(res, d, result);
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _fmaxN(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
            const T *HWY_RESTRICT c, T *HWY_RESTRICT result) {
  const D d;
  const auto va = hn::Load(d, a);
  const auto vb = hn::Load(d, b);
  const auto vc = hn::Load(d, c);
  auto res = internal::_fma_fp_xN<T, N>(va, vb, vc);
  hn::Store(res, d, result);
}

/* _<op>x<size>_<type> */

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

#define define_fp_xN_ter_op(type, name, size, op)                              \
  void _##op##x##size##_##name(                                                \
      const type *HWY_RESTRICT a, const type *HWY_RESTRICT b,                  \
      const type *HWY_RESTRICT c, type *HWY_RESTRICT r) {                      \
    _##op##xN<type, size>(a, b, c, r);                                         \
  }

/* 32-bits */
#if HWY_MAX_BYTES >= 4
define_fp_xN_bin_op(float, f32, 1, add);
define_fp_xN_bin_op(float, f32, 1, sub);
define_fp_xN_bin_op(float, f32, 1, mul);
define_fp_xN_bin_op(float, f32, 1, div);
define_fp_xN_unary_op(float, f32, 1, sqrt);
define_fp_xN_ter_op(float, f32, 1, fma);
#endif

/* 64-bits */
#if HWY_MAX_BYTES >= 8
define_fp_xN_bin_op(double, f64, 1, add);
define_fp_xN_bin_op(double, f64, 1, sub);
define_fp_xN_bin_op(double, f64, 1, mul);
define_fp_xN_bin_op(double, f64, 1, div);
define_fp_xN_unary_op(double, f64, 1, sqrt);
define_fp_xN_ter_op(double, f64, 1, fma);

define_fp_xN_bin_op(float, f32, 2, add);
define_fp_xN_bin_op(float, f32, 2, sub);
define_fp_xN_bin_op(float, f32, 2, mul);
define_fp_xN_bin_op(float, f32, 2, div);
define_fp_xN_unary_op(float, f32, 2, sqrt);
define_fp_xN_ter_op(float, f32, 2, fma);
#endif

/* 128-bits */
#if HWY_MAX_BYTES >= 16
define_fp_xN_bin_op(double, f64, 2, add);
define_fp_xN_bin_op(double, f64, 2, sub);
define_fp_xN_bin_op(double, f64, 2, mul);
define_fp_xN_bin_op(double, f64, 2, div);
define_fp_xN_unary_op(double, f64, 2, sqrt);
define_fp_xN_ter_op(double, f64, 2, fma);

define_fp_xN_bin_op(float, f32, 4, add);
define_fp_xN_bin_op(float, f32, 4, sub);
define_fp_xN_bin_op(float, f32, 4, mul);
define_fp_xN_bin_op(float, f32, 4, div);
define_fp_xN_unary_op(float, f32, 4, sqrt);
define_fp_xN_ter_op(float, f32, 4, fma);
#endif

/* 256-bits */
#if HWY_MAX_BYTES >= 32
define_fp_xN_bin_op(double, f64, 4, add);
define_fp_xN_bin_op(double, f64, 4, sub);
define_fp_xN_bin_op(double, f64, 4, mul);
define_fp_xN_bin_op(double, f64, 4, div);
define_fp_xN_unary_op(double, f64, 4, sqrt);
define_fp_xN_ter_op(double, f64, 4, fma);

define_fp_xN_bin_op(float, f32, 8, add);
define_fp_xN_bin_op(float, f32, 8, sub);
define_fp_xN_bin_op(float, f32, 8, mul);
define_fp_xN_bin_op(float, f32, 8, div);
define_fp_xN_unary_op(float, f32, 8, sqrt);
define_fp_xN_ter_op(float, f32, 8, fma);
#endif

/* 512-bits */
#if HWY_MAX_BYTES >= 64
define_fp_xN_bin_op(double, f64, 8, add);
define_fp_xN_bin_op(double, f64, 8, sub);
define_fp_xN_bin_op(double, f64, 8, mul);
define_fp_xN_bin_op(double, f64, 8, div);
define_fp_xN_unary_op(double, f64, 8, sqrt);
define_fp_xN_ter_op(double, f64, 8, fma);

define_fp_xN_bin_op(float, f32, 16, add);
define_fp_xN_bin_op(float, f32, 16, sub);
define_fp_xN_bin_op(float, f32, 16, mul);
define_fp_xN_bin_op(float, f32, 16, div);
define_fp_xN_unary_op(float, f32, 16, sqrt);
define_fp_xN_ter_op(float, f32, 16, fma);
#endif

template <typename T>
void _round(const T *HWY_RESTRICT a, T *HWY_RESTRICT result,
            const size_t count) {
  using D = hn::ScalableTag<T>;
  const D d;
  const size_t N = hn::Lanes(d);
  size_t i = 0;
  for (; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto res = round<D>(a_vec);
    hn::Store(res, d, result + i);
  }
  //   using C = hn::CappedTag<T, 1>;
  //   const C c;
  //   for (; i < count; ++i) {
  //     auto sigma_vec = hn::Load(d, sigma + i);
  //     auto tau_vec = hn::Load(d, tau + i);
  //     auto res = ud_round<C>(sigma_vec, tau_vec);
  //     hn::Store(res, c, result + i);
  //   }
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
    auto res = add<D>(a_vec, b_vec);
    hn::Store(res, d, result + i);
  }
  //   using C = hn::CappedTag<T, 1>;
  //   const C c;
  //   for (; i < count; ++i) {
  //     auto a_vec = hn::Load(c, a + i);
  //     auto b_vec = hn::Load(c, b + i);
  //     auto res = ud_add<C>(a_vec, b_vec);
  //     hn::Store(res, c, result + i);
  //   }
}

template <typename T>
HWY_NOINLINE void _sub(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const D d;
  const size_t N = hn::Lanes(d);
  size_t i = 0;
  for (; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto b_vec = hn::Load(d, b + i);
    auto res = sub<D>(a_vec, b_vec);
    hn::Store(res, d, result + i);
  }
  //   using C = hn::CappedTag<T, 1>;
  //   const C c;
  //   for (; i < count; ++i) {
  //     auto a_vec = hn::Load(c, a + i);
  //     auto b_vec = hn::Load(c, b + i);
  //     auto res = ud_sub<C>(a_vec, b_vec);
  //     hn::Store(res, c, result + i);
  //   }
}

template <typename T>
HWY_NOINLINE void _mul(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);
  size_t i = 0;
  for (; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto b_vec = hn::Load(d, b + i);
    auto res = mul<D>(a_vec, b_vec);
    hn::Store(res, d, result + i);
  }
  //   using C = hn::CappedTag<T, 1>;
  //   const C c;
  //   for (; i < count; ++i) {
  //     auto a_vec = hn::Load(c, a + i);
  //     auto b_vec = hn::Load(c, b + i);
  //     auto res = ud_mul<C>(a_vec, b_vec);
  //     hn::Store(res, c, result + i);
  //   }
}

template <typename T>
HWY_NOINLINE void _div(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);
  size_t i = 0;
  for (; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto b_vec = hn::Load(d, b + i);
    auto res = div<D>(a_vec, b_vec);
    hn::Store(res, d, result + i);
  }
  //   using C = hn::CappedTag<T, 1>;
  //   const C c;
  //   for (; i < count; ++i) {
  //     auto a_vec = hn::Load(c, a + i);
  //     auto b_vec = hn::Load(c, b + i);
  //     auto res = ud_div<C>(a_vec, b_vec);
  //     hn::Store(res, c, result + i);
  //   }
}

template <typename T>
HWY_NOINLINE void _sqrt(const T *HWY_RESTRICT a, T *HWY_RESTRICT result,
                        const size_t count) {
  using D = hn::ScalableTag<T>;
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);
  size_t i = 0;
  for (; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto res = sqrt<D>(a_vec);
    hn::Store(res, d, result + i);
  }
  //   using C = hn::CappedTag<T, 1>;
  //   const C c;
  //   for (; i < count; ++i) {
  //     auto a_vec = hn::Load(d, a + i);
  //     auto res = ud_sqrt<C>(a_vec);
  //     hn::Store(res, c, result + i);
  //   }
}

template <typename T>
HWY_NOINLINE void _fma(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
                       const T *HWY_RESTRICT c, T *HWY_RESTRICT result,
                       const size_t count) {
  using D = hn::ScalableTag<T>;
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);
  size_t i = 0;
  for (; i + N <= count; i += N) {
    auto a_vec = hn::Load(d, a + i);
    auto b_vec = hn::Load(d, b + i);
    auto c_vec = hn::Load(d, c + i);
    auto res = fma<D>(a_vec, b_vec, c_vec);
    hn::Store(res, d, result + i);
  }
  //   using C = hn::CappedTag<T, 1>;
  //   const C c;
  //   for (; i < count; ++i) {
  //     auto a_vec = hn::Load(d, a + i);
  //     auto b_vec = hn::Load(d, b + i);
  //     auto c_vec = hn::Load(d, c + i);
  //     auto res = ud_fma<C>(a_vec, b_vec, c_vec);
  //     hn::Store(res, c, d + i);
  //   }
}

void _round_f32(const float *HWY_RESTRICT a, float *HWY_RESTRICT result,
                const size_t count) {
  _round<float>(a, result, count);
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

void _fma_f32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              const float *HWY_RESTRICT c, float *HWY_RESTRICT result,
              const size_t count) {
  _fma<float>(a, b, c, result, count);
}

void _round_f64(const double *HWY_RESTRICT a, double *HWY_RESTRICT result,
                const size_t count) {
  _round<double>(a, result, count);
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

void _fma_f64(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              const double *HWY_RESTRICT c, double *HWY_RESTRICT result,
              const size_t count) {
  _fma<double>(a, b, c, result, count);
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
} // namespace prism::ud::vector
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace prism::ud::vector {

HWY_EXPORT(_round_f32);
HWY_EXPORT(_add_f32);
HWY_EXPORT(_sub_f32);
HWY_EXPORT(_mul_f32);
HWY_EXPORT(_div_f32);
HWY_EXPORT(_sqrt_f32);
HWY_EXPORT(_fma_f32);

HWY_EXPORT(_round_f64);
HWY_EXPORT(_add_f64);
HWY_EXPORT(_sub_f64);
HWY_EXPORT(_mul_f64);
HWY_EXPORT(_div_f64);
HWY_EXPORT(_sqrt_f64);
HWY_EXPORT(_fma_f64);

/* 32-bits */
#if HWY_MAX_BYTES >= 4
HWY_EXPORT(_addx1_f32);
HWY_EXPORT(_subx1_f32);
HWY_EXPORT(_mulx1_f32);
HWY_EXPORT(_divx1_f32);
HWY_EXPORT(_sqrtx1_f32);
HWY_EXPORT(_fmax1_f32);
#endif

/* 64-bits */
#if HWY_MAX_BYTES >= 8
HWY_EXPORT(_addx1_f64);
HWY_EXPORT(_subx1_f64);
HWY_EXPORT(_mulx1_f64);
HWY_EXPORT(_divx1_f64);
HWY_EXPORT(_sqrtx1_f64);
HWY_EXPORT(_fmax1_f64);

HWY_EXPORT(_addx2_f32);
HWY_EXPORT(_subx2_f32);
HWY_EXPORT(_mulx2_f32);
HWY_EXPORT(_divx2_f32);
HWY_EXPORT(_sqrtx2_f32);
HWY_EXPORT(_fmax2_f32);
#endif

/* 128-bits */
#if HWY_MAX_BYTES >= 16
HWY_EXPORT(_addx2_f64);
HWY_EXPORT(_subx2_f64);
HWY_EXPORT(_mulx2_f64);
HWY_EXPORT(_divx2_f64);
HWY_EXPORT(_sqrtx2_f64);
HWY_EXPORT(_fmax2_f64);

HWY_EXPORT(_addx4_f32);
HWY_EXPORT(_subx4_f32);
HWY_EXPORT(_mulx4_f32);
HWY_EXPORT(_divx4_f32);
HWY_EXPORT(_sqrtx4_f32);
HWY_EXPORT(_fmax4_f32);
#endif

/* 256-bits */
#if (HWY_MAX_BYTES >= 32) && (HWY_TARGET != HWY_SSE4)
HWY_EXPORT(_addx4_f64);
HWY_EXPORT(_subx4_f64);
HWY_EXPORT(_mulx4_f64);
HWY_EXPORT(_divx4_f64);
HWY_EXPORT(_sqrtx4_f64);
HWY_EXPORT(_fmax4_f64);

HWY_EXPORT(_addx8_f32);
HWY_EXPORT(_subx8_f32);
HWY_EXPORT(_mulx8_f32);
HWY_EXPORT(_divx8_f32);
HWY_EXPORT(_sqrtx8_f32);
HWY_EXPORT(_fmax8_f32);
#endif

/* 512-bits */
#if HWY_MAX_BYTES >= 64
HWY_EXPORT(_addx8_f64);
HWY_EXPORT(_subx8_f64);
HWY_EXPORT(_mulx8_f64);
HWY_EXPORT(_divx8_f64);
HWY_EXPORT(_sqrtx8_f64);
HWY_EXPORT(_fmax8_f64);

HWY_EXPORT(_addx16_f32);
HWY_EXPORT(_subx16_f32);
HWY_EXPORT(_mulx16_f32);
HWY_EXPORT(_divx16_f32);
HWY_EXPORT(_sqrtx16_f32);
HWY_EXPORT(_fmax16_f32);
#endif

/* 1024-bits */
#if HWY_MAX_BYTES >= 128
HWY_EXPORT(_addx16_f64);
HWY_EXPORT(_subx16_f64);
HWY_EXPORT(_mulx16_f64);
HWY_EXPORT(_divx16_f64);
HWY_EXPORT(_sqrtx16_f64);
HWY_EXPORT(_fmax16_f64);
#endif

template <typename T>
void round(const T *HWY_RESTRICT a, T *HWY_RESTRICT result,
           const size_t count) {
  if constexpr (std::is_same_v<T, float>) {
    return HWY_DYNAMIC_DISPATCH(_round_f32)(a, result, count);
  } else if constexpr (std::is_same_v<T, double>) {
    return HWY_DYNAMIC_DISPATCH(_round_f64)(a, result, count);
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
void mul(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
         T *HWY_RESTRICT result, const size_t count) {
  if constexpr (std::is_same_v<T, float>) {
    return HWY_DYNAMIC_DISPATCH(_mul_f32)(a, b, result, count);
  } else if constexpr (std::is_same_v<T, double>) {
    return HWY_DYNAMIC_DISPATCH(_mul_f64)(a, b, result, count);
  }
}

template <typename T>
void div(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
         T *HWY_RESTRICT result, const size_t count) {
  if constexpr (std::is_same_v<T, float>) {
    return HWY_DYNAMIC_DISPATCH(_div_f32)(a, b, result, count);
  } else if constexpr (std::is_same_v<T, double>) {
    return HWY_DYNAMIC_DISPATCH(_div_f64)(a, b, result, count);
  }
}

template <typename T>
void sqrt(const T *HWY_RESTRICT a, T *HWY_RESTRICT result, const size_t count) {
  if constexpr (std::is_same_v<T, float>) {
    return HWY_DYNAMIC_DISPATCH(_sqrt_f32)(a, result, count);
  } else if constexpr (std::is_same_v<T, double>) {
    return HWY_DYNAMIC_DISPATCH(_sqrt_f64)(a, result, count);
  }
}

template <typename T>
void fma(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
         const T *HWY_RESTRICT c, T *HWY_RESTRICT result, const size_t count) {
  if constexpr (std::is_same_v<T, float>) {
    return HWY_DYNAMIC_DISPATCH(_fma_f32)(a, b, c, result, count);
  } else if constexpr (std::is_same_v<T, double>) {
    return HWY_DYNAMIC_DISPATCH(_fma_f64)(a, b, c, result, count);
  }
}

/* Array functions */

/* binary32 */

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

void fmaf32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
            const float *HWY_RESTRICT c, float *HWY_RESTRICT result,
            const size_t count) {
  return fma(a, b, c, result, count);
}

/* binary64 */

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

void fmaf64(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
            const double *HWY_RESTRICT c, double *HWY_RESTRICT result,
            const size_t count) {
  return fma(a, b, c, result, count);
}

/* Single vector instructions with dynamic dispatch */

/* 64-bits functions */
#if HWY_MAX_BYTES >= 8

/* binary32 */

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

void sqrtf32x2(const float *HWY_RESTRICT a, float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_sqrtx2_f32)(a, result);
}

void fmaf32x2(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              const float *HWY_RESTRICT c, float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_fmax2_f32)(a, b, c, result);
}
#endif

/* 128-bits functions */
#if HWY_MAX_BYTES >= 16
/* binary64 */
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

void fmaf64x2(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              const double *HWY_RESTRICT c, double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_fmax2_f64)(a, b, c, result);
}

/* binary32 */
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

void sqrtf32x4(const float *HWY_RESTRICT a, float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_sqrtx4_f32)(a, result);
}

void fmaf32x4(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              const float *HWY_RESTRICT c, float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_fmax4_f32)(a, b, c, result);
}

#endif

/* 256-bits functions*/

#if HWY_MAX_BYTES >= 32

/* binary32 */

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

void sqrtf32x8(const float *HWY_RESTRICT a, float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_sqrtx8_f32)(a, result);
}

void fmaf32x8(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              const float *HWY_RESTRICT c, float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_fmax8_f32)(a, b, c, result);
}

/* binary64 */

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

void sqrtf64x4(const double *HWY_RESTRICT a, double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_sqrtx4_f64)(a, result);
}

void fmaf64x4(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              const double *HWY_RESTRICT c, double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_fmax4_f64)(a, b, c, result);
}

#endif

/* 512-bits functions */

#if HWY_MAX_BYTES >= 64

/* binary64 */

void addf64x8(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_addx8_f64)(a, b, result);
}

void subf64x8(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_subx8_f64)(a, b, result);
}

void mulf64x8(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_mulx8_f64)(a, b, result);
}

void divf64x8(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_divx8_f64)(a, b, result);
}

void sqrtf64x8(const double *HWY_RESTRICT a, double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_sqrtx8_f64)(a, result);
}

void fmaf64x8(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              const double *HWY_RESTRICT c, double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_fmax8_f64)(a, b, c, result);
}

/* binary32 */

void addf32x16(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
               float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_addx16_f32)(a, b, result);
}

void subf32x16(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
               float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_subx16_f32)(a, b, result);
}

void mulf32x16(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
               float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_mulx16_f32)(a, b, result);
}

void divf32x16(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
               float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_divx16_f32)(a, b, result);
}

void sqrtf32x16(const float *HWY_RESTRICT a, float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_sqrtx16_f32)(a, result);
}

void fmaf32x16(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
               const float *HWY_RESTRICT c, float *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_fmax16_f32)(a, b, c, result);
}

#endif

/* 1024-bits functions */
#if HWY_MAX_BYTES >= 128

/* binary64 */
void addf64x16(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
               double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_addx16_f64)(a, b, result);
}

void subf64x16(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
               double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_subx16_f64)(a, b, result);
}

void mulf64x16(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
               double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_mulx16_f64)(a, b, result);
}

void divf64x16(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
               double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_divx16_f64)(a, b, result);
}

void sqrtf64x16(const double *HWY_RESTRICT a, double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_sqrtx16_f64)(a, result);
}

void fmaf64x16(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
               const double *HWY_RESTRICT c, double *HWY_RESTRICT result) {
  HWY_DYNAMIC_DISPATCH(_fmax16_f64)(a, b, c, result);
}

#endif

/* Single vector functions with static dispatch */

/* 64-bits */

#if HWY_MAX_BYTES >= 8

/* binary32 */

f32x2_v addf32x2_static(const f32x2_v a, const f32x2_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(8) float result_ptr[2];
  HWY_STATIC_DISPATCH(_addx2_f32)(a_ptr, b_ptr, result_ptr);
  f32x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x2_v));
  return result;
}

f32x2_v subf32x2_static(const f32x2_v a, const f32x2_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(8) float result_ptr[2];
  HWY_STATIC_DISPATCH(_subx2_f32)(a_ptr, b_ptr, result_ptr);
  f32x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x2_v));
  return result;
}

f32x2_v mulf32x2_static(const f32x2_v a, const f32x2_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(8) float result_ptr[2];
  HWY_STATIC_DISPATCH(_mulx2_f32)(a_ptr, b_ptr, result_ptr);
  f32x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x2_v));
  return result;
}

f32x2_v divf32x2_static(const f32x2_v a, const f32x2_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(8) float result_ptr[2];
  HWY_STATIC_DISPATCH(_divx2_f32)(a_ptr, b_ptr, result_ptr);
  f32x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x2_v));
  return result;
}

f32x2_v sqrtf32x2_static(const f32x2_v a) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  alignas(8) float result_ptr[2];
  HWY_STATIC_DISPATCH(_sqrtx2_f32)(a_ptr, result_ptr);
  f32x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x2_v));
  return result;
}

f32x2_v fmaf32x2_static(const f32x2_v a, const f32x2_v b, const f32x2_v c) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  const float *c_ptr = reinterpret_cast<const float *>(&c);
  alignas(8) float result_ptr[2];
  HWY_STATIC_DISPATCH(_fmax2_f32)(a_ptr, b_ptr, c_ptr, result_ptr);
  f32x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x2_v));
  return result;
}

#endif

/* 128-bits */

#if HWY_MAX_BYTES >= 16

/* binary64 */

f64x2_v addf64x2_static(const f64x2_v a, const f64x2_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(16) double result_ptr[2];
  HWY_STATIC_DISPATCH(_addx2_f64)(a_ptr, b_ptr, result_ptr);
  f64x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x2_v));
  return result;
}

f64x2_v subf64x2_static(const f64x2_v a, const f64x2_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(16) double result_ptr[2];
  HWY_STATIC_DISPATCH(_subx2_f64)(a_ptr, b_ptr, result_ptr);
  f64x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x2_v));
  return result;
}

f64x2_v mulf64x2_static(const f64x2_v a, const f64x2_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(16) double result_ptr[2];
  HWY_STATIC_DISPATCH(_mulx2_f64)(a_ptr, b_ptr, result_ptr);
  f64x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x2_v));
  return result;
}

f64x2_v divf64x2_static(const f64x2_v a, const f64x2_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(16) double result_ptr[2];
  HWY_STATIC_DISPATCH(_divx2_f64)(a_ptr, b_ptr, result_ptr);
  f64x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x2_v));
  return result;
}

f64x2_v sqrtf64x2_static(const f64x2_v a) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  alignas(16) double result_ptr[2];
  HWY_STATIC_DISPATCH(_sqrtx2_f64)(a_ptr, result_ptr);
  f64x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x2_v));
  return result;
}

f64x2_v fmaf64x2_static(const f64x2_v a, const f64x2_v b, const f64x2_v c) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  const double *c_ptr = reinterpret_cast<const double *>(&c);
  alignas(16) double result_ptr[2];
  HWY_STATIC_DISPATCH(_fmax2_f64)(a_ptr, b_ptr, c_ptr, result_ptr);
  f64x2_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x2_v));
  return result;
}

/* binary32 */

f32x4_v addf32x4_static(const f32x4_v a, const f32x4_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(16) float result_ptr[4];
  HWY_STATIC_DISPATCH(_addx4_f32)(a_ptr, b_ptr, result_ptr);
  f32x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x4_v));
  return result;
}

f32x4_v subf32x4_static(const f32x4_v a, const f32x4_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(16) float result_ptr[4];
  HWY_STATIC_DISPATCH(_subx4_f32)(a_ptr, b_ptr, result_ptr);
  f32x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x4_v));
  return result;
}

f32x4_v mulf32x4_static(const f32x4_v a, const f32x4_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(16) float result_ptr[4];
  HWY_STATIC_DISPATCH(_mulx4_f32)(a_ptr, b_ptr, result_ptr);
  f32x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x4_v));
  return result;
}

f32x4_v divf32x4_static(const f32x4_v a, const f32x4_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(16) float result_ptr[4];
  HWY_STATIC_DISPATCH(_divx4_f32)(a_ptr, b_ptr, result_ptr);
  f32x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x4_v));
  return result;
}

f32x4_v sqrtf32x4_static(const f32x4_v a) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  alignas(16) float result_ptr[4];
  HWY_STATIC_DISPATCH(_sqrtx4_f32)(a_ptr, result_ptr);
  f32x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x4_v));
  return result;
}

f32x4_v fmaf32x4_static(const f32x4_v a, const f32x4_v b, const f32x4_v c) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  const float *c_ptr = reinterpret_cast<const float *>(&c);
  alignas(16) float result_ptr[4];
  HWY_STATIC_DISPATCH(_fmax4_f32)(a_ptr, b_ptr, c_ptr, result_ptr);
  f32x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x4_v));
  return result;
}

#endif

/* 256-bits */

#if HWY_MAX_BYTES >= 32

/* binary64 */

f64x4_v addf64x4_static(const f64x4_v a, const f64x4_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(32) double result_ptr[4];
  HWY_STATIC_DISPATCH(_addx4_f64)(a_ptr, b_ptr, result_ptr);
  f64x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x4_v));
  return result;
}

f64x4_v subf64x4_static(const f64x4_v a, const f64x4_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(32) double result_ptr[4];
  HWY_STATIC_DISPATCH(_subx4_f64)(a_ptr, b_ptr, result_ptr);
  f64x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x4_v));
  return result;
}

f64x4_v mulf64x4_static(const f64x4_v a, const f64x4_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(32) double result_ptr[4];
  HWY_STATIC_DISPATCH(_mulx4_f64)(a_ptr, b_ptr, result_ptr);
  f64x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x4_v));
  return result;
}

f64x4_v divf64x4_static(const f64x4_v a, const f64x4_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(32) double result_ptr[4];
  HWY_STATIC_DISPATCH(_divx4_f64)(a_ptr, b_ptr, result_ptr);
  f64x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x4_v));
  return result;
}

f64x4_v sqrtf64x4_static(const f64x4_v a) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  alignas(32) double result_ptr[4];
  HWY_STATIC_DISPATCH(_sqrtx4_f64)(a_ptr, result_ptr);
  f64x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x4_v));
  return result;
}

f64x4_v fmaf64x4_static(const f64x4_v a, const f64x4_v b, const f64x4_v c) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  const double *c_ptr = reinterpret_cast<const double *>(&c);
  alignas(32) double result_ptr[4];
  HWY_STATIC_DISPATCH(_fmax4_f64)(a_ptr, b_ptr, c_ptr, result_ptr);
  f64x4_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x4_v));
  return result;
}

/* binary32 */

f32x8_v addf32x8_static(const f32x8_v a, const f32x8_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(32) float result_ptr[8];
  HWY_STATIC_DISPATCH(_addx8_f32)(a_ptr, b_ptr, result_ptr);
  f32x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x8_v));
  return result;
}

f32x8_v subf32x8_static(const f32x8_v a, const f32x8_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(32) float result_ptr[8];
  HWY_STATIC_DISPATCH(_subx8_f32)(a_ptr, b_ptr, result_ptr);
  f32x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x8_v));
  return result;
}

f32x8_v mulf32x8_static(const f32x8_v a, const f32x8_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(32) float result_ptr[8];
  HWY_STATIC_DISPATCH(_mulx8_f32)(a_ptr, b_ptr, result_ptr);
  f32x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x8_v));
  return result;
}

f32x8_v divf32x8_static(const f32x8_v a, const f32x8_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(32) float result_ptr[8];
  HWY_STATIC_DISPATCH(_divx8_f32)(a_ptr, b_ptr, result_ptr);
  f32x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x8_v));
  return result;
}

f32x8_v sqrtf32x8_static(const f32x8_v a) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  alignas(32) float result_ptr[8];
  HWY_STATIC_DISPATCH(_sqrtx8_f32)(a_ptr, result_ptr);
  f32x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x8_v));
  return result;
}

f32x8_v fmaf32x8_static(const f32x8_v a, const f32x8_v b, const f32x8_v c) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  const float *c_ptr = reinterpret_cast<const float *>(&c);
  alignas(32) float result_ptr[8];
  HWY_STATIC_DISPATCH(_fmax8_f32)(a_ptr, b_ptr, c_ptr, result_ptr);
  f32x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x8_v));
  return result;
}

#endif

/* 512-bits */

#if HWY_MAX_BYTES >= 64

/* binary64 */

f64x8_v addf64x8_static(const f64x8_v a, const f64x8_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(64) double result_ptr[8];
  HWY_STATIC_DISPATCH(_addx8_f64)(a_ptr, b_ptr, result_ptr);
  f64x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x8_v));
  return result;
}

f64x8_v subf64x8_static(const f64x8_v a, const f64x8_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(64) double result_ptr[8];
  HWY_STATIC_DISPATCH(_subx8_f64)(a_ptr, b_ptr, result_ptr);
  f64x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x8_v));
  return result;
}

f64x8_v mulf64x8_static(const f64x8_v a, const f64x8_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(64) double result_ptr[8];
  HWY_STATIC_DISPATCH(_mulx8_f64)(a_ptr, b_ptr, result_ptr);
  f64x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x8_v));
  return result;
}

f64x8_v divf64x8_static(const f64x8_v a, const f64x8_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(64) double result_ptr[8];
  HWY_STATIC_DISPATCH(_divx8_f64)(a_ptr, b_ptr, result_ptr);
  f64x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x8_v));
  return result;
}

f64x8_v sqrtf64x8_static(const f64x8_v a) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  alignas(64) double result_ptr[8];
  HWY_STATIC_DISPATCH(_sqrtx8_f64)(a_ptr, result_ptr);
  f64x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x8_v));
  return result;
}

f64x8_v fmaf64x8_static(const f64x8_v a, const f64x8_v b, const f64x8_v c) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  const double *c_ptr = reinterpret_cast<const double *>(&c);
  alignas(64) double result_ptr[8];
  HWY_STATIC_DISPATCH(_fmax8_f64)(a_ptr, b_ptr, c_ptr, result_ptr);
  f64x8_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x8_v));
  return result;
}

/* binary32 */

f32x16_v addf32x16_static(const f32x16_v a, const f32x16_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(64) float result_ptr[16];
  HWY_STATIC_DISPATCH(_addx16_f32)(a_ptr, b_ptr, result_ptr);
  f32x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x16_v));
  return result;
}

f32x16_v subf32x16_static(const f32x16_v a, const f32x16_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(64) float result_ptr[16];
  HWY_STATIC_DISPATCH(_subx16_f32)(a_ptr, b_ptr, result_ptr);
  f32x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x16_v));
  return result;
}

f32x16_v mulf32x16_static(const f32x16_v a, const f32x16_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(64) float result_ptr[16];
  HWY_STATIC_DISPATCH(_mulx16_f32)(a_ptr, b_ptr, result_ptr);
  f32x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x16_v));
  return result;
}

f32x16_v divf32x16_static(const f32x16_v a, const f32x16_v b) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  alignas(64) float result_ptr[16];
  HWY_STATIC_DISPATCH(_divx16_f32)(a_ptr, b_ptr, result_ptr);
  f32x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x16_v));
  return result;
}

f32x16_v sqrtf32x16_static(const f32x16_v a) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  alignas(64) float result_ptr[16];
  HWY_STATIC_DISPATCH(_sqrtx16_f32)(a_ptr, result_ptr);
  f32x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x16_v));
  return result;
}

f32x16_v fmaf32x16_static(const f32x16_v a, const f32x16_v b,
                          const f32x16_v c) {
  const float *a_ptr = reinterpret_cast<const float *>(&a);
  const float *b_ptr = reinterpret_cast<const float *>(&b);
  const float *c_ptr = reinterpret_cast<const float *>(&c);
  alignas(64) float result_ptr[16];
  HWY_STATIC_DISPATCH(_fmax16_f32)(a_ptr, b_ptr, c_ptr, result_ptr);
  f32x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f32x16_v));
  return result;
}

#endif // 512-bits

/* 1024-bits */
#if HWY_MAX_BYTES >= 128

/* binary64 */

f64x16_v addf64x16_static(const f64x16_v a, const f64x16_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(128) double result_ptr[16];
  HWY_STATIC_DISPATCH(_addx16_f64)(a_ptr, b_ptr, result_ptr);
  f64x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x16_v));
  return result;
}

f64x16_v subf64x16_static(const f64x16_v a, const f64x16_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(128) double result_ptr[16];
  HWY_STATIC_DISPATCH(_subx16_f64)(a_ptr, b_ptr, result_ptr);
  f64x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x16_v));
  return result;
}

f64x16_v mulf64x16_static(const f64x16_v a, const f64x16_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(128) double result_ptr[16];
  HWY_STATIC_DISPATCH(_mulx16_f64)(a_ptr, b_ptr, result_ptr);
  f64x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x16_v));
  return result;
}

f64x16_v divf64x16_static(const f64x16_v a, const f64x16_v b) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  alignas(128) double result_ptr[16];
  HWY_STATIC_DISPATCH(_divx16_f64)(a_ptr, b_ptr, result_ptr);
  f64x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x16_v));
  return result;
}

f64x16_v sqrtf64x16_static(const f64x16_v a) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  alignas(128) double result_ptr[16];
  HWY_STATIC_DISPATCH(_sqrtx16_f64)(a_ptr, result_ptr);
  f64x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x16_v));
  return result;
}

f64x16_v fmaf64x16_static(const f64x16_v a, const f64x16_v b,
                          const f64x16_v c) {
  const double *a_ptr = reinterpret_cast<const double *>(&a);
  const double *b_ptr = reinterpret_cast<const double *>(&b);
  const double *c_ptr = reinterpret_cast<const double *>(&c);
  alignas(128) double result_ptr[16];
  HWY_STATIC_DISPATCH(_fmax16_f64)(a_ptr, b_ptr, c_ptr, result_ptr);
  f64x16_v result;
  std::memcpy(&result, result_ptr, sizeof(f64x16_v));
  return result;
}

#endif // 1024-bits

#define define_un_op_dynamic_dispatch(op, type, size)                          \
  void op##type##x##size##_dynamic(const type##x##size##_v a,                  \
                                   type##x##size##_v &result) {                \
    type##x##size##_u a_union = {.vector = a};                                 \
    type##x##size##_u result_union;                                            \
    op##type(a_union.array, result_union.array, size);                         \
    result = result_union.vector;                                              \
  }

#define define_bin_op_dynamic_dispatch(op, type, size)                         \
  void op##type##x##size##_dynamic(const type##x##size##_v a,                  \
                                   const type##x##size##_v b,                  \
                                   type##x##size##_v &result) {                \
    type##x##size##_u a_union = {.vector = a};                                 \
    type##x##size##_u b_union = {.vector = b};                                 \
    type##x##size##_u result_union;                                            \
    op##type(a_union.array, b_union.array, result_union.array, size);          \
    result = result_union.vector;                                              \
  }

#define define_ter_op_dynamic_dispatch(op, type, size)                         \
  void op##type##x##size##_dynamic(                                            \
      const type##x##size##_v a, const type##x##size##_v b,                    \
      const type##x##size##_v c, type##x##size##_v &result) {                  \
    type##x##size##_u a_union = {.vector = a};                                 \
    type##x##size##_u b_union = {.vector = b};                                 \
    type##x##size##_u c_union = {.vector = c};                                 \
    type##x##size##_u result_union;                                            \
    op##type(a_union.array, b_union.array, c_union.array, result_union.array,  \
             size);                                                            \
    result = result_union.vector;                                              \
  }

// if the vector types are native, they are never passed by pointers, so we call
// directly the dynamic dispatch tp avoid ABI mismatches
// For x86_64, the vector types f32x2, f32x4 and f64x2 are native to the target

#define define_un_op_dynamic_dispatch_native(op, type, size)                   \
  void op##type##x##size##_dynamic(const type##x##size##_v a,                  \
                                   type##x##size##_v &result) {                \
    type##x##size##_u a_union = {.vector = a};                                 \
    type##x##size##_u result_union;                                            \
    HWY_DYNAMIC_DISPATCH(_##op##x##size##_##type)                              \
    (a_union.array, result_union.array);                                       \
    result = result_union.vector;                                              \
  }

#define define_bin_op_dynamic_dispatch_native(op, type, size)                  \
  void op##type##x##size##_dynamic(const type##x##size##_v a,                  \
                                   const type##x##size##_v b,                  \
                                   type##x##size##_v &result) {                \
    type##x##size##_u a_union = {.vector = a};                                 \
    type##x##size##_u b_union = {.vector = b};                                 \
    type##x##size##_u result_union;                                            \
    HWY_DYNAMIC_DISPATCH(_##op##x##size##_##type)                              \
    (a_union.array, b_union.array, result_union.array);                        \
    result = result_union.vector;                                              \
  }

#define define_ter_op_dynamic_dispatch_native(op, type, size)                  \
  void op##type##x##size##_dynamic(                                            \
      const type##x##size##_v a, const type##x##size##_v b,                    \
      const type##x##size##_v c, type##x##size##_v &result) {                  \
    type##x##size##_u a_union = {.vector = a};                                 \
    type##x##size##_u b_union = {.vector = b};                                 \
    type##x##size##_u c_union = {.vector = c};                                 \
    type##x##size##_u result_union;                                            \
    HWY_DYNAMIC_DISPATCH(_##op##x##size##_##type)                              \
    (a_union.array, b_union.array, c_union.array, result_union.array);         \
    result = result_union.vector;                                              \
  }

/* Single vector functions, dynamic dispatch */

/* IEEE-754 binary32 x2 */

union f32x2_u {
  f32x2_v vector;
  alignas(8) float array[2];
};

#if defined(__x86_64__)
define_bin_op_dynamic_dispatch_native(add, f32, 2);
define_bin_op_dynamic_dispatch_native(sub, f32, 2);
define_bin_op_dynamic_dispatch_native(mul, f32, 2);
define_bin_op_dynamic_dispatch_native(div, f32, 2);
define_un_op_dynamic_dispatch_native(sqrt, f32, 2);
define_ter_op_dynamic_dispatch_native(fma, f32, 2);
#else
define_bin_op_dynamic_dispatch(add, f32, 2);
define_bin_op_dynamic_dispatch(sub, f32, 2);
define_bin_op_dynamic_dispatch(mul, f32, 2);
define_bin_op_dynamic_dispatch(div, f32, 2);
define_un_op_dynamic_dispatch(sqrt, f32, 2);
define_ter_op_dynamic_dispatch(fma, f32, 2);
#endif

/* IEEE-754 binary64 x2 */

union f64x2_u {
  f64x2_v vector;
  alignas(16) double array[2];
};

#if defined(__x86_64__)
define_bin_op_dynamic_dispatch_native(add, f64, 2);
define_bin_op_dynamic_dispatch_native(sub, f64, 2);
define_bin_op_dynamic_dispatch_native(mul, f64, 2);
define_bin_op_dynamic_dispatch_native(div, f64, 2);
define_un_op_dynamic_dispatch_native(sqrt, f64, 2);
define_ter_op_dynamic_dispatch_native(fma, f64, 2);
#else
define_bin_op_dynamic_dispatch(add, f64, 2);
define_bin_op_dynamic_dispatch(sub, f64, 2);
define_bin_op_dynamic_dispatch(mul, f64, 2);
define_bin_op_dynamic_dispatch(div, f64, 2);
define_un_op_dynamic_dispatch(sqrt, f64, 2);
define_ter_op_dynamic_dispatch(fma, f64, 2);
#endif

/* IEEE-754 binary32 x4 */

union f32x4_u {
  f32x4_v vector;
  alignas(16) float array[4];
};

#if defined(__x86_64__)
define_bin_op_dynamic_dispatch_native(add, f32, 4);
define_bin_op_dynamic_dispatch_native(sub, f32, 4);
define_bin_op_dynamic_dispatch_native(mul, f32, 4);
define_bin_op_dynamic_dispatch_native(div, f32, 4);
define_un_op_dynamic_dispatch_native(sqrt, f32, 4);
define_ter_op_dynamic_dispatch_native(fma, f32, 4);
#else
define_bin_op_dynamic_dispatch(add, f32, 4);
define_bin_op_dynamic_dispatch(sub, f32, 4);
define_bin_op_dynamic_dispatch(mul, f32, 4);
define_bin_op_dynamic_dispatch(div, f32, 4);
define_un_op_dynamic_dispatch(sqrt, f32, 4);
define_ter_op_dynamic_dispatch(fma, f32, 4);
#endif

/* IEEE-754 binary64 x4 */

union f64x4_u {
  f64x4_v vector;
  alignas(32) double array[4];
};

define_bin_op_dynamic_dispatch(add, f64, 4);
define_bin_op_dynamic_dispatch(sub, f64, 4);
define_bin_op_dynamic_dispatch(mul, f64, 4);
define_bin_op_dynamic_dispatch(div, f64, 4);
define_un_op_dynamic_dispatch(sqrt, f64, 4);
define_ter_op_dynamic_dispatch(fma, f64, 4);

/* IEEE-754 binary32 x8 */

union f32x8_u {
  f32x8_v vector;
  alignas(32) float array[8];
};

define_bin_op_dynamic_dispatch(add, f32, 8);
define_bin_op_dynamic_dispatch(sub, f32, 8);
define_bin_op_dynamic_dispatch(mul, f32, 8);
define_bin_op_dynamic_dispatch(div, f32, 8);
define_un_op_dynamic_dispatch(sqrt, f32, 8);
define_ter_op_dynamic_dispatch(fma, f32, 8);

/* IEEE-754 binary64 x8 */

union f64x8_u {
  f64x8_v vector;
  alignas(64) double array[8];
};

define_bin_op_dynamic_dispatch(add, f64, 8);
define_bin_op_dynamic_dispatch(sub, f64, 8);
define_bin_op_dynamic_dispatch(mul, f64, 8);
define_bin_op_dynamic_dispatch(div, f64, 8);
define_un_op_dynamic_dispatch(sqrt, f64, 8);
define_ter_op_dynamic_dispatch(fma, f64, 8);

/* IEEE-754 binary32 x16 */

union f32x16_u {
  f32x16_v vector;
  alignas(64) float array[16];
};

define_bin_op_dynamic_dispatch(add, f32, 16);
define_bin_op_dynamic_dispatch(sub, f32, 16);
define_bin_op_dynamic_dispatch(mul, f32, 16);
define_bin_op_dynamic_dispatch(div, f32, 16);
define_un_op_dynamic_dispatch(sqrt, f32, 16);
define_ter_op_dynamic_dispatch(fma, f32, 16);

/* IEEE-754 binary64 x16 */

union f64x16_u {
  f64x16_v vector;
  alignas(128) double array[16];
};

define_bin_op_dynamic_dispatch(add, f64, 16);
define_bin_op_dynamic_dispatch(sub, f64, 16);
define_bin_op_dynamic_dispatch(mul, f64, 16);
define_bin_op_dynamic_dispatch(div, f64, 16);
define_un_op_dynamic_dispatch(sqrt, f64, 16);
define_ter_op_dynamic_dispatch(fma, f64, 16);

} // namespace prism::ud::vector

#endif // HWY_ONCE

#if HWY_ONCE
namespace prism::ud::scalar {

float addf32(float a, float b) { return ud::scalar::add<float>(a, b); }
float subf32(float a, float b) { return ud::scalar::sub<float>(a, b); }
float mulf32(float a, float b) { return ud::scalar::mul<float>(a, b); }
float divf32(float a, float b) { return ud::scalar::div<float>(a, b); }
float sqrtf32(float a) { return ud::scalar::sqrt<float>(a); }
float fmaf32(float a, float b, float c) {
  return ud::scalar::fma<float>(a, b, c);
}

double addf64(double a, double b) { return ud::scalar::add<double>(a, b); }
double subf64(double a, double b) { return ud::scalar::sub<double>(a, b); }
double mulf64(double a, double b) { return ud::scalar::mul<double>(a, b); }
double divf64(double a, double b) { return ud::scalar::div<double>(a, b); }
double sqrtf64(double a) { return ud::scalar::sqrt<double>(a); }
double fmaf64(double a, double b, double c) {
  return ud::scalar::fma<double>(a, b, c);
}

} // namespace prism::ud::scalar
#endif // HWY_ONCE