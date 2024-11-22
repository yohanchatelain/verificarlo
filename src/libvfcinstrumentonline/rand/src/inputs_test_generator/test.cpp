#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>

const std::size_t N = 1'000'000;

#ifndef REAL
#define REAL double
#warning "REAL not defined, using double"
#endif

template <typename T> void compute_proba(std::map<T, int> &visited_i, int n) {
  std::cout << "Visited: " << visited_i.size() << std::endl;
  for (auto [key, value] : visited_i) {
    std::cout << std::hexfloat << key << " " << std::defaultfloat
              << value / (float)n << std::endl;
  }
}

template <typename T> T apply_op(const char op, T a, T b, T c) {
  switch (op) {
  case '+':
    return a + b;
  case '-':
    return a - b;
  case 'x':
    return a * b;
  case '/':
    return a / b;
  case 'f':
    return a * b + c;
  default:
    return 0;
  }
}

int main(int argc, char *argv[]) {

  bool is_fma = false;
  if (argc == 5) {
    // fma case
    is_fma = true;
  } else if (argc != 4) {
    fprintf(stderr, "Usage: %s <op> <a> <b>\n", argv[0]);
    return 1;
  }

  if constexpr (std::is_same_v<REAL, float>) {
    std::cout << "Using float\n";
  } else if constexpr (std::is_same_v<REAL, double>) {
    std::cout << "Using double\n";
  }

  REAL a = strtod(argv[2], NULL);
  REAL b = strtod(argv[3], NULL);
  REAL c = (is_fma) ? strtod(argv[4], NULL) : 0;

  std::map<REAL, int> visited;

  for (int i = 0; i < N; i++)
    visited[apply_op(argv[1][0], a, b, c)]++;

  compute_proba(visited, N);

  return 0;
}