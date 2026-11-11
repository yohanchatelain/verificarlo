#include <stdio.h>

int main(void) {
  volatile double step = 0.5;
  int count = 0;
  for (int i = 0; i < 10000; i++) {
    double x = (double)i + step;
    double y = (double)i;
    if (x > y) {
      count++;
    }
  }
  printf("Stable count: %d\n", count);
  return (count == 10000) ? 0 : 1;
}
