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

// Include other necessary headers (float_const.h, float_struct.h, shishua.h,
// vector_types.h)

// force fma hardware instruction
// fail if not supported

#ifdef DEBUG
#include <cstdio>
#define debug_print(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__);
#else
#define debug_print(fmt, ...)
#endif

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
#else
#ifdef XOROSHIRO
#include "xoroshiro128+.cpp"
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

uint64_t next_seed(uint64_t seed_state) {
  uint64_t z = (seed_state += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

#define rotl(x, k) ((x << k) | (x >> (64 - k)))

uint64_t _get_rand_uint64() {
#ifdef XOROSHIRO
#ifdef USE_BOOST_TLS
  auto state = rng_state.get();
  if (!state) {
    state = new xoroshiro_state();
    rng_state.reset(state);
  }
#else
#endif
  __m256i result = xoroshiro128plus_avx2_next(&rng_state);
  return _mm256_extract_epi32(result, 0);
#elif defined(SHISHUA)
#ifdef USE_BOOST_TLS
  return prng_uint64(rng_state.get());
#else
  return prng_uint64(&rng_state);
#endif
#else
#error "No PRNG defined"
#endif
}

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
  const union {
    uint64_t i;
    double d;
  } u = {.i = UINT64_C(0x3FF) << 52 | x >> 12};
  return u.d - 1.0;
}

void init() {
  if (already_initialized)
    return;
  already_initialized = true;

  uint64_t seed = 1;
  struct timeval t1;
  gettimeofday(&t1, nullptr);
  seed = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);

#ifdef XOROSHIRO
  xoroshiro128plus_avx2_init(&rng_state, seed);
#elif defined(SHISHUA)
  uint64_t seed_state[4] = {next_seed(seed), next_seed(seed), next_seed(seed),
                            next_seed(seed)};
  prng_init(&rng_state, seed_state);
#else
#error "No PRNG defined"
#endif
}

// Implement other functions (get_exponent, predecessor, abs, pow2, etc.) using
// templates
template <typename T> T predecessor(T a) {
  if (a == 0)
    return -std::numeric_limits<T>::denorm_min();
  // return previous floating-point number
  if constexpr (std::is_same<T, float>::value) {
    uint32_t i = *reinterpret_cast<uint32_t *>(&a);
    i -= 1;
    return *reinterpret_cast<float *>(&i);
  } else {
    uint64_t i = *reinterpret_cast<uint64_t *>(&a);
    i -= 1;
    return *reinterpret_cast<double *>(&i);
  }
}

template <typename T> uint32_t get_exponent(T a) {
  // return exponent of a
  if constexpr (std::is_same<T, float>::value) {
    return (reinterpret_cast<uint32_t &>(a) >> 23) & 0xFF;
  } else {
    return (reinterpret_cast<uint64_t &>(a) >> 52) & 0x7FF;
  }
}

template <typename T, typename I> T pow2(I n) {
  if constexpr (std::is_same<T, float>::value) {
    float x = 1.0f;
    uint32_t i = *reinterpret_cast<uint32_t *>(&x);
    i += n << 23;
    x = *reinterpret_cast<float *>(&i);
    return x;
  } else {
    double x = 1.0;
    uint64_t i = *reinterpret_cast<uint64_t *>(&x);
    i += static_cast<uint64_t>(n) << 52;
    x = *reinterpret_cast<double *>(&i);
    return x;
  }
}

template <typename T> T sr_round(T sigma, T tau, T z) {
  T eps = std::is_same<T, float>::value ? 0x1.0p-23f : 0x1.0p-52;
  bool sign_tau = tau < 0;
  bool sign_sigma = sigma < 0;
  uint32_t eta;
  if (sign_tau != sign_sigma) {
    eta = get_exponent(predecessor(std::abs(sigma)));
  } else {
    eta = get_exponent(sigma);
  }
  T ulp = (sign_tau ? 1 : -1) * pow2<T>(eta) * eps;
  T pi = ulp * z;
  T round;
  if (std::abs(tau + pi) >= std::abs(ulp)) {
    round = ulp;
  } else {
    round = 0;
  }
  return round;
}

template <typename T> T ud_round(T a) {
  if (a == 0)
    return a;
  using IntType =
      typename std::conditional<sizeof(T) == 4, uint32_t, uint64_t>::type;
  IntType a_bits;
  std::memcpy(&a_bits, &a, sizeof(T));
  uint64_t rand = _get_rand_uint64();
  a_bits += (rand & 0x01) ? 1 : -1;
  std::memcpy(&a, &a_bits, sizeof(T));
  return a;
}

