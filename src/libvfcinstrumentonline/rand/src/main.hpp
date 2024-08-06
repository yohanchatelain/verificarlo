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

#ifdef XOROSHIRO
  auto seeds = get_seeds<1>();
  xoroshiro256plus::scalar::init(rng_state, seeds);
  debug_print("initialized xoroshiro256+ with seed %lu\n", seeds[0]);
#elif defined(SHISHUA)
  auto seeds = get_seeds<4>();
  uint64_t seed_state[4] = {seeds[0], seeds[1], seeds[2], seeds[3]};
  prng_init(&rng_state, seed_state);
#elif defined(USE_CXX11_RANDOM)
  auto seeds = get_seeds<1>();
  gen.seed(seeds[0]);
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