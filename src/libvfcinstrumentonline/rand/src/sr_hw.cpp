
#include "src/debug.hpp"
#include "src/sr.h"

// Generates code for every target that this compiler can support.
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "src/sr_hw.cpp" // this file
#include "hwy/foreach_target.h"            // must come before highway.h

// clang-format off
#include "hwy/highway.h"
#include "src/sr_hw-inl.h"
#include "src/sr_hw.h"
#include "src/xoroshiro256+_hw-inl.hpp"
// clang-format on

// Optional, can instead add HWY_ATTR to all functions.
HWY_BEFORE_NAMESPACE();
namespace prism::sr::vector {

namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;
namespace dbg = prism::vector::HWY_NAMESPACE;

namespace internal {

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
inline V _add_fp_xN(const V a, const V b) {
  auto res = add<D>(a, b);
  std::cout << "# _add_fp_XN: " << std::endl;
  dbg::debug_vec<D>("# a:", a);
  dbg::debug_vec<D>("# b:", b);
  dbg::debug_vec<D>("# result:", res);
  std::cout << "# :_add_fp_xN" << std::endl;
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
inline V _sub_fp_xN(const V a, const V b) {
  auto res = sub<D>(a, b);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
inline V _mul_fp_xN(const V a, const V b) {
  auto res = mul<D>(a, b);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
inline V _div_fp_xN(const V a, const V b) {
  auto res = div<D>(a, b);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
inline V _sqrt_fp_xN(const V a) {
  auto res = sqrt<D>(a);
  return res;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
inline V _fma_fp_xN(const V a, const V b, const V c) {
  auto res = fma<D>(a, b, c);
  return res;
}

#define define_vector_un_op_wrapper(op, vtype, ftype, size)                    \
  inline vtype##x##size _##op##_##vtype##_x##size(const vtype##x##size a) {    \
    return _##op##_fp_xN<ftype, size>(a);                                      \
  }

#define define_vector_bin_op_wrapper(op, vtype, ftype, size)                   \
  inline vtype##x##size _##op##_##vtype##_x##size(const vtype##x##size a,      \
                                                  const vtype##x##size b) {    \
    return _##op##_fp_xN<ftype, size>(a, b);                                   \
  }

#define define_vector_ter_op_wrapper(op, vtype, ftype, size)                   \
  inline vtype##x##size _##op##_##vtype##_x##size(const vtype##x##size a,      \
                                                  const vtype##x##size b,      \
                                                  const vtype##x##size c) {    \
    return _##op##_fp_xN<ftype, size>(a, b, c);                                \
  }

/* 32-bits */
#if HWY_MAX_BYTES >= 4
using f32x1_t = hn::FixedTag<float, 1>;
using f32x1 = hn::Vec<f32x1_t>;
define_vector_bin_op_wrapper(add, f32, float, 1);
define_vector_bin_op_wrapper(sub, f32, float, 1);
define_vector_bin_op_wrapper(mul, f32, float, 1);
define_vector_bin_op_wrapper(div, f32, float, 1);
define_vector_un_op_wrapper(sqrt, f32, float, 1);
define_vector_ter_op_wrapper(fma, f32, float, 1);
#endif

/* 64-bits */
#if HWY_MAX_BYTES >= 8
using f64x1_t = hn::FixedTag<double, 1>;
using f64x1 = hn::Vec<f64x1_t>;
define_vector_bin_op_wrapper(add, f64, double, 1);
define_vector_bin_op_wrapper(sub, f64, double, 1);
define_vector_bin_op_wrapper(mul, f64, double, 1);
define_vector_bin_op_wrapper(div, f64, double, 1);
define_vector_un_op_wrapper(sqrt, f64, double, 1);
define_vector_ter_op_wrapper(fma, f64, double, 1);

using f32x2_t = hn::FixedTag<float, 2>;
using f32x2 = hn::Vec<f32x2_t>;
define_vector_bin_op_wrapper(add, f32, float, 2);
define_vector_bin_op_wrapper(sub, f32, float, 2);
define_vector_bin_op_wrapper(mul, f32, float, 2);
define_vector_bin_op_wrapper(div, f32, float, 2);
define_vector_un_op_wrapper(sqrt, f32, float, 2);
define_vector_ter_op_wrapper(fma, f32, float, 2);

#endif

/* 128-bits */
#if HWY_MAX_BYTES >= 16
using f64x2_t = hn::FixedTag<double, 2>;
using f64x2 = hn::Vec<f64x2_t>;
define_vector_bin_op_wrapper(add, f64, double, 2);
define_vector_bin_op_wrapper(sub, f64, double, 2);
define_vector_bin_op_wrapper(mul, f64, double, 2);
define_vector_bin_op_wrapper(div, f64, double, 2);
define_vector_un_op_wrapper(sqrt, f64, double, 2);
define_vector_ter_op_wrapper(fma, f64, double, 2);

using f32x4_t = hn::FixedTag<float, 4>;
using f32x4 = hn::Vec<f32x4_t>;
define_vector_bin_op_wrapper(add, f32, float, 4);
define_vector_bin_op_wrapper(sub, f32, float, 4);
define_vector_bin_op_wrapper(mul, f32, float, 4);
define_vector_bin_op_wrapper(div, f32, float, 4);
define_vector_un_op_wrapper(sqrt, f32, float, 4);
define_vector_ter_op_wrapper(fma, f32, float, 4);
#endif

/* 256-bits */
#if HWY_MAX_BYTES >= 32
using f64x4_t = hn::FixedTag<double, 4>;
using f64x4 = hn::Vec<f64x4_t>;
define_vector_bin_op_wrapper(add, f64, double, 4);
define_vector_bin_op_wrapper(sub, f64, double, 4);
define_vector_bin_op_wrapper(mul, f64, double, 4);
define_vector_bin_op_wrapper(div, f64, double, 4);
define_vector_un_op_wrapper(sqrt, f64, double, 4);
define_vector_ter_op_wrapper(fma, f64, double, 4);

using f32x8_t = hn::FixedTag<float, 8>;
using f32x8 = hn::Vec<f32x8_t>;
define_vector_bin_op_wrapper(add, f32, float, 8);
define_vector_bin_op_wrapper(sub, f32, float, 8);
define_vector_bin_op_wrapper(mul, f32, float, 8);
define_vector_bin_op_wrapper(div, f32, float, 8);
define_vector_un_op_wrapper(sqrt, f32, float, 8);
define_vector_ter_op_wrapper(fma, f32, float, 8);
#endif

/* 512-bits */
#if HWY_MAX_BYTES >= 64
using f64x8_t = hn::FixedTag<double, 8>;
using f64x8 = hn::Vec<f64x8_t>;
define_vector_bin_op_wrapper(add, f64, double, 8);
define_vector_bin_op_wrapper(sub, f64, double, 8);
define_vector_bin_op_wrapper(mul, f64, double, 8);
define_vector_bin_op_wrapper(div, f64, double, 8);
define_vector_un_op_wrapper(sqrt, f64, double, 8);
define_vector_ter_op_wrapper(fma, f64, double, 8);

using f32x16_t = hn::FixedTag<float, 16>;
using f32x16 = hn::Vec<f32x16_t>;
define_vector_bin_op_wrapper(add, f32, float, 16);
define_vector_bin_op_wrapper(sub, f32, float, 16);
define_vector_bin_op_wrapper(mul, f32, float, 16);
define_vector_bin_op_wrapper(div, f32, float, 16);
define_vector_un_op_wrapper(sqrt, f32, float, 16);
define_vector_ter_op_wrapper(fma, f32, float, 16);
#endif

/* 1024-bits */
#if HWY_MAX_BYTES >= 128
using f64x16_t = hn::FixedTag<double, 16>;
using f64x16 = hn::Vec<f64x16_t>;
define_vector_bin_op_wrapper(add, f64, double, 16);
define_vector_bin_op_wrapper(sub, f64, double, 16);
define_vector_bin_op_wrapper(mul, f64, double, 16);
define_vector_bin_op_wrapper(div, f64, double, 16);
define_vector_un_op_wrapper(sqrt, f64, double, 16);
define_vector_ter_op_wrapper(fma, f64, double, 16);
#endif

} // namespace internal

template <typename T, std::size_t N>
void _print(const T *HWY_RESTRICT a, const char *msg) {
  std::cout << msg;
  for (int i = 0; i < N; i++) {
    std::cout << a[i] << " ";
  }
  std::cout << std::endl;
}

template <typename T, std::size_t N, class D = hn::FixedTag<T, N>,
          class V = hn::Vec<D>>
void _addxN(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
            T *HWY_RESTRICT result) {
  std::cout << "# _addxN: " << std::endl;
  _print<T, N>(a, "# a: ");
  _print<T, N>(b, "# b: ");
  const D d;
  const auto va = hn::Load(d, a);
  const auto vb = hn::Load(d, b);
  dbg::debug_vec<D>("# va:", va);
  dbg::debug_vec<D>("# vb:", vb);
  auto res = internal::_add_fp_xN<T, N>(va, vb);
  dbg::debug_vec<D>("# vresult:", res);
  hn::Store(res, d, result);
  _print<T, N>(result, "# result: ");
  std::cout << "# :_addxN" << std::endl;
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
                               type *HWY_RESTRICT result) {                    \
    _##op##xN<type, size>(a, result);                                          \
  }

#define define_fp_xN_bin_op(type, name, size, op)                              \
  void _##op##x##size##_##name(const type *HWY_RESTRICT a,                     \
                               const type *HWY_RESTRICT b,                     \
                               type *HWY_RESTRICT result) {                    \
    _##op##xN<type, size>(a, b, result);                                       \
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
void _round(const T *HWY_RESTRICT sigma, const T *HWY_RESTRICT tau,
            T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const D d;
  const size_t N = hn::Lanes(d);

  if (count >= N) {
    for (size_t i = 0; i + N <= count; i += N) {
      auto sigma_vec = hn::Load(d, sigma + i);
      auto tau_vec = hn::Load(d, tau + i);
      auto res = round<D>(sigma_vec, tau_vec);
      hn::Store(res, d, result + i);
    }
  } else {
    using C = hn::CappedTag<T, 1>;
    const C c;
    for (size_t i = 0; i < count; ++i) {
      auto sigma_vec = hn::Load(c, sigma + i);
      auto tau_vec = hn::Load(c, tau + i);
      auto res = round<C>(sigma_vec, tau_vec);
      hn::Store(res, c, result + i);
    }
  }
}

// TODO: make two versions: one fixed size and one scalable
template <typename T>
void _add(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
          T *HWY_RESTRICT result, const size_t count) {

  using D = hn::ScalableTag<T>;
  const D d;
  const size_t N = hn::Lanes(d);

  if (count >= N) {
    for (size_t i = 0; i + N <= count; i += N) {
      auto a_vec = hn::Load(d, a + i);
      auto b_vec = hn::Load(d, b + i);
      auto res = add<D>(a_vec, b_vec);
      hn::Store(res, d, result + i);
    }
  } else {
    using C = hn::CappedTag<T, 1>;
    const C c;
    for (size_t i = 0; i < count; ++i) {
      auto a_vec = hn::Load(c, a + i);
      auto b_vec = hn::Load(c, b + i);
      auto res = add<C>(a_vec, b_vec);
      hn::Store(res, c, result + i);
    }
  }

  std::cout << "# _add: " << std::endl;
  std::cout << "# a: ";
  for (int j = 0; j < count; j++) {
    std::cout << a[j] << " ";
  }
  std::cout << std::endl;
  std::cout << "# b: ";
  for (int j = 0; j < count; j++) {
    std::cout << b[j] << " ";
  }
  std::cout << std::endl;
  std::cout << "# result: ";
  for (int j = 0; j < count; j++) {
    std::cout << result[j] << " ";
  }
  std::cout << std::endl;
}

template <typename T>
void _sub(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
          T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const D d;
  const size_t N = hn::Lanes(d);

  if (count >= N) {
    for (size_t i = 0; i + N <= count; i += N) {
      auto a_vec = hn::Load(d, a + i);
      auto b_vec = hn::Load(d, b + i);
      auto res = sub<D>(a_vec, b_vec);
      hn::Store(res, d, result + i);
    }
  } else {
    using C = hn::CappedTag<T, 1>;
    const C c;
    for (size_t i = 0; i < count; ++i) {
      auto a_vec = hn::Load(c, a + i);
      auto b_vec = hn::Load(c, b + i);
      auto res = sub<C>(a_vec, b_vec);
      hn::Store(res, c, result + i);
    }
  }
}

template <typename T>
void _mul(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
          T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);

  if (count >= N) {
    for (size_t i = 0; i + N <= count; i += N) {
      auto a_vec = hn::Load(d, a + i);
      auto b_vec = hn::Load(d, b + i);
      auto res = mul<D>(a_vec, b_vec);
      hn::Store(res, d, result + i);
    }
  } else {
    using C = hn::CappedTag<T, 1>;
    const C c;
    for (size_t i = 0; i < count; ++i) {
      auto a_vec = hn::Load(c, a + i);
      auto b_vec = hn::Load(c, b + i);
      auto res = mul<C>(a_vec, b_vec);
      hn::Store(res, c, result + i);
    }
  }
}

template <typename T>
void _div(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
          T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);