template <typename T> T ud_add(T a, T b) { return ud_round(a + b); }
template <typename T> T ud_sub(T a, T b) { return ud_add(a, -b); }
template <typename T> T ud_mul(T a, T b) { return ud_round(a * b); }
template <typename T> T ud_div(T a, T b) { return ud_round(a / b); }

// Implement vector versions of ud_round (ud_round_b32_2x, ud_round_b32_4x,
// etc.)

template <typename T> void twosum(T a, T b, T *tau, T *sigma) {
  *sigma = a + b;
  T z = *sigma - a;
  *tau = (a - (*sigma - z)) + (b - z);
}

template <typename T>
__attribute__((target("fma"))) void twoprodfma(T a, T b, T *tau, T *sigma) {
  *sigma = a * b;
  *tau = std::fma(a, b, -(*sigma));
}

template <typename T> T sr_add(T a, T b) {
  T z = get_rand_double01();
  T tau, sigma, round;
  twosum(a, b, &tau, &sigma);
  round = sr_round(sigma, tau, z);
  return sigma + round;
}

template <typename T> T sr_sub(T a, T b) { return sr_add(a, -b); }

template <typename T> T sr_mul(T a, T b) {
  T z = get_rand_double01();
  T tau, sigma, round;
  twoprodfma(a, b, &tau, &sigma);
  round = sr_round(sigma, tau, z);
  return sigma + round;
}

template <typename T> T add_round_odd(T a, T b) {
  // return addition with rounding to odd
  // https://www.lri.fr/~melquion/doc/08-tc.pdf
  T x, e;
  twosum(a, b, &x, &e);
  return (e == 0 || *reinterpret_cast<T *>(&x) & 1) ? x : x + 1;
}

template <typename T> T __attribute__((target("fma"))) sr_div(T a, T b) {
  T z = get_rand_double01();
  T sigma = a / b;
  T tau = std::fma(-sigma, b, a) / b;
  T round = sr_round(sigma, tau, z);
  return sigma + round;
}

template <typename T> T __attribute__((target("fma,sse2"))) sr_sqrt(T a) {
  T z = get_rand_double01();
  T sigma;
#if defined(__SSE2__)
  if constexpr (std::is_same<T, float>::value) {
    // call assembly sqrtss
    asm("sqrtss %1, %0"
        : "=x"(sigma) // Output operand
        : "x"(a)      // Input operand
    );
  } else {
    // call assembly sqrtsd
    asm("sqrtsd %1, %0"
        : "=x"(sigma) // Output operand
        : "x"(a)      // Input operand
    );
  }
#else
  T sigma = std::sqrt(a);
#endif
  T tau = std::fma(-sigma, sigma, a) / (2 * sigma);
  T round = sr_round(sigma, tau, z);
  return sigma + round;
}

// specializations for float and double
float sr_add_float(float a, float b) { return sr_add<float>(a, b); }
float sr_sub_float(float a, float b) { return sr_sub<float>(a, b); }
float sr_mul_float(float a, float b) { return sr_mul<float>(a, b); }
float sr_div_float(float a, float b) { return sr_div<float>(a, b); }
float sr_sqrt_float(float a) { return sr_sqrt<float>(a); }

double sr_add_double(double a, double b) { return sr_add<double>(a, b); }
double sr_sub_double(double a, double b) { return sr_sub<double>(a, b); }
double sr_mul_double(double a, double b) { return sr_mul<double>(a, b); }
double sr_div_double(double a, double b) { return sr_div<double>(a, b); }
double sr_sqrt_double(double a) { return sr_sqrt<double>(a); }

float ud_add_float(float a, float b) { return ud_add<float>(a, b); }
float ud_sub_float(float a, float b) { return ud_sub<float>(a, b); }
float ud_mul_float(float a, float b) { return ud_mul<float>(a, b); }
float ud_div_float(float a, float b) { return ud_div<float>(a, b); }

double ud_add_double(double a, double b) { return ud_add<double>(a, b); }
double ud_sub_double(double a, double b) { return ud_sub<double>(a, b); }
double ud_mul_double(double a, double b) { return ud_mul<double>(a, b); }
double ud_div_double(double a, double b) { return ud_div<double>(a, b); }

// Use a global object to ensure initialization
struct Initializer {
  Initializer() { init(); }
};

Initializer initializer;