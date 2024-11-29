
#include "src/xoroshiro256+_hw.h"

// First undef to prevent error when re-included.
#undef HWY_TARGET_INCLUDE
// For dynamic dispatch, specify the name of the current file (unfortunately
// __FILE__ is not reliable) so that foreach_target.h can re-include it.
#define HWY_TARGET_INCLUDE "src/xoroshiro256+_hw.cpp"
// Generates code for each enabled target by re-including this source file.
#include "hwy/foreach_target.h"

#include "hwy/base.h"
#include "hwy/highway.h"

#include "src/xoroshiro256+_hw-inl.hpp"

HWY_BEFORE_NAMESPACE(); // at file scope
namespace prism {
namespace scalar {
namespace xoroshiro256plus {

namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;
RNGInitializer::RNGInitializer() : rng(RNGInitializer::rng_constructor()) {}

hn::CachedXoshiro<> *RNGInitializer::get() {
  // check if the pointer is not null
  // means that the current target has not been initialized
  // happens with thread_local variables (lazy init?)
  if (this->rng == nullptr) {
    this->rng = RNGInitializer::rng_constructor();
  }
  return this->rng;
}

thread_local RNGInitializer rng;

} // namespace HWY_NAMESPACE

} // namespace xoroshiro256plus
} // namespace scalar

namespace vector {
namespace xoroshiro256plus {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

RNGInitializer::RNGInitializer() : rng(RNGInitializer::rng_constructor()) {}

hn::VectorXoshiro *RNGInitializer::get() {
  // check if the pointer is not null
  // means that the current target has not been initialized
  // happens with thread_local variables (lazy init?)
  if (this->rng == nullptr) {
    this->rng = RNGInitializer::rng_constructor();
  }
  return this->rng;
}

thread_local RNGInitializer rng;

} // namespace HWY_NAMESPACE
} // namespace xoroshiro256plus
} // namespace vector

} // namespace prism
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace prism {
namespace scalar {
namespace xoroshiro256plus {

HWY_EXPORT(uniformf32);
HWY_EXPORT(uniformf64);
HWY_EXPORT(randomu64);

namespace static_dispatch {
float uniform(float) { return HWY_STATIC_DISPATCH(uniformf32)(); }
double uniform(double) { return HWY_STATIC_DISPATCH(uniformf64)(); }
std::uint64_t random() { return HWY_STATIC_DISPATCH(randomu64)(); }

} // namespace static_dispatch

namespace dynamic_dispatch {

HWY_DLLEXPORT float uniform(float) {
  return HWY_DYNAMIC_DISPATCH(uniformf32)();
}
HWY_DLLEXPORT double uniform(double) {
  return HWY_DYNAMIC_DISPATCH(uniformf64)();
}

HWY_DLLEXPORT std::uint64_t random() {
  return HWY_DYNAMIC_DISPATCH(randomu64)();
}

} // namespace dynamic_dispatch

} // namespace xoroshiro256plus
} // namespace scalar

} // namespace prism

#endif // HWY_ONCE