  if (count >= N) {
    for (size_t i = 0; i + N <= count; i += N) {
      auto a_vec = hn::Load(d, a + i);
      auto b_vec = hn::Load(d, b + i);
      auto res = div<D>(a_vec, b_vec);
      hn::Store(res, d, result + i);
    }
  } else {
    using C = hn::CappedTag<T, 1>;
    const C c;
    for (size_t i = 0; i < count; ++i) {
      auto a_vec = hn::Load(c, a + i);
      auto b_vec = hn::Load(c, b + i);
      auto res = div<C>(a_vec, b_vec);
      hn::Store(res, c, result + i);
    }
  }
}

template <typename T>
void _sqrt(const T *HWY_RESTRICT a, T *HWY_RESTRICT result,
           const size_t count) {
  using D = hn::ScalableTag<T>;
  const hn::ScalableTag<T> d;
  const size_t N = hn::Lanes(d);

  if (count >= N) {
    for (size_t i = 0; i + N <= count; i += N) {
      auto a_vec = hn::Load(d, a + i);
      auto res = sqrt<D>(a_vec);
      hn::Store(res, d, result + i);
    }
  } else {
    using C = hn::CappedTag<T, 1>;
    const C c;
    for (size_t i = 0; i < count; ++i) {
      auto a_vec = hn::Load(c, a + i);
      auto res = sqrt<C>(a_vec);
      hn::Store(res, c, result + i);
    }
  }
}

template <typename T>
void _fma(const T *HWY_RESTRICT a, const T *HWY_RESTRICT b,
          const T *HWY_RESTRICT c, T *HWY_RESTRICT result, const size_t count) {
  using D = hn::ScalableTag<T>;
  const D d;
  const size_t N = hn::Lanes(d);

  if (count >= N) {
    for (size_t i = 0; i + N <= count; i += N) {
      auto a_vec = hn::Load(d, a + i);
      auto b_vec = hn::Load(d, b + i);
      auto c_vec = hn::Load(d, c + i);
      auto res = fma<D>(a_vec, b_vec, c_vec);
      hn::Store(res, d, result + i);
    }
  } else {
    using C = hn::CappedTag<T, 1>;
    const C ct;
    for (size_t i = 0; i < count; ++i) {
      auto a_vec = hn::Load(ct, a + i);
      auto b_vec = hn::Load(ct, b + i);
      auto c_vec = hn::Load(ct, c + i);
      auto res = fma<C>(a_vec, b_vec, c_vec);
      hn::Store(res, ct, result + i);
    }
  }
}

/* binary32 */

void _round_f32(const float *HWY_RESTRICT sigma, const float *HWY_RESTRICT tau,
                float *HWY_RESTRICT result, const size_t count) {
  _round<float>(sigma, tau, result, count);
}

void _add_f32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              float *HWY_RESTRICT result, const size_t count) {
  std::cout << "# _add_f32: " << std::endl;
  std::cout << "# a: ";
  for (int i = 0; i < count; i++) {
    std::cout << a[i] << " ";
  }
  std::cout << std::endl;
  std::cout << "# b: ";
  for (int i = 0; i < count; i++) {
    std::cout << b[i] << " ";
  }
  std::cout << std::endl;
  _add<float>(a, b, result, count);
  std::cout << "# result: ";
  for (int i = 0; i < count; i++) {
    std::cout << result[i] << " ";
  }
  std::cout << std::endl;
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

void _fma_f32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
              const float *HWY_RESTRICT c, float *HWY_RESTRICT result,
              const size_t count) {
  _fma<float>(a, b, c, result, count);
}

