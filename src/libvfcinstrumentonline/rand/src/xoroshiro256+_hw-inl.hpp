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

// TODO: make seeds different for scalar and vector versions

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
namespace dbg = prism::vector::HWY_NAMESPACE;
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
    const auto thread_id = (syscall(__NR_gettid) % syscall(__NR_getpid));
    return new hn::VectorXoshiro(seed, thread_id + 1);
  }
};

extern thread_local RNGInitializer rng;

template <class D, class V = hn::VFromD<D>> V uniform(const D d) {
  using T = hn::TFromD<D>;
  auto z = rng.get()->Uniform(T{});
  using D_RNG = hn::DFromV<decltype(z)>;
  const D_RNG d_rng;
  const auto N = hn::Lanes(d_rng);
  std::array<T, N> z_array;
  hn::Store(z, d_rng, z_array.data());
  return hn::Load(d, z_array.data());
}

template <class D, class V = hn::VFromD<D>> V random(const D d) {
  using T = hn::TFromD<D>;
  auto z = rng.get()->operator()(T{});
  using D_RNG = hn::DFromV<decltype(z)>;
  const D_RNG d_rng;
  const auto N = hn::Lanes(d_rng);
  std::array<T, N> z_array;
  hn::Store(z, d_rng, z_array.data());
  return hn::Load(d, z_array.data());
}

} // namespace HWY_NAMESPACE

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace xoroshiro256plus
} // namespace vector
} // namespace prism
HWY_AFTER_NAMESPACE();

#endif // HIGHWAY_HWY_SRLIB_RAND_XOROSHIRO256P_H_