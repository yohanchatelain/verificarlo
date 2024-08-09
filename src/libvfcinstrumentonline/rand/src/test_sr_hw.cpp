#include <iostream>

#include "hwy/highway.h"
#include "hwy/print-inl.h"

#include "src/sr_hw.hpp"
#include "src/xoroshiro256+_hw.hpp"

HWY_BEFORE_NAMESPACE(); // at file scope
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

void run_float() {

  using floatx = hn::ScalableTag<float>;
  floatx tag;

  float va[] = {0.1f, 0.1f, 0.1f, 0.1f};
  float vb[] = {0.01f, 0.01f, 0.01f, 0.01f};
  auto a = hn::Load(tag, va);
  auto b = hn::Load(tag, vb);

  std::cout << std::hexfloat;
  hn::Print(tag, "a", a, 0, 7, "%+.6a");
  hn::Print(tag, "b", b, 0, 7, "%+.6a");

  auto c = sr_add<float>(a, b);

  hn::Print(tag, "c", c, 0, 7, "%+.6a");
}

void run_double() {

  using doublex = hn::ScalableTag<double>;
  doublex tag;

  double va[] = {0.1, 0.1, 0.1, 0.1};
  double vb[] = {0.01, 0.01, 0.01, 0.01};
  auto a = hn::Load(tag, va);
  auto b = hn::Load(tag, vb);

  std::cout << std::hexfloat;
  hn::Print(tag, "a", a, 0, 7, "%+.13a");
  hn::Print(tag, "b", b, 0, 7, "%+.13a");

  auto c = sr_add<double>(a, b);

  hn::Print(tag, "c", c, 0, 7, "%+.13a");
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

int main() {
  N_SSSE3::run_float();
  N_SSSE3::run_double();
  return 0;
}
