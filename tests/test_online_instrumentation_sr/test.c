#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "interflop/common/float_const.h"

#ifndef REAL
#warning "REAL is not defined, defaulting to double"
#define REAL double
#endif

typedef union {
  double f64;
  int64_t s64;
} binary64;

typedef union {
  float f32;
  int32_t s32;
} binary32;

#define OPERATOR(a, b)                                                         \
  switch (op) {                                                                \
  case '+':                                                                    \
    return a + b;                                                              \
  case '-':                                                                    \
    return a - b;                                                              \
  case 'x':                                                                    \
    return a * b;                                                              \
  case '/':                                                                    \
    return a / b;                                                              \
  default:                                                                     \
    fprintf(stderr, "Error: unknown operation\n");                             \
    abort();                                                                   \
  }

__attribute__((noinline)) REAL operator(char op, REAL a, REAL b) {
  OPERATOR(a, b)
}

int main(int argc, const char *argv[]) {

  if (argc != 4) {
    fprintf(stderr, "usage: ./test <op> a b\n");
    exit(1);
  }

  const char op = argv[1][0];
  REAL a = atof(argv[2]);
  REAL b = atof(argv[3]);

  printf("%.13a\n", operator(op, a, b));

  return EXIT_SUCCESS;
}
