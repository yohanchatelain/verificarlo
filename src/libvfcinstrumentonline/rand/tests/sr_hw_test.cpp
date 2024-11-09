
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
namespace {

constexpr bool verbose = false;

using VecArgF32 = hwy::AlignedUniquePtr<float[]>;
using VecArgF64 = hwy::AlignedUniquePtr<double[]>;

using binary_f32_op = void (*)(const VecArgF32 &, const VecArgF32 &,
                               const VecArgF32 &, const size_t);
using binary_f64_op = void (*)(const VecArgF64 &, const VecArgF64 &,
                               const VecArgF64 &, const size_t);

template <std::size_t S, typename T, typename Op, std::size_t N = 10>
void MeasureFunction(Op func) {

  constexpr size_t inputs_size = S;
  const auto a = hwy::MakeUniqueAlignedArray<T>(inputs_size);
  const auto b = hwy::MakeUniqueAlignedArray<T>(inputs_size);
  auto c = hwy::MakeUniqueAlignedArray<T>(inputs_size);

  for (int i = 0; i < inputs_size; i++) {
    a[i] = 1.0;
    b[i] = 0x1.0p-24;
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
  }

  // min, median, mean, max
  auto min = *std::min_element(times.begin(), times.end());
  auto max = *std::max_element(times.begin(), times.end());
  auto mean = std::accumulate(times.begin(), times.end(), 0.0) / N;
  auto std = std::sqrt(
      std::inner_product(times.begin(), times.end(), times.begin(), 0.0) / N -
      mean * mean);

  fprintf(stderr, "[%-4zu] ", inputs_size);
  std::cerr << mean << " ± " << std << " [" << min << " - " << max << "] "
            << "(" << N << ")" << std::endl;
}

void test_addf32(const VecArgF32 &a, const VecArgF32 &b, const VecArgF32 &c,
                 const size_t count) {
  addf32(a.get(), b.get(), c.get(), count);
}

void test_subf32(const VecArgF32 &a, const VecArgF32 &b, const VecArgF32 &c,
                 const size_t count) {
  subf32(a.get(), b.get(), c.get(), count);
}

void test_mulf32(const VecArgF32 &a, const VecArgF32 &b, const VecArgF32 &c,
                 const size_t count) {
  mulf32(a.get(), b.get(), c.get(), count);
}

void test_divf32(const VecArgF32 &a, const VecArgF32 &b, const VecArgF32 &c,
                 const size_t count) {
  divf32(a.get(), b.get(), c.get(), count);
}

void test_addf64(const VecArgF64 &a, const VecArgF64 &b, const VecArgF64 &c,
                 const size_t count) {
  addf64(a.get(), b.get(), c.get(), count);
}

void test_subf64(const VecArgF64 &a, const VecArgF64 &b, const VecArgF64 &c,
                 const size_t count) {
  subf64(a.get(), b.get(), c.get(), count);
}

void test_mulf64(const VecArgF64 &a, const VecArgF64 &b, const VecArgF64 &c,
                 const size_t count) {
  mulf64(a.get(), b.get(), c.get(), count);
}

void test_divf64(const VecArgF64 &a, const VecArgF64 &b, const VecArgF64 &c,
                 const size_t count) {
  divf64(a.get(), b.get(), c.get(), count);
}

void test_addf32x2(const VecArgF32 &a, const VecArgF32 &b, const VecArgF32 &c,
                   const size_t count) {
  addf32x2(a.get(), b.get(), c.get());
}

void test_addf32x4(const VecArgF32 &a, const VecArgF32 &b, const VecArgF32 &c,
                   const size_t count) {
  addf32x4(a.get(), b.get(), c.get());
}

void test_addf32x8(const VecArgF32 &a, const VecArgF32 &b, const VecArgF32 &c,
                   const size_t count) {

#if HWY_MAX_BYTES >= 32
  addf32x8(a.get(), b.get(), c.get());
#endif
}

TEST(SRBenchmark, SRAddF32) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32 with " << N << " repetitions\n";
  MeasureFunction<32, float>(&test_addf32);
  MeasureFunction<64, float>(&test_addf32);
  MeasureFunction<128, float>(&test_addf32);
  MeasureFunction<256, float>(&test_addf32);
  MeasureFunction<512, float>(&test_addf32);
  MeasureFunction<1024, float>(&test_addf32);
}

