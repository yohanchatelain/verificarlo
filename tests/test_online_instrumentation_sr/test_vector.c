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

#ifndef SIZE
#warning "size is not defined, defaulting to 2"
#define SIZE 2
#endif

typedef float float2 __attribute__((vector_size(8)));
typedef float float4 __attribute__((vector_size(16)));
typedef float float8 __attribute__((vector_size(32)));
typedef float float16 __attribute__((vector_size(64)));
typedef double double2 __attribute__((vector_size(16)));
typedef double double4 __attribute__((vector_size(32)));
typedef double double8 __attribute__((vector_size(64)));
typedef double double16 __attribute__((vector_size(128)));
typedef int int2 __attribute__((vector_size(8)));
typedef int int4 __attribute__((vector_size(16)));
typedef int int8 __attribute__((vector_size(32)));
typedef int int16 __attribute__((vector_size(64)));

#define define_binary_vector(type, operation, operator, size)                  \
  type##size operation##_##size##x##_##type(type##size a, type##size b) {      \
    return a operator b;                                                       \
  }

#define define_unary_vector(type, operation, operator, size)                   \
  type##size operation##_##size##x##_##type(type##size a) { return operator a; }

#define define_comparison_vector(type, operation, operator, size)              \
  int##size operation##_##size##x##_##type##_cmp(type##size a, type##size b) { \
    return a operator b;                                                       \
  }

#define define_fmaf_vector(size)                                               \
  float##size operation##_##size##x##_##float(float##size a, float##size b,    \
                                              float##size c) {                 \
    float##size res;                                                           \
    for (int i = 0; i < size; i++) {                                           \
      res[i] = __builtin_fmaf(a[i], b[i], c[i]);                               \
    }                                                                          \
    return res;                                                                \
  }

#define define_fma_vector(size)                                                \
  double##size fma##_##size##x##_##double(double##size a, double##size b,      \
                                          double##size c) {                    \
    double##size res;                                                          \
    for (int i = 0; i < size; i++) {                                           \
      res[i] = __builtin_fma(a[i], b[i], c[i]);                                \
    }                                                                          \
    return res;                                                                \
  }

define_binary_vector(float, add, +, 2);
define_binary_vector(float, sub, -, 2);
define_binary_vector(float, mul, *, 2);
define_binary_vector(float, div, /, 2);
define_binary_vector(double, add, +, 2);
define_binary_vector(double, sub, -, 2);
define_binary_vector(double, mul, *, 2);
define_binary_vector(double, div, /, 2);
define_fma_vector(2);
define_fmaf_vector(2);

define_binary_vector(float, add, +, 4);
define_binary_vector(float, sub, -, 4);
define_binary_vector(float, mul, *, 4);
define_binary_vector(float, div, /, 4);
define_binary_vector(double, add, +, 4);
define_binary_vector(double, sub, -, 4);
define_binary_vector(double, mul, *, 4);
define_binary_vector(double, div, /, 4);
define_fma_vector(4);
define_fmaf_vector(4);

define_binary_vector(float, add, +, 8);
define_binary_vector(float, sub, -, 8);
define_binary_vector(float, mul, *, 8);
define_binary_vector(float, div, /, 8);
define_binary_vector(double, add, +, 8);
define_binary_vector(double, sub, -, 8);
define_binary_vector(double, mul, *, 8);
define_binary_vector(double, div, /, 8);
define_fma_vector(8);
define_fmaf_vector(8);

define_binary_vector(float, add, +, 16);
define_binary_vector(float, sub, -, 16);
define_binary_vector(float, mul, *, 16);
define_binary_vector(float, div, /, 16);
define_binary_vector(double, add, +, 16);
define_binary_vector(double, sub, -, 16);
define_binary_vector(double, mul, *, 16);
define_binary_vector(double, div, /, 16);
define_fma_vector(16);
define_fmaf_vector(16);

double sqrt_double(double a) { return __builtin_sqrt(a); }

double2 operator_2x_double(char op, double2 a, double2 b, double2 c) {
  switch (op) {
  case '+':
    return add_2x_double(a, b);
  case '-':
    return sub_2x_double(a, b);
  case 'x':
    return mul_2x_double(a, b);
  case '/':
    return div_2x_double(a, b);
  case 's':
    return sqrt_double(a);
  case 'f':
    return fma_2x_double(a, b, c);
  default:
    fprintf(stderr, "Error: unknown operation\n");
    abort();
  }
}

double4 operator_4x_double(char op, double4 a, double4 b, double4 c) {
  switch (op) {
  case '+':
    return add_4x_double(a, b);
  case '-':
    return sub_4x_double(a, b);
  case 'x':
    return mul_4x_double(a, b);
  case '/':
    return div_4x_double(a, b);
  case 's':
    return sqrt_double(a);
  case 'f':
    return fma_4x_double(a, b, c);
  default:
    fprintf(stderr, "Error: unknown operation\n");
    abort();
  }
}

double8 operator_8x_double(char op, double8 a, double8 b, double8 c) {
  switch (op) {
  case '+':
    return add_8x_double(a, b);
  case '-':
    return sub_8x_double(a, b);
  case 'x':
    return mul_8x_double(a, b);
  case '/':
    return div_8x_double(a, b);
  case 's':
    return sqrt_double(a);
  case 'f':
    return fma_8x_double(a, b, c);
  default:
    fprintf(stderr, "Error: unknown operation\n");
    abort();
  }
}

double16 operator_16x_double(char op, double16 a, double16 b, double16 c) {
  switch (op) {
  case '+':
    return add_16x_double(a, b);
  case '-':
    return sub_16x_double(a, b);
  case 'x':
    return mul_16x_double(a, b);
  case '/':
    return div_16x_double(a, b);
  case 's':
    return sqrt_double(a);
  case 'f':
    return fma_16x_double(a, b, c);
  default:
    fprintf(stderr, "Error: unknown operation\n");
    abort();
  }
}

float sqrt_float(float a) { return __builtin_sqrtf(a); }

float2 operator_2x_float(char op, float2 a, float2 b, float2 c) {
  switch (op) {
  case '+':
    return add_2x_float(a, b);
  case '-':
    return sub_2x_float(a, b);
  case 'x':
    return mul_2x_float(a, b);
  case '/':
    return div_2x_float(a, b);
  case 's':
    return sqrt_float(a);
  case 'f':
    return fma_2x_float(a, b, c);
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
  REAL##SIZE a = atof(argv[2]), b = 0, c = 0;

  if (op != 's')
    b = atof(argv[3]);

  if (op == 'f')
    c = atof(argv[4]);

#ifdef DOUBLE
  // fprintf(stderr, "[double] op = %c, a = %.13a, b = %.13a\n", op, a, b);
  printf("%.13a\n", operator_double(op, a, b, c, SIZE));
#elif defined(FLOAT)
  // fprintf(stderr, "[float] op = %c, a = %.6a, b = %.6a\n", op, a, b);
  printf("%.6a\n", operator_float(op, a, b, c, SIZE));
#endif

  return EXIT_SUCCESS;
}
