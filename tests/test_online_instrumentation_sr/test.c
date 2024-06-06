#include "interflop/common/float_const.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef REAL
#warning "REAL is not defined, defaulting to double"
#define REAL double
#endif

typedef uint64_t xoroshiro_state[2];

static xoroshiro_state rng_state;
static pid_t global_tid = 0;
static int already_initialized = 0;

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

static inline double get_rand_double01() {
  uint64_t x = get_rand_uint64();
  const union {
    uint64_t i;
    double d;
  } u = {.i = UINT64_C(0x3FF) << 52 | x >> 12};
  return u.d - 1.0;
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

double add_double(double a, double b) { return a + b; }

double sub_double(double a, double b) { return a - b; }

double mul_double(double a, double b) { return a * b; }

double div_double(double a, double b) { return a / b; }

double sqrt_double(double a) { return __builtin_sqrt(a); }

double operator_double(char op, double a, double b) {
  switch (op) {
  case '+':
    return add_double(a, b);
  case '-':
    return sub_double(a, b);
  case 'x':
    return mul_double(a, b);
  case '/':
    return div_double(a, b);
  case 's':
    return sqrt_double(a);
  default:
    fprintf(stderr, "Error: unknown operation\n");
    abort();
  }
}

float add_float(float a, float b) { return a + b; }

float sub_float(float a, float b) { return a - b; }

float mul_float(float a, float b) { return a * b; }

float div_float(float a, float b) { return a / b; }

float sqrt_float(float a) { return __builtin_sqrtf(a); }

float operator_float(char op, float a, float b) {
  switch (op) {
  case '+':
    return add_float(a, b);
  case '-':
    return sub_float(a, b);
  case 'x':
    return mul_float(a, b);
  case '/':
    return div_float(a, b);
  case 's':
    return sqrt_float(a);
  default:
    fprintf(stderr, "Error: unknown operation\n");
    abort();
  }
}

int main(int argc, const char *argv[]) {

  if (argc == 3) {
    if (argv[1][0] != 's') {
      fprintf(stderr, "usage: ./test <op> a\n");
      exit(1);
    }
  } else if (argc != 4) {
    fprintf(stderr, "usage: ./test <op> a b\n");
    exit(1);
  }

  const char op = argv[1][0];
  REAL a = atof(argv[2]), b = 0;

  if (op != 's')
    b = atof(argv[3]);

#if REAL_TYPE == DOUBLE
  printf("%.13a\n", operator_double(op, a, b));
#elif REAL_TYPE == FLOAT
  printf("%.6a\n", operator_float(op, a, b));
#endif

  return EXIT_SUCCESS;
}
