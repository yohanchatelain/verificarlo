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
#define SIZE 1
#endif

#ifndef N
#define N 1
#endif

#ifdef __GNUC__
typedef double double1;
typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef double double8 __attribute__((ext_vector_type(8)));
// typedef double double16 __attribute__((ext_vector_type(16)));
typedef float float1;
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef float float8 __attribute__((ext_vector_type(8)));
typedef float float16 __attribute__((ext_vector_type(16)));
typedef int int1;
typedef int int2 __attribute__((ext_vector_type(2)));
typedef int int4 __attribute__((ext_vector_type(4)));
typedef int int8 __attribute__((ext_vector_type(8)));
typedef int int16 __attribute__((ext_vector_type(16)));
#elif __clang__
typedef float float1;
typedef float float2 __attribute__((vector_size(8)));
typedef float float4 __attribute__((vector_size(16)));
typedef float float8 __attribute__((vector_size(32)));
typedef float float16 __attribute__((vector_size(64)));
typedef double double1;
typedef double double2 __attribute__((vector_size(16)));
typedef double double4 __attribute__((vector_size(32)));
typedef double double8 __attribute__((vector_size(64)));
// typedef double double16 __attribute__((vector_size(128)));
typedef int int1;
typedef int int2 __attribute__((vector_size(8)));
typedef int int4 __attribute__((vector_size(16)));
typedef int int8 __attribute__((vector_size(32)));
typedef int int16 __attribute__((vector_size(64)));
#else
#error "Compiler must be gcc or clang"
#endif

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
  float##size fma##_##size##x##_##float(float##size a, float##size b,          \
                                        float##size c) {                       \
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

double1 fma_1x_double(double1 a, double1 b, double1 c) {
  return __builtin_fma(a, b, c);
}

float1 fma_1x_float(float1 a, float1 b, float1 c) {
  return __builtin_fmaf(a, b, c);
}

/* scalar functions */
define_binary_vector(float, add, +, 1);
define_binary_vector(float, sub, -, 1);
define_binary_vector(float, mul, *, 1);
define_binary_vector(float, div, /, 1);
define_binary_vector(double, add, +, 1);
define_binary_vector(double, sub, -, 1);
define_binary_vector(double, mul, *, 1);
define_binary_vector(double, div, /, 1);

/* vector functions */
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
// define_binary_vector(double, add, +, 16);
// define_binary_vector(double, sub, -, 16);
// define_binary_vector(double, mul, *, 16);
// define_binary_vector(double, div, /, 16);
// define_fma_vector(16);
define_fmaf_vector(16);

#define define_operator_vector(type, size)                                     \
  type##size operator_##size##x_##type(char op, type##size a, type##size b,    \
                                       type##size c) {                         \
    switch (op) {                                                              \
    case '+':                                                                  \
      return add##_##size##x##_##type(a, b);                                   \
    case '-':                                                                  \
      return sub##_##size##x##_##type(a, b);                                   \
    case 'x':                                                                  \
      return mul##_##size##x##_##type(a, b);                                   \
    case '/':                                                                  \
      return div##_##size##x##_##type(a, b);                                   \
    case 'f':                                                                  \
      return fma##_##size##x##_##type(a, b, c);                                \
    default:                                                                   \
      fprintf(stderr, "Error: unknown operation\n");                           \
      abort();                                                                 \
    }                                                                          \
  }

define_operator_vector(double, 1);
define_operator_vector(double, 2);
define_operator_vector(double, 4);
define_operator_vector(double, 8);
// define_operator_vector(double, 16);
define_operator_vector(float, 1);
define_operator_vector(float, 2);
define_operator_vector(float, 4);
define_operator_vector(float, 8);
define_operator_vector(float, 16);

void operator_double(char op, double a, double b, double c, int size) {
  switch (size) {
  case 1: {
    double1 aa = a, bb = b, cc = c;
    double1 res1 = operator_1x_double(op, aa, bb, cc);
    fprintf(stderr, "%.13a\n", res1);
  } break;
  case 2: {
    double2 aa = {a, 2 * a}, bb = {b, 2 * b}, cc = {c, 2 * c};
    double2 res2 = operator_2x_double(op, aa, bb, cc);
    fprintf(stderr, "%.13a %.13a\n", res2[0], res2[1]);
  } break;
  case 4: {
    double4 aaa = {a, 2 * a, 3 * a, 4 * a}, bbb = {b, 2 * b, 3 * b, 4 * b},
            ccc = {c, 2 * c, 3 * c, 4 * c};
    double4 res4 = operator_4x_double(op, aaa, bbb, ccc);
    fprintf(stderr, "%.13a %.13a %.13a %.13a\n", res4[0], res4[1], res4[2],
            res4[3]);
  } break;
  case 8: {
    double8 aaaa = {a, 2 * a, 3 * a, 4 * a, 5 * a, 6 * a, 7 * a, 8 * a},
            bbbb = {b, 2 * b, 3 * b, 4 * b, 5 * b, 6 * b, 7 * b, 8 * b},
            cccc = {c, 2 * c, 3 * c, 4 * c, 5 * c, 6 * c, 7 * c, 8 * c};
    double8 res8 = operator_8x_double(op, aaaa, bbbb, cccc);
    fprintf(stderr, "%.13a %.13a %.13a %.13a %.13a %.13a %.13a %.13a\n",
            res8[0], res8[1], res8[2], res8[3], res8[4], res8[5], res8[6],
            res8[7]);
  } break;
  // case 16: {
  //   double16 aaaaa = {a,      2 * a,  3 * a,  4 * a,  5 * a,  6 * a,
  //                     7 * a,  8 * a,  9 * a,  10 * a, 11 * a, 12 * a,
  //                     13 * a, 14 * a, 15 * a, 16 * a},
  //            bbbbb = {b,      2 * b,  3 * b,  4 * b,  5 * b,  6 * b,
  //                     7 * b,  8 * b,  9 * b,  10 * b, 11 * b, 12 * b,
  //                     13 * b, 14 * b, 15 * b, 16 * b},
  //            ccccc = {c,      2 * c,  3 * c,  4 * c,  5 * c,  6 * c,
  //                     7 * c,  8 * c,  9 * c,  10 * c, 11 * c, 12 * c,
  //                     13 * c, 14 * c, 15 * c, 16 * c};
  //   double16 res16 = operator_16x_double(op, aaaaa, bbbbb, ccccc);
  //   fprintf(stderr,
  //           "%.13a %.13a %.13a %.13a %.13a %.13a %.13a %.13a %.13a %.13a "
  //           "%.13a %.13a %.13a %.13a %.13a %.13a\n",
  //           res16[0], res16[1], res16[2], res16[3], res16[4], res16[5],
  //           res16[6], res16[7], res16[8], res16[9], res16[10], res16[11],
  //           res16[12], res16[13], res16[14], res16[15]);
  // } break;
  default:
    fprintf(stderr, "Error: unknown size\n");
    abort();
    break;
  }
}

