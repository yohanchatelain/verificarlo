#include "hwy/nanobenchmark.h"

#include <chrono>
#include <numeric>
#include <stddef.h>
#include <stdio.h>

#include "gtest/gtest.h"

#include "hwy/tests/hwy_gtest.h"
#include "hwy/tests/test_util-inl.h"

#include "src/sr_hw.h"

namespace sr {
namespace vector {

using VecArgf32 = hwy::AlignedUniquePtr<float[]>;
using VecArgf64 = hwy::AlignedUniquePtr<double[]>;

using binary_f32_op = void (*)(const VecArgf32 &, const VecArgf32 &,
                               const VecArgf32 &, const size_t);
using binary_f64_op = void (*)(const VecArgf64 &, const VecArgf64 &,
                               const VecArgf64 &, const size_t);

template <typename T> constexpr const char *GetFormatString() {
  if constexpr (std::is_same_v<T, float>) {
    return "%+.6a ";
  } else {
    return "%+.13a ";
  }
}

template <std::size_t S, typename T, typename Op, std::size_t N = 10>
void MeasureFunction(Op func, const std::size_t lanes = 0,
                     const bool verbose = false) {

  constexpr size_t inputs_size = S;
  const auto a = hwy::MakeUniqueAlignedArray<T>(inputs_size);
  const auto b = hwy::MakeUniqueAlignedArray<T>(inputs_size);
  auto c = hwy::MakeUniqueAlignedArray<T>(inputs_size);
  constexpr T ulp = std::is_same_v<T, float> ? 0x1.0p-24f : 0x1.0p-53;
  constexpr const char *fmt = GetFormatString<T>();

  for (size_t i = 0; i < inputs_size; i++) {
    a[i] = 1.0;
    b[i] = ulp;
  }

  std::vector<double> times(N);

  for (size_t i = 0; i < N; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    func(a, b, c, inputs_size);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    times[i] = diff.count();
    if (verbose)
      std::cout << "Iteration: " << i << " time: " << diff.count() << " s\n";

    if (verbose) {
      for (size_t i = 0; i < inputs_size; i++) {
        fprintf(stderr, fmt, c[i]);
        if ((lanes != 0) and ((i % lanes) == (lanes - 1))) {
          fprintf(stderr, "\n");
        }
      }
      fprintf(stderr, "\n");
    }
  }

  // min, median, mean, max
  auto min = *std::min_element(times.begin(), times.end());
  auto max = *std::max_element(times.begin(), times.end());
  auto mean = std::accumulate(times.begin(), times.end(), 0.0) / N;
  auto std = std::sqrt(
      std::inner_product(times.begin(), times.end(), times.begin(), 0.0) / N -
      mean * mean);

  fprintf(stderr, "[%-4zu] ", inputs_size);
  fprintf(stderr, "%.4e ± %.4e [%.4e - %.4e] (%zu)\n", mean, std, min, max, N);
}

template <std::size_t S, typename T, typename V, typename Op,
          std::size_t N = 10>
void MeasureFunctionX(Op func, const std::size_t lanes = 0,
                      const bool verbose = false) {

  constexpr size_t inputs_size = S;
  constexpr T ulp = std::is_same_v<T, float> ? 0x1.0p-24 : 0x1.0p-53;
  V a = {0};
  V b = {0};
  constexpr const char *fmt = GetFormatString<T>();

  for (size_t i = 0; i < inputs_size; i++) {
    a[i] = 1.0;
    b[i] = ulp;
  }

  std::vector<double> times(N);

  for (size_t i = 0; i < N; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    auto c = func(a, b, inputs_size);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    times[i] = diff.count();
    if (verbose)
      std::cout << "Iteration: " << i << " time: " << diff.count() << " s\n";

    if (verbose) {
      for (size_t j = 0; j < inputs_size; j++) {
        fprintf(stderr, fmt, ((T *)&c)[j]);
        if ((lanes != 0) and ((j % lanes) == (lanes - 1))) {
          fprintf(stderr, "\n");
        }
      }
      fprintf(stderr, "\n");
    }
  }

  // min, median, mean, max
  auto min = *std::min_element(times.begin(), times.end());
  auto max = *std::max_element(times.begin(), times.end());
  auto mean = std::accumulate(times.begin(), times.end(), 0.0) / N;
  auto std = std::sqrt(
      std::inner_product(times.begin(), times.end(), times.begin(), 0.0) / N -
      mean * mean);

  fprintf(stderr, "[%-4zu] ", inputs_size);
  fprintf(stderr, "%.4e ± %.4e [%.4e - %.4e] (%zu)\n", mean, std, min, max, N);
}

/* Test array inputs */

#define define_array_test(op, type)                                            \
  void test_##op##type(const VecArg##type &a, const VecArg##type &b,           \
                       const VecArg##type &c, const size_t count) {            \
    op##type(a.get(), b.get(), c.get(), count);                                \
  }

define_array_test(add, f32);
define_array_test(sub, f32);
define_array_test(mul, f32);
define_array_test(div, f32);

define_array_test(add, f64);
define_array_test(sub, f64);
define_array_test(mul, f64);
define_array_test(div, f64);

/* Test vector pointer inputs */

#define define_array_test_vector(op, type, size)                               \
  void test_##op##type##x##size(const VecArg##type &a, const VecArg##type &b,  \
                                const VecArg##type &c, const size_t count) {   \
    op##type##x##size(a.get(), b.get(), c.get());                              \
  }

