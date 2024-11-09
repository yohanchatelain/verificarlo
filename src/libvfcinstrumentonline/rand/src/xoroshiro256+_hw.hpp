#include <array>
#include <immintrin.h>
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

HWY_BEFORE_NAMESPACE(); // at file scope
namespace sr {
namespace HWY_NAMESPACE {
namespace xoroshiro256plus {

namespace hn = hwy::HWY_NAMESPACE;

uint64_t get_user_seed() throw() {
  debug_msg("[get_user_seed] START");
  // try to get VFC_RNG_SEED from the environment
  const char *env_seed = getenv("VFC_RNG_SEED");
  if (env_seed) {
    try {
      return std::stoll(env_seed, nullptr, 10);
    } catch (const std::exception &e) {
      std::cerr << "Error parsing VFC_RNG_SEED: ";
      std::cerr << e.what() << '\n';
    }
  } else {
    uint64_t seed = 1;
    struct timeval t1;
    gettimeofday(&t1, nullptr);
    seed = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);
    debug_msg("[get_user_seed] STOP");
    return seed;
  }
  return 0;
}

// TODO: add threadNumber to the constructor
static auto rng = hn::VectorXoshiro(get_user_seed());

template <class D, class V = hn::VFromD<D>> V uniform(const D d) {
  debug_msg("[uniform] START");
  using T = hn::TFromD<D>;
  // rng.Uniform returns at most 128 bits vector
  // so we need to call it multiple times to fill the vector D
  auto z = rng.Uniform(T{}, hn::Lanes(d));
  auto z_load = hn::Load(d, z.data());
  debug_msg("[uniform] END");
  return z_load;
}

} // namespace xoroshiro256plus

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
} // namespace sr
HWY_AFTER_NAMESPACE();

#endif // HIGHWAY_HWY_SRLIB_RAND_XOROSHIRO256P_H_