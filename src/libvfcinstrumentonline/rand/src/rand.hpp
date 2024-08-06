#ifndef __VERIFICARLO_SRLIB_RAND_HPP__
#define __VERIFICARLO_SRLIB_RAND_HPP__

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <vector>

#include "debug.hpp"
#include "utils.hpp"
#include "vector_types.hpp"

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
#include "xoroshiro256+.hpp"
using rng_t = xoroshiro256plus::scalar::rn_uint64;
typedef xoroshiro256plus::scalar::state rng_state_t;
static __thread rng_state_t rng_state;
using rn_int64_x2 = sse4_2::int64x2_t;
using rn_int32_x4 = sse4_2::int32x4_t;
using float2 = scalar::floatx2_t;
using float4 = sse4_2::floatx4_t;
using double2 = sse4_2::doublex2_t;
typedef xoroshiro256plus::sse4_2::state rng_state_x2_t;
static __thread rng_state_x2_t rng_state_x2;
#elif defined(SHISHUA)
#include "shishua.h"
typedef prng_state rng_state_t;
static __thread rng_state_t rng_state;
#elif defined(USE_CXX11_RANDOM)
// nothing to do
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
  rng = xoroshiro256plus::scalar::next(rng_state);
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

rn_int64_x2 get_rand_uint64_x2() {
  rn_int64_x2 rng;
#ifdef XOROSHIRO
  rng = xoroshiro256plus::sse4_2::next(rng_state_x2);
#else
#error "No PRNG defined"
#endif
  return rng;
}

uint8_t get_rand_uint1() { return _get_rand_uint64() & 0x1; }

#ifdef XOROSHIRO
template <typename I, typename T> I get_rand_uint() {
  return static_cast<T>(_get_rand_uint64());
}
template <> scalar::uint32x2_t get_rand_uint<scalar::uint32x2_t, float>() {
  return scalar::uint32x2_t{.u64 = _get_rand_uint64()};
}
template <> rn_int64_x2 get_rand_uint<rn_int64_x2, double>() {
  return get_rand_uint64_x2();
}
scalar::uint32x2_t get_rand_uint32_x2() {
  return get_rand_uint<scalar::uint32x2_t, float>();
}
float2 get_rand_float01_x2() {
  float2 x = {.u64 = _get_rand_uint64() >> 9};
  x.f[0] *= 0x1.0p-24f;
  x.f[1] *= 0x1.0p-24f;
  return x;
}
double2 get_rand_double01_x2() {
  rn_int64_x2 x = get_rand_uint64_x2();
  double2 ulp = {0x1.0p-53, 0x1.0p-53};
  return (x >> 11) * ulp;
}
#elif defined(SHISHUA)
template <typename T> T get_rand_uint() {
  return static_cast<T>(prng_uint64(&rng_state));
}
#elif defined(USE_CXX11_RANDOM)
template <typename T> T get_rand_uint() {
  return static_cast<T>(_get_rand_uint64());
}
#endif

uint32_t get_rand_uint32_t() { return get_rand_uint<uint32_t, float>(); }
uint64_t get_rand_uint64_t() { return get_rand_uint<uint64_t, double>(); }

float get_rand_float01() {
  uint32_t x = get_rand_uint32_t();
  return (x >> 9) * 0x1.0p-24f;
}

double get_rand_double01() {
  uint64_t x = get_rand_uint64_t();
  return (x >> 11) * 0x1.0p-53;
}

template <typename T> T get_rand01() {
  using ieee = typename sr::utils::IEEE754<T>;
  using U = typename ieee::U;
  constexpr U exponent_size = ieee::exponent_size;
  constexpr U ulp = ieee::ulp;
  U u = get_rand_uint<U>();
  return u >> (exponent_size + 1) * ulp;
}

uint64_t splitmix64(uint64_t &seed) {
  uint64_t z = (seed += 0x9e3779b97f4a7c15);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
  z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
  return z ^ (z >> 31);
}

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

template <int N> std::array<uint64_t, N> get_seeds() {
  std::array<uint64_t, N> seeds;
  uint64_t seed = get_user_seed();
  for (int i = 0; i < N; i++) {
    seed = splitmix64(seed);
    seeds[i] = seed;
  }
  return seeds;
}

#endif // __VERIFICARLO_SRLIB_RAND_HPP__