/* 64-bits */
#if HWY_MAX_BYTES >= 8
define_array_test_vector(add, f32, 2);
define_array_test_vector(sub, f32, 2);
define_array_test_vector(mul, f32, 2);
define_array_test_vector(div, f32, 2);
#endif

/* 128-bits */
#if HWY_MAX_BYTES >= 16
define_array_test_vector(add, f32, 4);
define_array_test_vector(sub, f32, 4);
define_array_test_vector(mul, f32, 4);
define_array_test_vector(div, f32, 4);

define_array_test_vector(add, f64, 2);
define_array_test_vector(sub, f64, 2);
define_array_test_vector(mul, f64, 2);
define_array_test_vector(div, f64, 2);
#endif

/* 256-bits */
#if HWY_MAX_BYTES >= 32
define_array_test_vector(add, f32, 8);
define_array_test_vector(sub, f32, 8);
define_array_test_vector(mul, f32, 8);
define_array_test_vector(div, f32, 8);

define_array_test_vector(add, f64, 4);
define_array_test_vector(sub, f64, 4);
define_array_test_vector(mul, f64, 4);
define_array_test_vector(div, f64, 4);
#endif

/* 512-bits */
#if HWY_MAX_BYTES >= 64
define_array_test_vector(add, f64, 8);
define_array_test_vector(sub, f64, 8);
define_array_test_vector(mul, f64, 8);
define_array_test_vector(div, f64, 8);

define_array_test_vector(add, f32, 16);
define_array_test_vector(sub, f32, 16);
define_array_test_vector(mul, f32, 16);
define_array_test_vector(div, f32, 16);
#endif

/* Test vector inputs (static dispatch) */

#define define_vector_test(op, type, size)                                     \
  type##x##size##_v test_##op##type##x##size##_v(const type##x##size##_v a,    \
                                                 const type##x##size##_v b,    \
                                                 const size_t count) {         \
    return op##type##x##size##_v(a, b);                                        \
  }

/* 64-bits */

#if HWY_MAX_BYTES >= 8
define_vector_test(add, f32, 2);
define_vector_test(sub, f32, 2);
define_vector_test(mul, f32, 2);
define_vector_test(div, f32, 2);
#endif

/* 128-bits */

#if HWY_MAX_BYTES >= 16
define_vector_test(add, f64, 2);
define_vector_test(sub, f64, 2);
define_vector_test(mul, f64, 2);
define_vector_test(div, f64, 2);

define_vector_test(add, f32, 4);
define_vector_test(sub, f32, 4);
define_vector_test(mul, f32, 4);
define_vector_test(div, f32, 4);
#endif

/* 256-bits */
#if HWY_MAX_BYTES >= 32
define_vector_test(add, f64, 4);
define_vector_test(sub, f64, 4);
define_vector_test(mul, f64, 4);
define_vector_test(div, f64, 4);

define_vector_test(add, f32, 8);
define_vector_test(sub, f32, 8);
define_vector_test(mul, f32, 8);
define_vector_test(div, f32, 8);
#endif

/* 512-bits */
#if HWY_MAX_BYTES >= 64
define_vector_test(add, f64, 8);
define_vector_test(sub, f64, 8);
define_vector_test(mul, f64, 8);
define_vector_test(div, f64, 8);

