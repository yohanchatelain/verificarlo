#include <iostream>

#include "hwy/highway.h"
#include "hwy/print-inl.h"

#include "src/sr_hw-inl.h"
#include "src/xoroshiro256+_hw.hpp"

HWY_BEFORE_NAMESPACE(); // at file scope
namespace sr {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

template <class D, typename T = hn::TFromD<D>> void run(const size_t N) {

  D tag{};

  T *va = new T[N];
  T *vb = new T[N];

  const size_t lanes = hn::Lanes(tag);
  std::cout << "Lanes: " << lanes << std::endl;
  std::cout << "Elements: " << N << std::endl;

  constexpr T ulp = std::is_same_v<T, float> ? 0x1.0p-24f : 0x1.0p-53;
  constexpr const char *fmt = std::is_same_v<T, float> ? "%+.6a" : "%+.13a";

  for (size_t i = 0; i < N; i++) {
    va[i] = 1;
    vb[i] = ulp;
  }

  auto a = hn::Load(tag, va);
  auto b = hn::Load(tag, vb);

  std::cout << std::hexfloat;

  for (size_t i = 0; i < lanes; i++) {
    hn::Print(tag, "a", a, i, lanes, fmt);
  }
  for (size_t i = 0; i < lanes; i++) {
    hn::Print(tag, "b", b, i, lanes, fmt);
  }

  auto c = sr_add<D>(a, b);

  for (size_t i = 0; i < lanes; i++) {
    hn::Print(tag, "c", c, i, lanes, fmt);
  }

  delete[] va;
  delete[] vb;
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
} // namespace sr
HWY_AFTER_NAMESPACE();

int main() {

  // print HWY_TARGET
  std::cout << "HWY_TARGET: " << HWY_TARGET << std::endl;

  using floatx = hwy::HWY_NAMESPACE::ScalableTag<float>;
  using doublex = hwy::HWY_NAMESPACE::ScalableTag<double>;

  for (size_t i = 0; i < 3; i++) {
    const size_t N = 2 << i;
    std::cout << "Elements: " << N << std::endl;

    std::cout << "Float" << std::endl;
    sr::HWY_NAMESPACE::run<floatx>(N);
    std::cout << std::endl;

    std::cout << "Double" << std::endl;
    sr::HWY_NAMESPACE::run<doublex>(N);
    std::cout << std::endl;

    std::cout << std::endl;
  }

  return 0;
}