TEST(SRBenchmark, SRSubF32) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf32 with " << N << " repetitions\n";
  MeasureFunction<32, float>(&test_subf32);
  MeasureFunction<64, float>(&test_subf32);
  MeasureFunction<128, float>(&test_subf32);
  MeasureFunction<256, float>(&test_subf32);
  MeasureFunction<512, float>(&test_subf32);
  MeasureFunction<1024, float>(&test_subf32);
}

TEST(SRBenchmark, SRMulF32) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf32 with " << N << " repetitions\n";
  MeasureFunction<32, float>(&test_mulf32);
  MeasureFunction<64, float>(&test_mulf32);
  MeasureFunction<128, float>(&test_mulf32);
  MeasureFunction<256, float>(&test_mulf32);
  MeasureFunction<512, float>(&test_mulf32);
  MeasureFunction<1024, float>(&test_mulf32);
}

TEST(SRBenchmark, SRDivF32) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf32 with " << N << " repetitions\n";
  MeasureFunction<32, float>(&test_divf32);
  MeasureFunction<64, float>(&test_divf32);
  MeasureFunction<128, float>(&test_divf32);
  MeasureFunction<256, float>(&test_divf32);
  MeasureFunction<512, float>(&test_divf32);
  MeasureFunction<1024, float>(&test_divf32);
}

TEST(SRBenchmark, SRAddF64) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf64 with " << N << " repetitions\n";
  MeasureFunction<32, double>(&test_addf64);
  MeasureFunction<64, double>(&test_addf64);
  MeasureFunction<128, double>(&test_addf64);
  MeasureFunction<256, double>(&test_addf64);
  MeasureFunction<512, double>(&test_addf64);
  MeasureFunction<1024, double>(&test_addf64);
}

TEST(SRBenchmark, SRSubF64) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::subf64 with " << N << " repetitions\n";
  MeasureFunction<32, double>(&test_subf64);
  MeasureFunction<64, double>(&test_subf64);
  MeasureFunction<128, double>(&test_subf64);
  MeasureFunction<256, double>(&test_subf64);
  MeasureFunction<512, double>(&test_subf64);
  MeasureFunction<1024, double>(&test_subf64);
}

TEST(SRBenchmark, SRMulF64) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::mulf64 with " << N << " repetitions\n";
  MeasureFunction<32, double>(&test_mulf64);
  MeasureFunction<64, double>(&test_mulf64);
  MeasureFunction<128, double>(&test_mulf64);
  MeasureFunction<256, double>(&test_mulf64);
  MeasureFunction<512, double>(&test_mulf64);
  MeasureFunction<1024, double>(&test_mulf64);
}

TEST(SRBenchmark, SRDivF64) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::divf64 with " << N << " repetitions\n";
  MeasureFunction<32, double>(&test_divf64);
  MeasureFunction<64, double>(&test_divf64);
  MeasureFunction<128, double>(&test_divf64);
  MeasureFunction<256, double>(&test_divf64);
  MeasureFunction<512, double>(&test_divf64);
  MeasureFunction<1024, double>(&test_divf64);
}

TEST(SRBenchmark, SRAddF32x2) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x2 with " << N << " repetitions\n";
  MeasureFunction<2, float>(&test_addf32x2);
  MeasureFunction<2, float>(&test_addf32x2);
  MeasureFunction<4, float>(&test_addf32x2);
}

TEST(SRBenchmark, SRAddF32x4) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x4 with " << N << " repetitions\n";
  MeasureFunction<4, float>(&test_addf32x4);
  MeasureFunction<8, float>(&test_addf32x4);
  MeasureFunction<16, float>(&test_addf32x4);
}

TEST(SRBenchmark, SRAddF32x8) {
  constexpr size_t N = 10;
  std::cout << "Measure function sr::addf32x8 with " << N << " repetitions\n";
  MeasureFunction<8, float>(&test_addf32x8);
  MeasureFunction<16, float>(&test_addf32x8);
  MeasureFunction<32, float>(&test_addf32x8);
}

} // namespace
} // namespace sr

// HYW_TEST_MAIN();1
