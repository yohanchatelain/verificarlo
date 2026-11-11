#include <stdio.h>
#include <stdlib.h>

__attribute__((noinline)) double compute_residual(double a, double b, double c) {
  double s = a + b;
  double r = s - c;
  /* First comparison: always true (stable) */
  if (s > 0.1) {
    /* Second comparison: (0.1 + 0.2) - 0.3 > 0.0 is numerically unstable under MCA */
    if (r > 0.0) {
      return 1.0;
    } else {
      return -1.0;
    }
  }
  return 0.0;
}

int main(void) {
  volatile double a = 0.1;
  volatile double b = 0.2;
  volatile double c = 0.3;
  double res = compute_residual(a, b, c);
  printf("Result: %g\n", res);
  return 0;
}
