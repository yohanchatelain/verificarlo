#ifndef __VERIFICARLO_SRLIB_MAIN_HPP_
#define __VERIFICARLO_SRLIB_MAIN_HPP_

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <numeric>
#include <sys/syscall.h>
#include <sys/time.h>
#include <type_traits>
#include <unistd.h>

#include "debug.hpp"
#include "rand.hpp"
#include "sr.hpp"
#include "ud.hpp"
#include "utils.hpp"

void init() {
  debug_start();
  if (already_initialized) {
    return;
    debug_end();
  }
  already_initialized = true;

  uint64_t seed = 1;
  struct timeval t1;
  gettimeofday(&t1, nullptr);
  seed = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);

#ifdef XOROSHIRO
  xoroshiro128plus_avx2_init(&rng_state, seed);
  debug_print("initialized xoroshiro128+ with seed %lu\n", seed);
#elif defined(SHISHUA)
  uint64_t seed_state[4] = {next_seed(seed), next_seed(seed), next_seed(seed),
                            next_seed(seed)};
  prng_init(&rng_state, seed_state);
#else
#error "No PRNG defined"
#endif

  debug_end();
}

// Use a global object to ensure initialization
struct Initializer {
  Initializer() { init(); }
};

Initializer initializer;

#endif // __VERIFICARLO_SRLIB_MAIN_HPP_