/* binary64 */

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

void _fma_f64(const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,
              const double *HWY_RESTRICT c, double *HWY_RESTRICT result,
              const size_t count) {
  _fma<double>(a, b, c, result, count);
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
} // namespace prism::sr::vector
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace prism::sr::vector {

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
  std::cout << "# addf32" << std::endl;
  std::cout << "# a: ";
  for (int i = 0; i < count; i++) {
    std::cout << a[i] << " ";
  }
  std::cout << std::endl;
  std::cout << "# b: ";
  for (int i = 0; i < count; i++) {
    std::cout << b[i] << " ";
  }
  add(a, b, result, count);
  std::cout << "# result: ";
  for (int i = 0; i < count; i++) {
    std::cout << result[i] << " ";
  }
  std::cout << std::endl;
}

void subf32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
            float *HWY_RESTRICT result, const size_t count) {
  sub(a, b, result, count);
}

void mulf32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
            float *HWY_RESTRICT result, const size_t count) {
  mul(a, b, result, count);
}

void divf32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
            float *HWY_RESTRICT result, const size_t count) {
  div(a, b, result, count);
}

void sqrtf32(const float *HWY_RESTRICT a, float *HWY_RESTRICT result,
             const size_t count) {
  sqrt(a, result, count);
}

void fmaf32(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
            const float *HWY_RESTRICT c, float *HWY_RESTRICT result,
            const size_t count) {
  fma(a, b, c, result, count);
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

// void addf32x2(const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,
//               float *HWY_RESTRICT result) {
//   HWY_DYNAMIC_DISPATCH(_addx2_f32)(a, b, result);
// }

#define define_array_unary_op_dynamic(name, count)                             \
  void name##f32##x##count##_dynamic(const float *HWY_RESTRICT a,              \
                                     float *HWY_RESTRICT result) {             \
    return name<float>(a, result, count);                                      \
  }                                                                            \
  void name##f64x##count##_dynamic(const double *HWY_RESTRICT a,               \
                                   double *HWY_RESTRICT result) {              \
    return name<double>(a, result, count);                                     \
  }

