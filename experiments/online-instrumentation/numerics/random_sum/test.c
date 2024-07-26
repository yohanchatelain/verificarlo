// harmonic series
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef REAL
#define REAL double
#endif

int main(int argc, char *argv[]) {

  int n = atoi(argv[1]);
  int seed = atoi(argv[2]);

  srand(seed);

  REAL sum = 1;
  for (int i = 1; i <= n; i++) {
    sum += 0.1;
  }
  fprintf(stderr, "%.13a\n", sum);
  return 0;
}