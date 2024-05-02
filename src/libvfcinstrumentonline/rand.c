#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#define __INTERNAL_RNG_STATE xoroshiro_state

typedef uint64_t xoroshiro_state[2];

static __thread xoroshiro_state rng_state;
static pid_t global_tid = 0;
static bool already_initialized = 0;

uint64_t next_seed(uint64_t seed_state) {
  uint64_t z = (seed_state += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

#define rotl(x, k) ((x) << (k)) | ((x) >> (64 - (k)))

uint64_t get_rand_uint64(void) {
  const uint64_t s0 = rng_state[0];
  uint64_t s1 = rng_state[1];
  const uint64_t result = rotl(s0 + s1, 17) + s0;

  s1 ^= s0;
  rng_state[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21); // a, b
  rng_state[1] = rotl(s1, 28);                   // c

  return result;
}

int32_t get_rand_float(float a) {
  const uint64_t half_max_int = (uint64_t)(UINT64_MAX / 2);
  if (a == 0)
    return 0;
  return (get_rand_uint64() < half_max_int) ? 1 : -1;
}

int64_t get_rand_double(double a) {
  const uint64_t half_max_int = (uint64_t)(UINT64_MAX / 2);
  if (a == 0)
    return 0;
  return (get_rand_uint64() < half_max_int) ? 1 : -1;
}

__attribute__((constructor)) static void init(void) {
  if (already_initialized == 0) {
    already_initialized = 1;
  } else {
    return;
  }
  uint64_t seed = 0;
  struct timeval t1;
  gettimeofday(&t1, NULL);
  seed = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);
  rng_state[0] = next_seed(seed);
  rng_state[1] = next_seed(seed);
}