#include <stdio.h>

__attribute__((noinline)) int convert_near_int(double a, double b) {
  /* 0.1 + 0.9 == 1.0 in IEEE-754, but under MCA random rounding it straddles 1.0:
   * some replicas produce 0.9999999999999999 -> (int)0, others 1.0 -> (int)1. */
  double sum = a + b;
  int idx = (int)sum;
  return idx;
}

int main(void) {
  volatile double a = 0.1;
  volatile double b = 0.9;
  int idx = convert_near_int(a, b);
  printf("Converted index: %d\n", idx);
  return 0;
}
