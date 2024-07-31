#include <stdint.h>
#include <stdio.h>

int main() {
  float a = 1.4999999;
  double b = 1.4999999;
  double c = 0.1;
  double d = __builtin_inff64();

  printf("a: %.13a\n", a);
  printf("b: %.13a\n", b);
  printf("c: %.13a\n", c);
  printf("d: %.13a\n", (float)c);
  printf("e: %.13a\n", d - d);

  return 0;
}
