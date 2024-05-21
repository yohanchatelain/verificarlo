#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "interflop/common/float_const.h"

#ifndef REAL
#warning "REAL is not defined, defaulting to double"
#define REAL double
#endif

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
