
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
namespace sr {
namespace scalar {
namespace xoroshiro256plus {

namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;
RNGInitializer::RNGInitializer() : rng(RNGInitializer::rng_constructor()) {}

hn::CachedXoshiro<> *RNGInitializer::get() {
#ifdef SR_DEBUG
  // check if the pointer is not null
  if (this->rng.get() == nullptr) {
    std::cerr << "Error srlib: ";
    std::cerr << "RNGInitializer::get() returned a null pointer\n";
    std::cerr << "Means that the current target is not supported\n";
    std::abort();
  }
#endif
  return this->rng.get();
}

RNGInitializer rng;

} // namespace HWY_NAMESPACE

} // namespace xoroshiro256plus
} // namespace scalar

namespace vector {
namespace xoroshiro256plus {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

RNGInitializer::RNGInitializer() : rng(RNGInitializer::rng_constructor()) {}

hn::VectorXoshiro *RNGInitializer::get() {
#ifdef SR_DEBUG
  // check if the pointer is not null
  if (this->rng.get() == nullptr) {
    std::cerr << "Error srlib: ";
    std::cerr << "RNGInitializer::get() returned a null pointer\n";
    std::cerr << "Means that the current target is not supported\n";
    std::abort();
  }
#endif
  return this->rng.get();
}

RNGInitializer rng;

} // namespace HWY_NAMESPACE
} // namespace xoroshiro256plus
} // namespace vector

} // namespace sr
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace sr {
namespace scalar {
namespace xoroshiro256plus {

HWY_EXPORT(uniformf32);
HWY_EXPORT(uniformf64);

namespace static_dispatch {
float uniform(float) { return HWY_STATIC_DISPATCH(uniformf32)(); }
double uniform(double) { return HWY_STATIC_DISPATCH(uniformf64)(); }

} // namespace static_dispatch

namespace dynamic_dispatch {

HWY_DLLEXPORT float uniform(float) {
  return HWY_DYNAMIC_DISPATCH(uniformf32)();
}
HWY_DLLEXPORT double uniform(double) {
  return HWY_DYNAMIC_DISPATCH(uniformf64)();
}

} // namespace dynamic_dispatch

} // namespace xoroshiro256plus
} // namespace scalar

} // namespace sr

#endif // HWY_ONCE