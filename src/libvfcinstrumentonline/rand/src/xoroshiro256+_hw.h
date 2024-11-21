#ifndef __XOROSHIRO256PLUS_HW_H__
#define __XOROSHIRO256PLUS_HW_H__

#include <iostream>
#include <random>

#include "hwy/base.h"

namespace sr {

HWY_API uint64_t get_user_seed() throw() {
  static bool initialized = false;
  static uint64_t seed = 0;

  if (initialized) {
    return seed;
  }

  // try to get VFC_RNG_SEED from the environment
  const char *env_seed = getenv("VFC_RNG_SEED");
  if (env_seed) {
    try {
      seed = std::stoll(env_seed, nullptr, 10);
    } catch (const std::exception &e) {
      std::cerr << "Error parsing VFC_RNG_SEED: ";
      std::cerr << e.what() << '\n';
      std::cerr << "setting seed to random\n";
    }
    initialized = true;
  }

  if (not initialized) {
    std::random_device rd;
    seed = rd();
    initialized = true;
  }

  return seed;
}

namespace scalar {
namespace xoroshiro256plus {

namespace static_dispatch {
HWY_DLLEXPORT float uniform(float);
HWY_DLLEXPORT double uniform(double);
HWY_DLLEXPORT std::uint64_t random();
} // namespace static_dispatch

namespace dynamic_dispatch {
HWY_DLLEXPORT float uniform(float);
HWY_DLLEXPORT double uniform(double);
HWY_DLLEXPORT std::uint64_t random();
} // namespace dynamic_dispatch

} // namespace xoroshiro256plus
} // namespace scalar

} // namespace sr

#endif // __XOROSHIRO256PLUS_HW_H__