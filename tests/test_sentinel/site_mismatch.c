#include <stdio.h>
#include <stdlib.h>

__attribute__((noinline)) int branch_a(double x) {
  return (x > 0.0) ? 1 : 0;
}

__attribute__((noinline)) int branch_b(double x) {
  return (x < 10.0) ? 1 : 0;
}

int main(void) {
  volatile double x = 1.0;
  const char *rep = getenv("VFC_SENTINEL_ACTIVE_REPLICA");
  /* Simulate uninstrumented control-flow divergence so replicas reach
   * different fcmp instruction addresses at the same sequence number. */
  int r = 0;
  if (rep && atoi(rep) == 1) {
    r = branch_b(x);
  } else {
    r = branch_a(x);
  }
  printf("r = %d\n", r);
  return 0;
}