#define define_array_bin_op_dynamic(name, count)                               \
  void name##f32##x##count##_dynamic(const float *HWY_RESTRICT a,              \
                                     const float *HWY_RESTRICT b,              \
                                     float *HWY_RESTRICT result) {             \
    return name<float>(a, b, result, count);                                   \
  }                                                                            \
  void name##f64x##count##_dynamic(const double *HWY_RESTRICT a,               \
                                   const double *HWY_RESTRICT b,               \
                                   double *HWY_RESTRICT result) {              \
    return name<double>(a, b, result, count);                                  \
  }

#define define_array_ter_op_dynamic(name, count)                               \
  void name##f32##x##count##_dynamic(                                          \
      const float *HWY_RESTRICT a, const float *HWY_RESTRICT b,                \
      const float *HWY_RESTRICT c, float *HWY_RESTRICT result) {               \
    return name<float>(a, b, c, result, count);                                \
  }                                                                            \
  void name##f64x##count##_dynamic(                                            \
      const double *HWY_RESTRICT a, const double *HWY_RESTRICT b,              \
      const double *HWY_RESTRICT c, double *HWY_RESTRICT result) {             \
    return name<double>(a, b, c, result, count);                               \
  }

/* 64-bits */
define_array_bin_op_dynamic(add, 2);
define_array_bin_op_dynamic(sub, 2);
define_array_bin_op_dynamic(mul, 2);
define_array_bin_op_dynamic(div, 2);
define_array_unary_op_dynamic(sqrt, 2);
define_array_ter_op_dynamic(fma, 2);

