#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "interflop/common/float_const.h"

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

typedef union {
  double f64;
  int64_t s64;
} binary64;

typedef union {
  float f32;
  int32_t s32;
} binary32;

double fma_double(double a, double b, double c) {
  return __builtin_fma(a, b, c);
}

float fma_float(float a, float b, float c) { return __builtin_fmaf(a, b, c); }

#ifdef FLOAT
#define fma fma_float
#elif defined(DOUBLE)
#define fma fma_double
#endif

#define OPERATOR(a, b, c)                                                      \
  switch (op) {                                                                \
  case '+':                                                                    \
    return a + b;                                                              \
  case '-':                                                                    \
    return a - b;                                                              \
  case 'x':                                                                    \
    return a * b;                                                              \
  case '/':                                                                    \
    return a / b;                                                              \
  case 'f':                                                                    \
    return fma(a, b, c);                                                       \
  default:                                                                     \
    fprintf(stderr, "Error: unknown operation\n");                             \
    abort();                                                                   \
  }

__attribute__((noinline)) REAL operator(char op, REAL a, REAL b,
                                        REAL c){OPERATOR(a, b, c)}

int64_t CALL_SHISHUA(int32_t) {
  return 0;
}

void print_shishua_info(uint32_t shishua_buffer_index, uint8_t *buf) {
  printf("Shishua buffer: \n");
  printf("Buffer index: %d\n", shishua_buffer_index);
  for (int i = 0; i < 256; i++) {
    if (i != 0 && i % 32 == 0)
      printf("\n");
    printf("%02x ", buf[i]);
  }
  printf("\n");
}

void shishua_test() {
  for (int i = 0; i < 512; i++) {
    print_shishua_info(0, NULL);
    if (i != 0 && i % 32 == 0)
      printf("\n");
    printf("SHISHUA: %ld\n", CALL_SHISHUA(0));
  }
  printf("\n");
}

int main(int argc, const char *argv[]) {

  if (argc == 5) {
    if (argv[1][0] != 'f') {
      fprintf(stderr, "usage: ./test <op> a b c\n");
      exit(1);
    }
  } else if (argc != 4) {
    fprintf(stderr, "usage: ./test <op> a b\n");
    exit(1);
  }

  const char op = argv[1][0];
  REAL a = atof(argv[2]);
  REAL b = atof(argv[3]);
  REAL c = 0;
  if (op == 'f') {
    c = atof(argv[4]);
  }

  printf("%.13a\n", operator(op, a, b, c));

  shishua_test();

  return EXIT_SUCCESS;
}
