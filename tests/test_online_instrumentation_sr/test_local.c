#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "interflop/common/float_const.h"

#ifndef REAL
#warning "REAL is not defined, defaulting to double"
#define REAL double
#endif

double add2_double(double a, double b);
double sub2_double(double a, double b);
double mul2_double(double a, double b);
double div2_double(double a, double b);
double sqrt2_double(double a);

double operator_double(char op, double a, double b) {
  switch (op) {
  case '+':
    return add2_double(a, b);
  case '-':
    return sub2_double(a, b);
  case 'x':
    return mul2_double(a, b);
  case '/':
    return div2_double(a, b);
  case 's':
    return sqrt2_double(a);
  default:
    fprintf(stderr, "Error: unknown operation\n");
    abort();
  }
}

float add2_float(float a, float b);
float sub2_float(float a, float b);
float mul2_float(float a, float b);
float div2_float(float a, float b);
float sqrt2_float(float a);

float operator_float(char op, float a, float b) {
  switch (op) {
  case '+':
    return add2_float(a, b);
  case '-':
    return sub2_float(a, b);
  case 'x':
    return mul2_float(a, b);
  case '/':
    return div2_float(a, b);
  case 's':
    return sqrt2_float(a);
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
