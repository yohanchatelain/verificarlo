#include <iostream>

#include "hwy/highway.h"
#include "hwy/print-inl.h"
#include "src/sr_hw.hpp"

namespace hn = hwy::HWY_NAMESPACE;

int main() {

  using floatx = hn::ScalableTag<float>;
  floatx tag;
  using uint32x = hn::ScalableTag<uint32_t>;
  uint32x tag_int;

  float va[] = {0.1f, 0.1f, 0.1f, 0.1f};
  float vb[] = {0.01f, 0.01f, 0.01f, 0.01f};
  auto a = hn::Load(tag, va);
  auto b = hn::Load(tag, vb);

  std::cout << std::hexfloat;
  hn::Print(tag, "a", a, 0, 7, "%+.6a");
  hn::Print(tag, "b", b, 0, 7, "%+.6a");

  // print type of a
  std::cout << typeid(a).name() << std::endl;

  auto c = N_SSSE3::sr_add<float>(a, b);

  auto c_int = hn::BitCast(tag_int, c);
  hn::Print(tag, "c", c, 0, 7, "%+.6a");
}