define_vector_test(add, f32, 16);
define_vector_test(sub, f32, 16);
define_vector_test(mul, f32, 16);
define_vector_test(div, f32, 16);
#endif

/* Test vector inputs (dynamic dispatch) */

#define define_vector_test_dynamic(op, type, size)                             \
  type##x##size##_v test_##op##type##x##size##_d(const type##x##size##_v a,    \
                                                 const type##x##size##_v b,    \
                                                 const size_t count) {         \
    return op##type##x##size##_d(a, b);                                        \
  }

define_vector_test_dynamic(add, f32, 2);
define_vector_test_dynamic(sub, f32, 2);
define_vector_test_dynamic(mul, f32, 2);
define_vector_test_dynamic(div, f32, 2);

define_vector_test_dynamic(add, f64, 2);
define_vector_test_dynamic(sub, f64, 2);
define_vector_test_dynamic(mul, f64, 2);
define_vector_test_dynamic(div, f64, 2);

define_vector_test_dynamic(add, f32, 4);
define_vector_test_dynamic(sub, f32, 4);
define_vector_test_dynamic(mul, f32, 4);
define_vector_test_dynamic(div, f32, 4);

define_vector_test_dynamic(add, f64, 4);
define_vector_test_dynamic(sub, f64, 4);
define_vector_test_dynamic(mul, f64, 4);
define_vector_test_dynamic(div, f64, 4);

define_vector_test_dynamic(add, f32, 8);
define_vector_test_dynamic(sub, f32, 8);
define_vector_test_dynamic(mul, f32, 8);
define_vector_test_dynamic(div, f32, 8);

define_vector_test_dynamic(add, f64, 8);
define_vector_test_dynamic(sub, f64, 8);
define_vector_test_dynamic(mul, f64, 8);
define_vector_test_dynamic(div, f64, 8);

define_vector_test_dynamic(add, f32, 16);
define_vector_test_dynamic(sub, f32, 16);
define_vector_test_dynamic(mul, f32, 16);
define_vector_test_dynamic(div, f32, 16);

// Recursion function to call MeasureFunction with powers of 2
template <size_t N, size_t Max, typename T, typename Op>
void callMeasureFunctions(Op function) {
  MeasureFunction<N, T>(function);
  if constexpr (N * 2 <= Max) {
    callMeasureFunctions<N * 2, Max, T, Op>(function);
  }
}

/* Test on Array inputs */

/* IEEE-754 binary32 */

TEST(SRArrayBenchmark, SRAddF32) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32 with " << N << " repetitions\n";
  callMeasureFunctions<2, 1024, float>(&test_addf32);
}

TEST(SRArrayBenchmark, SRSubF32) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32 with " << N << " repetitions\n";
  callMeasureFunctions<2, 1024, float>(&test_subf32);
}

TEST(SRArrayBenchmark, SRMulF32) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32 with " << N << " repetitions\n";
  callMeasureFunctions<2, 1024, float>(&test_mulf32);
}

TEST(SRArrayBenchmark, SRDivF32) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32 with " << N << " repetitions\n";
  callMeasureFunctions<2, 1024, float>(&test_divf32);
}

/* IEEE-754 binary64 */

TEST(SRArrayBenchmark, SRAddF64) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf64 with " << N << " repetitions\n";
  callMeasureFunctions<2, 1024, double>(&test_addf64);
}

TEST(SRArrayBenchmark, SRSubF64) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf64 with " << N << " repetitions\n";
  callMeasureFunctions<2, 1024, double>(&test_subf64);
}

TEST(SRArrayBenchmark, SRMulF64) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf64 with " << N << " repetitions\n";
  callMeasureFunctions<2, 1024, double>(&test_mulf64);
}

TEST(SRArrayBenchmark, SRDivF64) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf64 with " << N << " repetitions\n";
  callMeasureFunctions<2, 1024, double>(&test_divf64);
}

/* Test on single vector passed by pointer */

constexpr bool kVerbose = false;

/* IEEE-754 binary32 x2 */

TEST(SRVectorPtrBenchmark, SRAddF32x2) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x2 with " << N << " repetitions\n";
  MeasureFunction<2, float>(&test_addf32x2, 2, kVerbose);
}