/* 128-bits */
define_array_bin_op_dynamic(add, 4);
define_array_bin_op_dynamic(sub, 4);
define_array_bin_op_dynamic(mul, 4);
define_array_bin_op_dynamic(div, 4);
define_array_unary_op_dynamic(sqrt, 4);
define_array_ter_op_dynamic(fma, 4);

/* 256-bits */
define_array_bin_op_dynamic(add, 8);
define_array_bin_op_dynamic(sub, 8);
define_array_bin_op_dynamic(mul, 8);
define_array_bin_op_dynamic(div, 8);
define_array_unary_op_dynamic(sqrt, 8);
define_array_ter_op_dynamic(fma, 8);

/* 512-bits */
define_array_bin_op_dynamic(add, 16);
define_array_bin_op_dynamic(sub, 16);
define_array_bin_op_dynamic(mul, 16);
define_array_bin_op_dynamic(div, 16);
define_array_unary_op_dynamic(sqrt, 16);
define_array_ter_op_dynamic(fma, 16);

/* 1024-bits */
define_array_bin_op_dynamic(add, 32);
define_array_bin_op_dynamic(sub, 32);
define_array_bin_op_dynamic(mul, 32);
define_array_bin_op_dynamic(div, 32);
define_array_unary_op_dynamic(sqrt, 32);
define_array_ter_op_dynamic(fma, 32);

union f32x2_u {
  f32x2_v vector;
  alignas(8) float array[2];
};
union f64x2_u {
  f64x2_v vector;
  alignas(16) double array[2];
};
union f32x4_u {
  f32x4_v vector;
  alignas(16) float array[4];
};

union f64x4_u {
  f64x4_v vector;
  alignas(32) double array[4];
};
union f32x8_u {
  f32x8_v vector;
  alignas(32) float array[8];
};
union f64x8_u {
  f64x8_v vector;
  alignas(64) double array[8];
};
union f32x16_u {
  f32x16_v vector;
  alignas(64) float array[16];
};
union f64x16_u {
  f64x16_v vector;
  alignas(128) double array[16];
};

/* Single vector functions with static dispatch */

/* 64-bits */

#if HWY_MAX_BYTES >= 8

/* binary32 */

