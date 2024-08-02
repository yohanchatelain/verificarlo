#ifndef __VERIFICARLO_SRLIB_RAND_HPP__
#define __VERIFICARLO_SRLIB_RAND_HPP__

#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <unistd.h>

#include "debug.hpp"

#ifdef USE_BOOST_TLS
#define BOOST_THREAD_USE_LIB
#include <boost/thread/tss.hpp>

#ifdef XOROSHIRO
using xoroshiro_state = std::array<uint64_t, 2>;
boost::thread_specific_ptr<xoroshiro_state> rng_state;
#elif defined(SHISHUA)
boost::thread_specific_ptr<prng_state> rng_state;
boost::thread_specific_ptr<int> shishua_buffer_index;
boost::thread_specific_ptr<std::array<uint8_t, SHISHUA_RNG_BUFFER_SIZE>> buf;
#else
#error "No PRNG defined"
#endif

#else // USE TLS directly

#ifdef XOROSHIRO
#include "xoroshiro128+.hpp"
typedef __INTERNAL_RNG_STATE rng_state_t;
static __thread rng_state_t rng_state;
#elif defined(SHISHUA)
#include "shishua.h"
typedef prng_state rng_state_t;
static __thread rng_state_t rng_state;
#else
#error "No PRNG defined"
#endif
#endif

static pid_t global_tid = 0;
static bool already_initialized = false;

#define rotl(x, k) ((x << k) | (x >> (64 - k)))

uint64_t next_seed(uint64_t seed_state) {
  uint64_t z = (seed_state += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

#define USE_CXX11_RANDOM

#ifdef USE_CXX11_RANDOM
static std::random_device rd;
static std::mt19937_64 gen(rd());
#include <random>
#endif

uint64_t _get_rand_uint64() {
  uint64_t rng = 0;
#ifdef USE_CXX11_RANDOM
  rng = gen();
#elif defined(XOROSHIRO)
#ifdef USE_BOOST_TLS
  auto state = rng_state.get();
  if (!state) {
    state = new xoroshiro_state();
    rng_state.reset(state);
  }
#endif
  __m256i result = xoroshiro128plus_avx2_next(&rng_state);
  rng = _mm256_extract_epi32(result, 0);
#elif defined(SHISHUA)
#ifdef USE_BOOST_TLS
  rng = prng_uint64(rng_state.get());
#else
  rng = prng_uint64(&rng_state);
#endif
#else
#error "No PRNG defined"
#endif
  return rng;
}

uint8_t get_rand_uint1() { return _get_rand_uint64() & 0x1; }

#ifdef XOROSHIRO
template <typename T> T get_rand_uint() {
  return static_cast<T>(_get_rand_uint64());
}
#elif defined(SHISHUA)
template <typename T> T get_rand_uint() {
  return static_cast<T>(prng_uint64(&rng_state));
}
#endif

uint32_t get_rand_uint32_t() { return get_rand_uint<uint32_t>(); }
uint64_t get_rand_uint64_t() { return get_rand_uint<uint64_t>(); }

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, int64_t>::type
get_rand_float(T a) {
  const uint64_t half_max_int = UINT64_MAX / 2;
  if (a == 0)
    return 0;
  return (_get_rand_uint64() < half_max_int) ? 1 : -1;
}

// TODO: check if this is correct
float get_rand_float01() {
  uint32_t x = get_rand_uint32_t();
  const union {
    uint32_t i;
    float f;
  } u = {.i = UINT32_C(0x3F800000) | (x >> 9)};
  return u.f - 1.0f;
}

double get_rand_double01() {
  uint64_t x = get_rand_uint64_t();
  return std::generate_canonical<double, 64>(gen);
  const union {
    uint64_t i;
    double d;
  } u = {.i = UINT64_C(0x3FF) << 52 | x >> 12};
  debug_print("get_rand_double01() = %.13a\n", u.d - 1.0);
  return u.d - 1.0;
}

#endif // __VERIFICARLO_SRLIB_RAND_HPP__