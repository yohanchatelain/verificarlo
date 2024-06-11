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