TEST(SRVectorPtrBenchmark, SRSubF32x2) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x2 with " << N << " repetitions\n";
  MeasureFunction<2, float>(&test_subf32x2, 2, kVerbose);
}

TEST(SRVectorPtrBenchmark, SRMulF32x2) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x2 with " << N << " repetitions\n";
  MeasureFunction<2, float>(&test_mulf32x2, 2, kVerbose);
}

TEST(SRVectorPtrBenchmark, SRDivF32x2) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x2 with " << N << " repetitions\n";
  MeasureFunction<2, float>(&test_divf32x2, 2, kVerbose);
}

/* IEEE-754 binary64 x2 */

TEST(SRVectorPtrBenchmark, SRAddF64x2) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf64x2 with " << N << " repetitions\n";
  MeasureFunction<2, double>(&test_addf64x2, 2, kVerbose);
}

TEST(SRVectorPtrBenchmark, SRSubF64x2) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf64x2 with " << N << " repetitions\n";
  MeasureFunction<2, double>(&test_subf64x2, 2, kVerbose);
}

TEST(SRVectorPtrBenchmark, SRMulF64x2) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf64x2 with " << N << " repetitions\n";
  MeasureFunction<2, double>(&test_mulf64x2, 2, kVerbose);
}

TEST(SRVectorPtrBenchmark, SRDivF64x2) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf64x2 with " << N << " repetitions\n";
  MeasureFunction<2, double>(&test_divf64x2, 2, kVerbose);
}

/* IEEE-754 binary32 x4 */

TEST(SRVectorPtrBenchmark, SRAddF32x4) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x4 with " << N << " repetitions\n";
  MeasureFunction<4, float>(&test_addf32x4, 4, kVerbose);
#else
  GTEST_SKIP() << "SRAddF32x4 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRSubF32x4) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x4 with " << N << " repetitions\n";
  MeasureFunction<4, float>(&test_subf32x4, 4, kVerbose);
#else
  GTEST_SKIP() << "SRSubF32x4 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRMulF32x4) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x4 with " << N << " repetitions\n";
  MeasureFunction<4, float>(&test_mulf32x4, 4, kVerbose);
#else
  GTEST_SKIP() << "SRMulF32x4 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRDivF32x4) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x4 with " << N << " repetitions\n";
  MeasureFunction<4, float>(&test_divf32x4, 4, kVerbose);
#else
  GTEST_SKIP() << "SRDivF32x4 is not available";
#endif
}

/* IEEE-754 binary32 x8 */

TEST(SRVectorPtrBenchmark, SRAddF32x8) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x8 with " << N << " repetitions\n";
  MeasureFunction<8, float>(&test_addf32x8, 8, kVerbose);
#else
  GTEST_SKIP() << "SRAddF32x8 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRSubF32x8) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x8 with " << N << " repetitions\n";
  MeasureFunction<8, float>(&test_subf32x8, 8, kVerbose);
#else
  GTEST_SKIP() << "SRSubF32x8 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRMulF32x8) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x8 with " << N << " repetitions\n";
  MeasureFunction<8, float>(&test_mulf32x8, 8, kVerbose);
#else
  GTEST_SKIP() << "SRMulF32x8 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRDivF32x8) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x8 with " << N << " repetitions\n";
  MeasureFunction<8, float>(&test_divf32x8, 8, kVerbose);
#else
  GTEST_SKIP() << "SRDivF32x8 is not available";
#endif
}

/* IEEE-754 binary64 x4 */

TEST(SRVectorPtrBenchmark, SRAddF64x4) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf64x4 with " << N << "repetitions\n";
  MeasureFunction<4, double>(&test_addf64x4, 4, kVerbose);
#else
  GTEST_SKIP() << "SRAddF64x4 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRSubF64x4) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf64x4 with " << N << "repetitions\n";
  MeasureFunction<4, double>(&test_subf64x4, 4, kVerbose);
#else
  GTEST_SKIP() << "SRSubF64x4 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRMulF64x4) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf64x4 with " << N << "repetitions\n";
  MeasureFunction<4, double>(&test_mulf64x4, 4, kVerbose);
#else
  GTEST_SKIP() << "SRMulF64x4 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRDivF64x4) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf64x4 with " << N << "repetitions\n";
  MeasureFunction<4, double>(&test_divf64x4, 4, kVerbose);
#else
  GTEST_SKIP() << "SRDivF64x4 is not available";