void operator_float(char op, float a, float b, float c, int size) {
  switch (size) {
  case 1: {
    float1 aa = a, bb = b, cc = c;
    float1 res1 = operator_1x_float(op, aa, bb, cc);
    fprintf(stderr, "%.6a\n", res1);
  } break;
  case 2: {
    float2 aa, bb, cc;
    for (int i = 0; i < 2; i++) {
      aa[i] = (i + 1) * a;
      bb[i] = (i + 2) * b;
      cc[i] = (i + 3) * c;
    }
    float2 res2 = operator_2x_float(op, aa, bb, cc);
    fprintf(stderr, "%.6a %.6a\n", res2[0], res2[1]);
  } break;
  case 4: {
    float4 aaa = {a, 2 * a, 3 * a, 4 * a}, bbb = {b, 2 * b, 3 * b, 4 * b},
           ccc = {c, 2 * c, 3 * c, 4 * c};
    float4 res4 = operator_4x_float(op, aaa, bbb, ccc);
    fprintf(stderr, "%.6a %.6a %.6a %.6a\n", res4[0], res4[1], res4[2],
            res4[3]);
  } break;
  case 8: {
    float8 aaaa = {a, 2 * a, 3 * a, 4 * a, 5 * a, 6 * a, 7 * a, 8 * a},
           bbbb = {b, 2 * b, 3 * b, 4 * b, 5 * b, 6 * b, 7 * b, 8 * b},
           cccc = {c, 2 * c, 3 * c, 4 * c, 5 * c, 6 * c, 7 * c, 8 * c};
    float8 res8 = operator_8x_float(op, aaaa, bbbb, cccc);
    fprintf(stderr, "%.6a %.6a %.6a %.6a %.6a %.6a %.6a %.6a\n", res8[0],
            res8[1], res8[2], res8[3], res8[4], res8[5], res8[6], res8[7]);
  } break;
  case 16: {
    float16 aaaaa = {a,      2 * a,  3 * a,  4 * a,  5 * a,  6 * a,
                     7 * a,  8 * a,  9 * a,  10 * a, 11 * a, 12 * a,
                     13 * a, 14 * a, 15 * a, 16 * a},
            bbbbb = {b,      2 * b,  3 * b,  4 * b,  5 * b,  6 * b,
                     7 * b,  8 * b,  9 * b,  10 * b, 11 * b, 12 * b,
                     13 * b, 14 * b, 15 * b, 16 * b},
            ccccc = {c,      2 * c,  3 * c,  4 * c,  5 * c,  6 * c,
                     7 * c,  8 * c,  9 * c,  10 * c, 11 * c, 12 * c,
                     13 * c, 14 * c, 15 * c, 16 * c};
    float16 res16 = operator_16x_float(op, aaaaa, bbbbb, ccccc);
    fprintf(stderr,
            "%.6a %.6a %.6a %.6a %.6a %.6a %.6a %.6a %.6a %.6a "
            "%.6a %.6a %.6a %.6a %.6a %.6a\n",
            res16[0], res16[1], res16[2], res16[3], res16[4], res16[5],
            res16[6], res16[7], res16[8], res16[9], res16[10], res16[11],
            res16[12], res16[13], res16[14], res16[15]);
  } break;
  default:
    break;
  }
}

int main(int argc, const char *argv[]) {

  if (argc == 3) {
    if (argv[1][0] != 's') {
      fprintf(stderr, "usage: ./test <op> a\n");
      exit(1);
    }
  } else if (argc == 5) {
    // if (argv[1][0] != 'f') {
    //   fprintf(stderr, "usage: ./test <op> a b c\n");
    //   exit(1);
    // }
  } else if (argc != 4) {
    fprintf(stderr, "usage: ./test <op> a b\n");
    exit(1);
  }

  const char op = argv[1][0];
  REAL a = atof(argv[2]), b = 0, c = 0;
  int n = 1;

  if (op != 's')
    b = atof(argv[3]);

  if (op == 'f')
    c = atof(argv[4]);

  if (argc == 5)
    n = atoi(argv[4]);

  for (int i = 0; i < n; i++) {
#ifdef DOUBLE
    // fprintf(stderr, "[double] op = %c, a = %.13a, b = %.13a\n", op, a, b);
    operator_double(op, a, b, c, SIZE);
#elif defined(FLOAT)
    // fprintf(stderr, "[float] op = %c, a = %.6a, b = %.6a\n", op, a, b);
    operator_float(op, a, b, c, SIZE);
#endif
  }

  return EXIT_SUCCESS;
}
