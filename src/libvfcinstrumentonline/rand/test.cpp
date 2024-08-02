#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>

#ifndef REAL
#define REAL double
#endif

template <typename T> void compute_proba(std::map<T, int> &visited_i, int n) {
  std::cout << std::hexfloat;
  for (auto [key, value] : visited_i) {
    std::cout << key << " " << value / (float)n << std::endl;
  }
}

template <typename T> T apply_op(const char op, T a, T b) {
  switch (op) {
  case '+':
    return a + b;
  case '-':
    return a - b;
  case '*':
    return a * b;
  case '/':
    return a / b;
  default:
    return 0;
  }
}

int main(int argc, char *argv[]) {

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <op> <a> <b>\n", argv[0]);
    return 1;
  }

  REAL a = strtod(argv[2], NULL);
  REAL b = strtod(argv[3], NULL);

  std::map<REAL, int> visited;

  for (int i = 0; i < 1000; i++)
    visited[apply_op(argv[1][0], a, b)]++;

  compute_proba(visited, 1000);

  return 0;
}