#endif
}

/* IEEE-754 binary64 x8 */

TEST(SRVectorPtrBenchmark, SRAddF64x8) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf64x8 with " << N << "repetitions\n";
  MeasureFunction<8, double>(&test_addf64x8, 8, kVerbose);
#else
  GTEST_SKIP() << "SRAddF64x8 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRSubF64x8) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf64x8 with " << N << "repetitions\n";
  MeasureFunction<8, double>(&test_subf64x8, 8, kVerbose);
#else
  GTEST_SKIP() << "SRSubF64x8 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRMulF64x8) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf64x8 with " << N << "repetitions\n";
  MeasureFunction<8, double>(&test_mulf64x8, 8, kVerbose);
#else
  GTEST_SKIP() << "SRMulF64x8 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRDivF64x8) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf64x8 with " << N << "repetitions\n";
  MeasureFunction<8, double>(&test_divf64x8, 8, kVerbose);
#else
  GTEST_SKIP() << "SRDivF64x8 is not available";
#endif
}

/* IEEE-754 binary32 x16 */

TEST(SRVectorPtrBenchmark, SRAddF32x16) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x16 with " << N << "repetitions\n";
  MeasureFunction<16, float>(&test_addf32x16, 16, kVerbose);
#else
  GTEST_SKIP() << "SRAddF32x16 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRSubF32x16) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x16 with " << N << "repetitions\n";
  MeasureFunction<16, float>(&test_subf32x16, 16, kVerbose);
#else
  GTEST_SKIP() << "SRSubF32x16 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRMulF32x16) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x16 with " << N << "repetitions\n";
  MeasureFunction<16, float>(&test_mulf32x16, 16, kVerbose);
#else
  GTEST_SKIP() << "SRMulF32x16 is not available";
#endif
}

TEST(SRVectorPtrBenchmark, SRDivF32x16) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x16 with " << N << "repetitions\n";
  MeasureFunction<16, float>(&test_divf32x16, 16, kVerbose);
#else
  GTEST_SKIP() << "SRDivF32x16 is not available";
#endif
}

/* Test on single vector passed by value with static dispatch */

/* IEEE-754 binary32 x2 */

TEST(SRVectorBenchmark, SRAddF32x2V) {
#if HWY_MAX_BYTES >= 8
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x2_v with " << N << " repetitions\n";
  MeasureFunctionX<2, float, f32x2_v>(&test_addf32x2_v, 2, kVerbose);
#else
  GTEST_SKIP() << "SRAddF32x2V is not available";
#endif
}

TEST(SRVectorBenchmark, SRSubF32x2V) {
#if HWY_MAX_BYTES >= 8
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x2_v with " << N << " repetitions\n";
  MeasureFunctionX<2, float, f32x2_v>(&test_subf32x2_v, 2, kVerbose);
#else
  GTEST_SKIP() << "SRSubF32x2V is not available";
#endif
}

TEST(SRVectorBenchmark, SRMulF32x2V) {
#if HWY_MAX_BYTES >= 8
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x2_v with " << N << " repetitions\n";
  MeasureFunctionX<2, float, f32x2_v>(&test_mulf32x2_v, 2, kVerbose);
#else
  GTEST_SKIP() << "SRMulF32x2V is not available";
#endif
}

TEST(SRVectorBenchmark, SRDivF32x2V) {
#if HWY_MAX_BYTES >= 8
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x2_v with " << N << " repetitions\n";
  MeasureFunctionX<2, float, f32x2_v>(&test_divf32x2_v, 2, kVerbose);
#else
  GTEST_SKIP() << "SRDivF32x2V is not available";
#endif
}

/* IEEE-754 binary64 x2 */
TEST(SRVectorBenchmark, SRAddF64x2V) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf64x2_v with " << N << " repetitions\n";
  MeasureFunctionX<2, double, f64x2_v>(&test_addf64x2_v, 2, kVerbose);
#else
  GTEST_SKIP() << "SRAddF64x2V is not available";
#endif
}

TEST(SRVectorBenchmark, SRSubF64x2V) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf64x2_v with " << N << " repetitions\n";
  MeasureFunctionX<2, double, f64x2_v>(&test_subf64x2_v, 2, kVerbose);
#else
  GTEST_SKIP() << "SRSubF64x2V is not available";
#endif
}

