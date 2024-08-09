#ifndef __VERIFICARLO_SRLIB_RAND_XOROSHIRO256P_HPP__
#define __VERIFICARLO_SRLIB_RAND_XOROSHIRO256P_HPP__

#include <stdio.h>

#include <array>
#include <immintrin.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "hwy/highway.h"

#include "src/random-inl.h"

HWY_BEFORE_NAMESPACE(); // at file scope
namespace HWY_NAMESPACE {
namespace xoroshiro256plus {

namespace hn = hwy::HWY_NAMESPACE;

uint64_t get_user_seed() throw() {
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
    return seed;
  }
  return 0;
}

static auto rng = hn::VectorXoshiro(get_user_seed());

} // namespace xoroshiro256plus

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

#endif // __VERIFICARLO_SRLIB_RAND_XOROSHIRO256P_HPP__