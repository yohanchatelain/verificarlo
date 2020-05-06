#include <stdlib.h>
#include <stdio.h>

#include <interflop.h>

float add(float a, float b) {
  return a+b;
}

int main(int argc, char *argv[]) {

  float a = 0.1;
  float b = 0.01;

  float c = add(a,b);

  fprintf(stderr, "%.6a\n", c);
  
  interflop_call(-1, SET_PRECISION, BINARY32_TYPE, 1);

  c = add(a,b);
  fprintf(stderr, "%.6a\n", c);
  
  interflop_call(-1, SET_INEXACT, BINARY32_TYPE, &c);
  
  fprintf(stderr, "%.6a\n", c);
  fprintf(stderr, "%.6a\n", c);

  return 0;
}
  