TEST(SRVectorBenchmark, SRMulF64x2V) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf64x2_v with " << N << " repetitions\n";
  MeasureFunctionX<2, double, f64x2_v>(&test_mulf64x2_v, 2, kVerbose);
#else
  GTEST_SKIP() << "SRMulF64x2V is not available";
#endif
}

TEST(SRVectorBenchmark, SRDivF64x2V) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf64x2_v with " << N << " repetitions\n";
  MeasureFunctionX<2, double, f64x2_v>(&test_divf64x2_v, 2, kVerbose);
#else
  GTEST_SKIP() << "SRDivF64x2V is not available";
#endif
}

/* IEEE-754 binary32 x4 */

TEST(SRVectorBenchmark, SRAddF32x4V) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x4_v with " << N << " repetitions\n";
  MeasureFunctionX<4, float, f32x4_v>(&test_addf32x4_v, 4, kVerbose);
#else
  GTEST_SKIP() << "SRAddF32x4V is not available";
#endif
}

TEST(SRVectorBenchmark, SRSubF32x4V) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x4_v with " << N << " repetitions\n";
  MeasureFunctionX<4, float, f32x4_v>(&test_subf32x4_v, 4, kVerbose);
#else
  GTEST_SKIP() << "SRSubF32x4V is not available";
#endif
}

TEST(SRVectorBenchmark, SRMulF32x4V) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x4_v with " << N << " repetitions\n";
  MeasureFunctionX<4, float, f32x4_v>(&test_mulf32x4_v, 4, kVerbose);
#else
  GTEST_SKIP() << "SRMulF32x4V is not available";
#endif
}

TEST(SRVectorBenchmark, SRDivF32x4V) {
#if HWY_MAX_BYTES >= 16
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x4_v with " << N << " repetitions\n";
  MeasureFunctionX<4, float, f32x4_v>(&test_divf32x4_v, 4, kVerbose);
#else
  GTEST_SKIP() << "SRDivF32x4V is not available";
#endif
}

/* IEEE-754 binary32 x8 */

TEST(SRVectorBenchmark, SRAddF32x8V) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x8_v with " << N << " repetitions\n";
  MeasureFunctionX<8, float, f32x8_v>(&test_addf32x8_v, 8, kVerbose);
#else
  GTEST_SKIP() << "SRAddF32x8V is not available";
#endif
}

TEST(SRVectorBenchmark, SRSubF32x8V) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x8_v with " << N << " repetitions\n";
  MeasureFunctionX<8, float, f32x8_v>(&test_subf32x8_v, 8, kVerbose);
#else
  GTEST_SKIP() << "SRSubF32x8V is not available";
#endif
}

TEST(SRVectorBenchmark, SRMulF32x8V) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x8_v with " << N << " repetitions\n";
  MeasureFunctionX<8, float, f32x8_v>(&test_mulf32x8_v, 8, kVerbose);
#else
  GTEST_SKIP() << "SRMulF32x8V is not available";
#endif
}

TEST(SRVectorBenchmark, SRDivF32x8V) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x8_v with " << N << " repetitions\n";
  MeasureFunctionX<8, float, f32x8_v>(&test_divf32x8_v, 8, kVerbose);
#else
  GTEST_SKIP() << "SRDivF32x8V is not available";
#endif
}

/* IEEE-754 binary64 x4 */

TEST(SRVectorBenchmark, SRAddF64x4V) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf64x4_v with " << N << " repetitions\n";
  MeasureFunctionX<4, double, f64x4_v>(&test_addf64x4_v, 4, kVerbose);
#else
  GTEST_SKIP() << "SRAddF64x4V is not available";
#endif
}

TEST(SRVectorBenchmark, SRSubF64x4V) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf64x4_v with " << N << " repetitions\n";
  MeasureFunctionX<4, double, f64x4_v>(&test_subf64x4_v, 4, kVerbose);
#else
  GTEST_SKIP() << "SRSubF64x4V is not available";
#endif
}

TEST(SRVectorBenchmark, SRMulF64x4V) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf64x4_v with " << N << " repetitions\n";
  MeasureFunctionX<4, double, f64x4_v>(&test_mulf64x4_v, 4, kVerbose);
#else
  GTEST_SKIP() << "SRMulF64x4V is not available";
#endif
}