f32x2_v addf32x2_static(const f32x2_v a, const f32x2_v b) {
  f32x2_u a_union = {.vector = a};
  f32x2_u b_union = {.vector = b};
  f32x2_u result_union;
  HWY_STATIC_DISPATCH(_addx2_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x2_v subf32x2_static(const f32x2_v a, const f32x2_v b) {
  f32x2_u a_union = {.vector = a};
  f32x2_u b_union = {.vector = b};
  f32x2_u result_union;
  HWY_STATIC_DISPATCH(_subx2_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x2_v mulf32x2_static(const f32x2_v a, const f32x2_v b) {
  f32x2_u a_union = {.vector = a};
  f32x2_u b_union = {.vector = b};
  f32x2_u result_union;
  HWY_STATIC_DISPATCH(_mulx2_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x2_v divf32x2_static(const f32x2_v a, const f32x2_v b) {
  f32x2_u a_union = {.vector = a};
  f32x2_u b_union = {.vector = b};
  f32x2_u result_union;
  HWY_STATIC_DISPATCH(_divx2_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x2_v sqrtf32x2_static(const f32x2_v a) {
  f32x2_u a_union = {.vector = a};
  f32x2_u result_union;
  HWY_STATIC_DISPATCH(_sqrtx2_f32)
  (a_union.array, result_union.array);
  return result_union.vector;
}

f32x2_v fmaf32x2_static(const f32x2_v a, const f32x2_v b, const f32x2_v c) {
  f32x2_u a_union = {.vector = a};
  f32x2_u b_union = {.vector = b};
  f32x2_u c_union = {.vector = c};
  f32x2_u result_union;
  HWY_STATIC_DISPATCH(_fmax2_f32)
  (a_union.array, b_union.array, c_union.array, result_union.array);
  return result_union.vector;
}

#endif

/* 128-bits */

#if HWY_MAX_BYTES >= 16

/* binary64 */

f64x2_v addf64x2_static(const f64x2_v a, const f64x2_v b) {
  f64x2_u a_union = {.vector = a};
  f64x2_u b_union = {.vector = b};
  f64x2_u result_union;
  HWY_STATIC_DISPATCH(_addx2_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x2_v subf64x2_static(const f64x2_v a, const f64x2_v b) {
  f64x2_u a_union = {.vector = a};
  f64x2_u b_union = {.vector = b};
  f64x2_u result_union;
  HWY_STATIC_DISPATCH(_subx2_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x2_v mulf64x2_static(const f64x2_v a, const f64x2_v b) {
  f64x2_u a_union = {.vector = a};
  f64x2_u b_union = {.vector = b};
  f64x2_u result_union;
  HWY_STATIC_DISPATCH(_mulx2_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x2_v divf64x2_static(const f64x2_v a, const f64x2_v b) {
  f64x2_u a_union = {.vector = a};
  f64x2_u b_union = {.vector = b};
  f64x2_u result_union;
  HWY_STATIC_DISPATCH(_divx2_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x2_v sqrtf64x2_static(const f64x2_v a) {
  f64x2_u a_union = {.vector = a};
  f64x2_u result_union;
  HWY_STATIC_DISPATCH(_sqrtx2_f64)
  (a_union.array, result_union.array);
  return result_union.vector;
}

f64x2_v fmaf64x2_static(const f64x2_v a, const f64x2_v b, const f64x2_v c) {
  f64x2_u a_union = {.vector = a};
  f64x2_u b_union = {.vector = b};
  f64x2_u c_union = {.vector = c};
  f64x2_u result_union;
  HWY_STATIC_DISPATCH(_fmax2_f64)
  (a_union.array, b_union.array, c_union.array, result_union.array);
  return result_union.vector;
}

/* binary32 */

f32x4_v addf32x4_static(const f32x4_v a, const f32x4_v b) {
  f32x4_u a_union = {.vector = a};
  f32x4_u b_union = {.vector = b};
  f32x4_u result_union;
  HWY_STATIC_DISPATCH(_addx4_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x4_v subf32x4_static(const f32x4_v a, const f32x4_v b) {
  f32x4_u a_union = {.vector = a};
  f32x4_u b_union = {.vector = b};
  f32x4_u result_union;
  HWY_STATIC_DISPATCH(_subx4_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x4_v mulf32x4_static(const f32x4_v a, const f32x4_v b) {
  f32x4_u a_union = {.vector = a};
  f32x4_u b_union = {.vector = b};
  f32x4_u result_union;
  HWY_STATIC_DISPATCH(_mulx4_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x4_v divf32x4_static(const f32x4_v a, const f32x4_v b) {
  f32x4_u a_union = {.vector = a};
  f32x4_u b_union = {.vector = b};
  f32x4_u result_union;
  HWY_STATIC_DISPATCH(_divx4_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x4_v sqrtf32x4_static(const f32x4_v a) {
  f32x4_u a_union = {.vector = a};
  f32x4_u result_union;
  HWY_STATIC_DISPATCH(_sqrtx4_f32)
  (a_union.array, result_union.array);
  return result_union.vector;
}

f32x4_v fmaf32x4_static(const f32x4_v a, const f32x4_v b, const f32x4_v c) {
  f32x4_u a_union = {.vector = a};
  f32x4_u b_union = {.vector = b};
  f32x4_u c_union = {.vector = c};
  f32x4_u result_union;
  HWY_STATIC_DISPATCH(_fmax4_f32)
  (a_union.array, b_union.array, c_union.array, result_union.array);
  return result_union.vector;
}

#endif

/* 256-bits */

#if HWY_MAX_BYTES >= 32

/* binary64 */

template <typename T, size_t N>
void _print(const T array, const std::string &prefix) {
  std::cout << prefix;
  for (size_t i = 0; i < N; i++) {
    std::cout << array[i] << " ";
  }
  std::cout << std::endl;
}

f64x4_v addf64x4_static(const f64x4_v a, const f64x4_v b) {
  std::cout << "# addf64x4_static:" << std::endl;
  _print<f64x4_v, 4>(a, "# a: ");
  _print<f64x4_v, 4>(b, "# b: ");
  f64x4_u a_union = {.vector = a};
  f64x4_u b_union = {.vector = b};
  _print<double[], 4>(a_union.array, "# a_union:");
  _print<double[], 4>(b_union.array, "# b_union:");
  f64x4_u result_union;
  HWY_STATIC_DISPATCH(_addx4_f64)
  (a_union.array, b_union.array, result_union.array);
  _print<double[], 4>(result_union.array, "# result_union: ");
  std::cout << "# :addf64x4_static" << std::endl;
  return result_union.vector;
}

f64x4_v subf64x4_static(const f64x4_v a, const f64x4_v b) {
  f64x4_u a_union = {.vector = a};
  f64x4_u b_union = {.vector = b};
  f64x4_u result_union;
  HWY_STATIC_DISPATCH(_subx4_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x4_v mulf64x4_static(const f64x4_v a, const f64x4_v b) {
  f64x4_u a_union = {.vector = a};
  f64x4_u b_union = {.vector = b};
  f64x4_u result_union;
  HWY_STATIC_DISPATCH(_mulx4_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x4_v divf64x4_static(const f64x4_v a, const f64x4_v b) {
  f64x4_u a_union = {.vector = a};
  f64x4_u b_union = {.vector = b};
  f64x4_u result_union;
  HWY_STATIC_DISPATCH(_divx4_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x4_v sqrtf64x4_static(const f64x4_v a) {
  f64x4_u a_union = {.vector = a};
  f64x4_u result_union;
  HWY_STATIC_DISPATCH(_sqrtx4_f64)
  (a_union.array, result_union.array);
  return result_union.vector;
}

f64x4_v fmaf64x4_static(const f64x4_v a, const f64x4_v b, const f64x4_v c) {
  f64x4_u a_union = {.vector = a};
  f64x4_u b_union = {.vector = b};
  f64x4_u c_union = {.vector = c};
  f64x4_u result_union;
  HWY_STATIC_DISPATCH(_fmax4_f64)
  (a_union.array, b_union.array, c_union.array, result_union.array);
  return result_union.vector;
}

/* binary32 */

f32x8_v addf32x8_static(const f32x8_v a, const f32x8_v b) {
  f32x8_u a_union = {.vector = a};
  f32x8_u b_union = {.vector = b};
  f32x8_u result_union;
  HWY_STATIC_DISPATCH(_addx8_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x8_v subf32x8_static(const f32x8_v a, const f32x8_v b) {
  f32x8_u a_union = {.vector = a};
  f32x8_u b_union = {.vector = b};
  f32x8_u result_union;
  HWY_STATIC_DISPATCH(_subx8_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x8_v mulf32x8_static(const f32x8_v a, const f32x8_v b) {
  f32x8_u a_union = {.vector = a};
  f32x8_u b_union = {.vector = b};
  f32x8_u result_union;
  HWY_STATIC_DISPATCH(_mulx8_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x8_v divf32x8_static(const f32x8_v a, const f32x8_v b) {
  f32x8_u a_union = {.vector = a};
  f32x8_u b_union = {.vector = b};
  f32x8_u result_union;
  HWY_STATIC_DISPATCH(_divx8_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x8_v sqrtf32x8_static(const f32x8_v a) {
  f32x8_u a_union = {.vector = a};
  f32x8_u result_union;
  HWY_STATIC_DISPATCH(_sqrtx8_f32)
  (a_union.array, result_union.array);
  return result_union.vector;
}

f32x8_v fmaf32x8_static(const f32x8_v a, const f32x8_v b, const f32x8_v c) {
  f32x8_u a_union = {.vector = a};
  f32x8_u b_union = {.vector = b};
  f32x8_u c_union = {.vector = c};
  f32x8_u result_union;
  HWY_STATIC_DISPATCH(_fmax8_f32)
  (a_union.array, b_union.array, c_union.array, result_union.array);
  return result_union.vector;
}

#endif

/* 512-bits */

#if HWY_MAX_BYTES >= 64

/* binary64 */

f64x8_v addf64x8_static(const f64x8_v a, const f64x8_v b) {
  f64x8_u a_union = {.vector = a};
  f64x8_u b_union = {.vector = b};
  f64x8_u result_union;
  HWY_STATIC_DISPATCH(_addx8_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x8_v subf64x8_static(const f64x8_v a, const f64x8_v b) {
  f64x8_u a_union = {.vector = a};
  f64x8_u b_union = {.vector = b};
  f64x8_u result_union;
  HWY_STATIC_DISPATCH(_subx8_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x8_v mulf64x8_static(const f64x8_v a, const f64x8_v b) {
  f64x8_u a_union = {.vector = a};
  f64x8_u b_union = {.vector = b};
  f64x8_u result_union;
  HWY_STATIC_DISPATCH(_mulx8_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x8_v divf64x8_static(const f64x8_v a, const f64x8_v b) {
  f64x8_u a_union = {.vector = a};
  f64x8_u b_union = {.vector = b};
  f64x8_u result_union;
  HWY_STATIC_DISPATCH(_divx8_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x8_v sqrtf64x8_static(const f64x8_v a) {
  f64x8_u a_union = {.vector = a};
  f64x8_u result_union;
  HWY_STATIC_DISPATCH(_sqrtx8_f64)
  (a_union.array, result_union.array);
  return result_union.vector;
}

f64x8_v fmaf64x8_static(const f64x8_v a, const f64x8_v b, const f64x8_v c) {
  f64x8_u a_union = {.vector = a};
  f64x8_u b_union = {.vector = b};
  f64x8_u c_union = {.vector = c};
  f64x8_u result_union;
  HWY_STATIC_DISPATCH(_fmax8_f64)
  (a_union.array, b_union.array, c_union.array, result_union.array);
  return result_union.vector;
}

/* binary32 */

f32x16_v addf32x16_static(const f32x16_v a, const f32x16_v b) {
  f32x16_u a_union = {.vector = a};
  f32x16_u b_union = {.vector = b};
  f32x16_u result_union;
  HWY_STATIC_DISPATCH(_addx16_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x16_v subf32x16_static(const f32x16_v a, const f32x16_v b) {
  f32x16_u a_union = {.vector = a};
  f32x16_u b_union = {.vector = b};
  f32x16_u result_union;
  HWY_STATIC_DISPATCH(_subx16_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x16_v mulf32x16_static(const f32x16_v a, const f32x16_v b) {
  f32x16_u a_union = {.vector = a};
  f32x16_u b_union = {.vector = b};
  f32x16_u result_union;
  HWY_STATIC_DISPATCH(_mulx16_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x16_v divf32x16_static(const f32x16_v a, const f32x16_v b) {
  f32x16_u a_union = {.vector = a};
  f32x16_u b_union = {.vector = b};
  f32x16_u result_union;
  HWY_STATIC_DISPATCH(_divx16_f32)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f32x16_v sqrtf32x16_static(const f32x16_v a) {
  f32x16_u a_union = {.vector = a};
  f32x16_u result_union;
  HWY_STATIC_DISPATCH(_sqrtx16_f32)
  (a_union.array, result_union.array);
  return result_union.vector;
}

f32x16_v fmaf32x16_static(const f32x16_v a, const f32x16_v b,
                          const f32x16_v c) {
  f32x16_u a_union = {.vector = a};
  f32x16_u b_union = {.vector = b};
  f32x16_u c_union = {.vector = c};
  f32x16_u result_union;
  HWY_STATIC_DISPATCH(_fmax16_f32)
  (a_union.array, b_union.array, c_union.array, result_union.array);
  return result_union.vector;
}

#endif // 512-bits

/* 1024-bits */
#if HWY_MAX_BYTES >= 128

/* binary64 */

f64x16_v addf64x16_static(const f64x16_v a, const f64x16_v b) {
  f64x16_u a_union = {.vector = a};
  f64x16_u b_union = {.vector = b};
  f64x16_u result_union;
  HWY_STATIC_DISPATCH(_addx16_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x16_v subf64x16_static(const f64x16_v a, const f64x16_v b) {
  f64x16_u a_union = {.vector = a};
  f64x16_u b_union = {.vector = b};
  f64x16_u result_union;
  HWY_STATIC_DISPATCH(_subx16_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x16_v mulf64x16_static(const f64x16_v a, const f64x16_v b) {
  f64x16_u a_union = {.vector = a};
  f64x16_u b_union = {.vector = b};
  f64x16_u result_union;
  HWY_STATIC_DISPATCH(_mulx16_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x16_v divf64x16_static(const f64x16_v a, const f64x16_v b) {
  f64x16_u a_union = {.vector = a};
  f64x16_u b_union = {.vector = b};
  f64x16_u result_union;
  HWY_STATIC_DISPATCH(_divx16_f64)
  (a_union.array, b_union.array, result_union.array);
  return result_union.vector;
}

f64x16_v sqrtf64x16_static(const f64x16_v a) {
  f64x16_u a_union = {.vector = a};
  f64x16_u result_union;
  HWY_STATIC_DISPATCH(_sqrtx16_f64)
  (a_union.array, result_union.array);
  return result_union.vector;
}

f64x16_v fmaf64x16_static(const f64x16_v a, const f64x16_v b,
                          const f64x16_v c) {
  f64x16_u a_union = {.vector = a};
  f64x16_u b_union = {.vector = b};
  f64x16_u c_union = {.vector = c};
  f64x16_u result_union;
  HWY_STATIC_DISPATCH(_fmax16_f64)
  (a_union.array, b_union.array, c_union.array, result_union.array);
  return result_union.vector;
}

#endif // 1024-bits

} // namespace prism::sr::vector

#endif // HWY_ONCE

#if HWY_ONCE
namespace prism::sr::scalar {

float addf32(float a, float b) { return sr::scalar::add<float>(a, b); }
float subf32(float a, float b) { return sr::scalar::sub<float>(a, b); }
float mulf32(float a, float b) { return sr::scalar::mul<float>(a, b); }
float divf32(float a, float b) { return sr::scalar::div<float>(a, b); }
float sqrtf32(float a) { return sr::scalar::sqrt<float>(a); }
float fmaf32(float a, float b, float c) {
  return sr::scalar::fma<float>(a, b, c);
}

double addf64(double a, double b) { return sr::scalar::add<double>(a, b); }
double subf64(double a, double b) { return sr::scalar::sub<double>(a, b); }
double mulf64(double a, double b) { return sr::scalar::mul<double>(a, b); }
double divf64(double a, double b) { return sr::scalar::div<double>(a, b); }
double sqrtf64(double a) { return sr::scalar::sqrt<double>(a); }
double fmaf64(double a, double b, double c) {
  return sr::scalar::fma<double>(a, b, c);
}

} // namespace prism::sr::scalar

#endif // HWY_ONCE