#include <array>
#include <immintrin.h>
#include <random>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#if defined(HIGHWAY_HWY_SRLIB_RAND_XOROSHIRO256P_H_) ==                        \
    defined(HWY_TARGET_TOGGLE) // NOLINT
#ifdef HIGHWAY_HWY_SRLIB_RAND_XOROSHIRO256P_H_
#undef HIGHWAY_HWY_SRLIB_RAND_XOROSHIRO256P_H_
#else
#define HIGHWAY_HWY_SRLIB_RAND_XOROSHIRO256P_H_
#endif

#include "hwy/highway.h"

#include "src/random-inl.h"
#include "src/target-utils.h"
#include "src/xoroshiro256+_hw.h"

HWY_BEFORE_NAMESPACE(); // at file scope
namespace prism {
namespace scalar {
namespace xoroshiro256plus {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;
namespace pn = prism::HWY_NAMESPACE;

class RNGInitializer {
public:
  RNGInitializer();
  hn::CachedXoshiro<> *get();

private:
  hn::CachedXoshiro<> *rng = nullptr;
  static hn::CachedXoshiro<> *rng_constructor() {
    if (not pn::isCurrentTargetSupported())
      return nullptr;

    const auto seed = get_user_seed();
    const auto thread_id = syscall(__NR_gettid) % syscall(__NR_getpid);
    return new hn::CachedXoshiro<>(seed, thread_id);
  }
};

extern thread_local RNGInitializer rng;

HWY_API float uniformf32() { return rng.get()->Uniform(); }
HWY_API double uniformf64() { return rng.get()->Uniform(); }
HWY_API std::uint64_t randomu64() { return rng.get()->operator()(); }

} // namespace HWY_NAMESPACE
} // namespace xoroshiro256plus
} // namespace scalar

namespace vector {

namespace xoroshiro256plus {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;
namespace pvn = prism::vector::HWY_NAMESPACE;
namespace pn = prism::HWY_NAMESPACE;

class RNGInitializer {
public:
  RNGInitializer();
  hn::VectorXoshiro *get();

private:
  hn::VectorXoshiro *rng = nullptr;
  static hn::VectorXoshiro *rng_constructor() {
    if (not pn::isCurrentTargetSupported())
      return nullptr;

    const auto seed = get_user_seed();
    const auto thread_id = syscall(__NR_gettid) % syscall(__NR_getpid);
    return new hn::VectorXoshiro(seed, thread_id);
  }
};

extern thread_local RNGInitializer rng;

template <class D, class V = hn::VFromD<D>> V uniform(const D d) {
  pvn::debug_msg("[uniform] START");
  using T = hn::TFromD<D>;
  const std::size_t lanes = hn::Lanes(d);
  if constexpr ((lanes * sizeof(T)) <= 16) {
    /* allocate at least 128bits */
    /* since VectorXoshiro returns 128bits elements */
    const std::size_t size_rng = std::max(lanes, std::size_t{8});
    auto z = rng.get()->Uniform(T{}, size_rng);
    auto z_load = hn::Load(d, z.data());
    pvn::debug_msg("[uniform] END");
    return z_load;
  } else {
    const std::size_t size_rng = lanes;
    auto z = rng.get()->Uniform(T{}, size_rng);
    auto z_load = hn::Load(d, z.data());
    pvn::debug_msg("[uniform] END");
    return z_load;
  }
}

template <class D, class V = hn::VFromD<D>> V random(const D d) {
  pvn::debug_msg("[random] START");
  using T = hn::TFromD<D>;
  const std::size_t lanes = hn::Lanes(d);
  if constexpr ((lanes * sizeof(T)) <= 16) {
    /* allocate at least 128bits */
    /* since VectorXoshiro returns 128bits elements */
    const std::size_t size_rng = std::max(lanes, std::size_t{8});
    auto z = rng.get()->operator()(T{}, size_rng);
    auto z_load = hn::Load(d, z.data());
    pvn::debug_msg("[random] END");
    return z_load;
  } else {
    const std::size_t size_rng = lanes;
    auto z = rng.get()->operator()(T{}, size_rng);
    auto z_load = hn::Load(d, z.data());
    pvn::debug_msg("[random] END");
    return z_load;
  }
}

} // namespace HWY_NAMESPACE

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace xoroshiro256plus
} // namespace vector
} // namespace prism
HWY_AFTER_NAMESPACE();

#endif // HIGHWAY_HWY_SRLIB_RAND_XOROSHIRO256P_H_