TEST(SRVectorBenchmark, SRDivF64x4V) {
#if HWY_MAX_BYTES >= 32
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf64x4_v with " << N << " repetitions\n";
  MeasureFunctionX<4, double, f64x4_v>(&test_divf64x4_v, 4, kVerbose);
#else
  GTEST_SKIP() << "SRDivF64x4V is not available";
#endif
}

/* IEEE-754 binary64 x8 */

TEST(SRVectorBenchmark, SRAddF64x8V) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf64x8_v with " << N << " repetitions\n";
  MeasureFunctionX<8, double, f64x8_v>(&test_addf64x8_v, 8, kVerbose);
#else
  GTEST_SKIP() << "SRAddF64x8V is not available";
#endif
}

TEST(SRVectorBenchmark, SRSubF64x8V) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf64x8_v with " << N << " repetitions\n";
  MeasureFunctionX<8, double, f64x8_v>(&test_subf64x8_v, 8, kVerbose);

#else
  GTEST_SKIP() << "SRSubF64x8V is not available";
#endif
}

TEST(SRVectorBenchmark, SRMulF64x8V) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf64x8_v with " << N << " repetitions\n";
  MeasureFunctionX<8, double, f64x8_v>(&test_mulf64x8_v, 8, kVerbose);
#else
  GTEST_SKIP() << "SRMulF64x8V is not available";
#endif
}

TEST(SRVectorBenchmark, SRDivF64x8V) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf64x8_v with " << N << " repetitions\n";
  MeasureFunctionX<8, double, f64x8_v>(&test_divf64x8_v, 8, kVerbose);
#else
  GTEST_SKIP() << "SRDivF64x8V is not available";
#endif
}

/* IEEE-754 binary32 x16 */

TEST(SRVectorBenchmark, SRAddF32x16V) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x16_v with " << N
            << " repetitions\n";
  MeasureFunctionX<16, float, f32x16_v>(&test_addf32x16_v, 16, kVerbose);
#else
  GTEST_SKIP() << "SRAddF32x16V is not available";
#endif
}

TEST(SRVectorBenchmark, SRSubF32x16V) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x16_v with " << N
            << " repetitions\n";
  MeasureFunctionX<16, float, f32x16_v>(&test_subf32x16_v, 16, kVerbose);
#else
  GTEST_SKIP() << "SRSubF32x16V is not available";
#endif
}

TEST(SRVectorBenchmark, SRMulF32x16V) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x16_v with " << N
            << " repetitions\n";
  MeasureFunctionX<16, float, f32x16_v>(&test_mulf32x16_v, 16, kVerbose);
#else
  GTEST_SKIP() << "SRMulF32x16V is not available";
#endif
}

TEST(SRVectorBenchmark, SRDivF32x16V) {
#if HWY_MAX_BYTES >= 64
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x16_v with " << N
            << " repetitions\n";
  MeasureFunctionX<16, float, f32x16_v>(&test_divf32x16_v, 16, kVerbose);
#else
  GTEST_SKIP() << "SRDivF32x16V is not available";
#endif
}

/* Test on single vector passed by value with dynamic dispatch */

