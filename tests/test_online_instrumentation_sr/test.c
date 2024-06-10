#include "interflop/common/float_const.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef REAL_TYPE
#warning "REAL is not defined, defaulting to double"
#define REAL double
#define REAL_TYPE 1
#endif

#if REAL_TYPE == 1
#define DOUBLE
#define REAL double
#elif REAL_TYPE == 2
#define FLOAT
#define REAL float
#endif

#ifndef REAL_TYPE
#error "REAL_TYPE is not defined"
#endif

// __attribute__((constructor)) void init_test() {
//   fprintf(stderr, "init test\n");
// }

// void twosum_double(double a, double b, double *tau, double *sigma);
// double sr_rounding_double(double tau, double sigma, double c);

// typedef uint64_t xoroshiro_state[2];

// static __thread xoroshiro_state rng_state;
// static pid_t global_tid = 0;
// static int already_initialized = 0;

// uint64_t next_seed(uint64_t seed_state) {
//   uint64_t z = (seed_state += UINT64_C(0x9E377B97F4A7C15));
//   z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
//   z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
//   return z ^ (z >> 31);
// }

// __attribute__((used)) static void _set_seed(const int choose_seed,
//                                             uint64_t seed) {
//   if (!choose_seed) {
//     /* TODO: to optimize seeding with rdseed asm instruction for Intel arch
//     */ struct timeval t1; gettimeofday(&t1, NULL);
//     /* Hopefully the following seed is good enough for Montercarlo */
//     seed = t1.tv_sec ^ t1.tv_usec ^ gettid();
//   }
//   rng_state[0] = next_seed(seed);
//   rng_state[1] = next_seed(seed);
// }

// #define rotl(x, k) ((x) << (k)) | ((x) >> (64 - (k)))

// uint64_t get_rand_uint64(void) {
//   const uint64_t s0 = rng_state[0];
//   uint64_t s1 = rng_state[1];
//   const uint64_t result = rotl(s0 + s1, 17) + s0;

//   s1 ^= s0;
//   rng_state[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21); // a, b
//   rng_state[1] = rotl(s1, 28);                   // c

//   return result;
// }

// __attribute__((used)) static double get_rand_double01() {
//   uint64_t x = get_rand_uint64();
//   const union {
//     uint64_t i;
//     double d;
//   } u = {.i = UINT64_C(0x3FF) << 52 | x >> 12};
//   return u.d - 1.0;
// }

// int32_t get_rand_float(float a) {
//   const uint64_t half_max_int = (uint64_t)(UINT64_MAX / 2);
//   if (a == 0)
//     return 0;
//   return (get_rand_uint64() < half_max_int) ? 1 : -1;
// }

// int64_t get_rand_double(double a) {
//   const uint64_t half_max_int = (uint64_t)(UINT64_MAX / 2);
//   if (a == 0)
//     return 0;
//   return (get_rand_uint64() < half_max_int) ? 1 : -1;
// }

double add_double(double a, double b) { return a + b; }

double sub_double(double a, double b) { return a - b; }

double mul_double(double a, double b) { return a * b; }

double div_double(double a, double b) { return a / b; }

double sqrt_double(double a) { return __builtin_sqrt(a); }

double fma_double(double a, double b, double c) {
  return __builtin_fma(a, b, c);
}

double operator_double(char op, double a, double b, double c) {
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
  case 'f':
    return fma_double(a, b, c);
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

float fma_float(float a, float b, float c) { return __builtin_fmaf(a, b, c); }

float operator_float(char op, float a, float b, float c) {
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
  case 'f':
    return fma_float(a, b, c);
  default:
    fprintf(stderr, "Error: unknown operation\n");
    abort();
  }
}

// void run_test() {
//   double a = 0.1, b = 0.01, c = 0.0, d = 0.0, tau = 0.0, sigma = 0.0;
//   fprintf(stderr, "\n");
//   for (int i = 0; i < 10; i++) {
//     twosum_double(a, b, &tau, &sigma);
//     fprintf(stderr, "a = %.13a, b = %.13a, tau = %.13a, sigma = %.13a\n", a,
//     b,
//             tau, sigma);
//     d = sr_rounding_double(tau, sigma, i / 10.0);
//     fprintf(stderr, "round = %.13a\n", c);
//   }
// }

int main(int argc, const char *argv[]) {

  if (argc == 3) {
    if (argv[1][0] != 's') {
      fprintf(stderr, "usage: ./test <op> a\n");
      exit(1);
    }
  } else if (argc == 5) {
    if (argv[1][0] != 'f') {
      fprintf(stderr, "usage: ./test <op> a b c\n");
      exit(1);
    }

  } else if (argc != 4) {
    fprintf(stderr, "usage: ./test <op> a b\n");
    exit(1);
  }
  // run_test();

  const char op = argv[1][0];
  REAL a = atof(argv[2]), b = 0, c = 0;

  if (op != 's')
    b = atof(argv[3]);

  if (op == 'f')
    c = atof(argv[4]);

#ifdef DOUBLE
  // fprintf(stderr, "[double] op = %c, a = %.13a, b = %.13a\n", op, a, b);
  printf("%.13a\n", operator_double(op, a, b, c));
#elif defined(FLOAT)
  // fprintf(stderr, "[float] op = %c, a = %.6a, b = %.6a\n", op, a, b);
  printf("%.6a\n", operator_float(op, a, b, c));
#endif

  return EXIT_SUCCESS;
}