TEST(SRVectorDynamicBenchmark, SRAddF32x2D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x2_d with " << N << " repetitions\n";
  MeasureFunctionX<2, float, f32x2_v>(&test_addf32x2_d, 2, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRSubF32x2D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x2_d with " << N << " repetitions\n";
  MeasureFunctionX<2, float, f32x2_v>(&test_subf32x2_d, 2, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRMulF32x2D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x2_d with " << N << " repetitions\n";
  MeasureFunctionX<2, float, f32x2_v>(&test_mulf32x2_d, 2, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRDivF32x2D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x2_d with " << N << " repetitions\n";
  MeasureFunctionX<2, float, f32x2_v>(&test_divf32x2_d, 2, kVerbose);
}

/* IEEE-754 binary64 x2 */

TEST(SRVectorDynamicBenchmark, SRAddF64x2D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf64x2_d with " << N << " repetitions\n";
  MeasureFunctionX<2, double, f64x2_v>(&test_addf64x2_d, 2, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRSubF64x2D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf64x2_d with " << N << " repetitions\n";
  MeasureFunctionX<2, double, f64x2_v>(&test_subf64x2_d, 2, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRMulF64x2D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf64x2_d with " << N << " repetitions\n";
  MeasureFunctionX<2, double, f64x2_v>(&test_mulf64x2_d, 2, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRDivF64x2D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf64x2_d with " << N << " repetitions\n";
  MeasureFunctionX<2, double, f64x2_v>(&test_divf64x2_d, 2, kVerbose);
}

/* IEEE-754 binary32 x4 */

TEST(SRVectorDynamicBenchmark, SRAddF32x4D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x4_d with " << N << " repetitions\n";
  MeasureFunctionX<4, float, f32x4_v>(&test_addf32x4_d, 4, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRSubF32x4D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x4_d with " << N << " repetitions\n";
  MeasureFunctionX<4, float, f32x4_v>(&test_subf32x4_d, 4, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRMulF32x4D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x4_d with " << N << " repetitions\n";
  MeasureFunctionX<4, float, f32x4_v>(&test_mulf32x4_d, 4, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRDivF32x4D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x4_d with " << N << " repetitions\n";
  MeasureFunctionX<4, float, f32x4_v>(&test_divf32x4_d, 4, kVerbose);
}

/* IEEE-754 binary32 x8 */

TEST(SRVectorDynamicBenchmark, SRAddF32x8D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x8_d with " << N << " repetitions\n";
  MeasureFunctionX<8, float, f32x8_v>(&test_addf32x8_d, 8, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRSubF32x8D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x8_d with " << N << " repetitions\n";
  MeasureFunctionX<8, float, f32x8_v>(&test_subf32x8_d, 8, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRMulF32x8D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x8_d with " << N << " repetitions\n";
  MeasureFunctionX<8, float, f32x8_v>(&test_mulf32x8_d, 8, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRDivF32x8D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x8_d with " << N << " repetitions\n";
  MeasureFunctionX<8, float, f32x8_v>(&test_divf32x8_d, 8, kVerbose);
}

/* IEEE-754 binary64 x4 */

TEST(SRVectorDynamicBenchmark, SRAddF64x4D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf64x4_d with " << N << " repetitions\n";
  MeasureFunctionX<4, double, f64x4_v>(&test_addf64x4_d, 4, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRSubF64x4D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf64x4_d with " << N << " repetitions\n";
  MeasureFunctionX<4, double, f64x4_v>(&test_subf64x4_d, 4, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRMulF64x4D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf64x4_d with " << N << " repetitions\n";
  MeasureFunctionX<4, double, f64x4_v>(&test_mulf64x4_d, 4, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRDivF64x4D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf64x4_d with " << N << " repetitions\n";
  MeasureFunctionX<4, double, f64x4_v>(&test_divf64x4_d, 4, kVerbose);
}

/* IEEE-754 binary64 x8 */

TEST(SRVectorDynamicBenchmark, SRAddF64x8D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf64x8_d with " << N << " repetitions\n";
  MeasureFunctionX<8, double, f64x8_v>(&test_addf64x8_d, 8, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRSubF64x8D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf64x8_d with " << N << " repetitions\n";
  MeasureFunctionX<8, double, f64x8_v>(&test_subf64x8_d, 8, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRMulF64x8D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf64x8_d with " << N << " repetitions\n";
  MeasureFunctionX<8, double, f64x8_v>(&test_mulf64x8_d, 8, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRDivF64x8D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf64x8_d with " << N << " repetitions\n";
  MeasureFunctionX<8, double, f64x8_v>(&test_divf64x8_d, 8, kVerbose);
}

/* IEEE-754 binary32 x16 */

TEST(SRVectorDynamicBenchmark, SRAddF32x16D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x16_d with " << N
            << " repetitions\n";
  MeasureFunctionX<16, float, f32x16_v>(&test_addf32x16_d, 16, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRSubF32x16D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32x16_d with " << N
            << " repetitions\n";
  MeasureFunctionX<16, float, f32x16_v>(&test_subf32x16_d, 16, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRMulF32x16D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32x16_d with " << N
            << " repetitions\n";
  MeasureFunctionX<16, float, f32x16_v>(&test_mulf32x16_d, 16, kVerbose);
}

TEST(SRVectorDynamicBenchmark, SRDivF32x16D) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32x16_d with " << N
            << " repetitions\n";
  MeasureFunctionX<16, float, f32x16_v>(&test_divf32x16_d, 16, kVerbose);
}

} // namespace vector
} // namespace sr

// HYW_TEST_MAIN();1
