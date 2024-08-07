#include <iostream>

#include <immintrin.h>

#include "src/debug.hpp"
#include "src/main.hpp"
#include "src/rand.hpp"
#include "src/sr.hpp"
#include "src/sr_avx2.hpp"

#ifndef UNROLL
#define UNROLL 1
#endif

void run_avx2_add_d(__m256d a, __m256d b, __m256d c[UNROLL]) {
  for (int i = 0; i < UNROLL; i++) {
    c[i] = sr_add_avx2(a, b);
  }
}

void run_avx2_mul_d(__m256d a, __m256d b, __m256d c[UNROLL]) {
  for (int i = 0; i < UNROLL; i++) {
    c[i] = sr_mul_avx2(a, b);
  }
}

void run_avx2_div_d(__m256d a, __m256d b, __m256d c[UNROLL]) {
  for (int i = 0; i < UNROLL; i++) {
    c[i] = sr_div_avx2(a, b);
  }
}

void run_add_d(double a, double b, double c[UNROLL]) {
  for (int i = 0; i < UNROLL; i++) {
    c[i] = sr_add(a, b);
  }
}

void run_mul_d(double a, double b, double c[UNROLL]) {
  for (int i = 0; i < UNROLL; i++) {
    c[i] = sr_mul(a, b);
  }
}

void run_div_d(double a, double b, double c[UNROLL]) {
  for (int i = 0; i < UNROLL; i++) {
    c[i] = sr_div(a, b);
  }
}

void print_m256d(__m256d a) {
  std::cout << std::hexfloat;
  for (int i = 0; i < sizeof(a) / sizeof(double); i++)
    std::cout << a[i] << " ";
  std::cout << std::defaultfloat;
}

void print_array_m256d(__m256d a[UNROLL]) {
  for (int i = 0; i < UNROLL; i++) {
    print_m256d(a[i]);
    std::cout << std::endl;
  }
}

void print_array_d(double a[UNROLL]) {
  std::cout << std::hexfloat;
  for (int i = 0; i < UNROLL; i++) {
    std::cout << a[i] << std::endl;
  }
  std::cout << std::defaultfloat;
}

void test_scalar_d(double a, double b) {
  std::cout << "sr_add" << std::endl;
  double cs[UNROLL];
  run_add_d(a, b, cs);
  print_array_d(cs);
  std::cout << "sr_mul" << std::endl;
  run_mul_d(a, b, cs);
  print_array_d(cs);
  std::cout << "sr_div" << std::endl;
  run_div_d(a, b, cs);
  print_array_d(cs);
}

void test_avx2_d(__m256d a, __m256d b) {
  std::cout << "sr_add_avx" << std::endl;
  __m256d c[UNROLL];
  run_avx2_add_d(a, b, c);

  std::cout << std::hexfloat;
  print_array_m256d(c);

  std::cout << "sr_mul_avx" << std::endl;
  run_avx2_mul_d(a, b, c);
  print_array_m256d(c);

  std::cout << "sr_div_avx" << std::endl;
  run_avx2_div_d(a, b, c);
  print_array_m256d(c);
}

int main() {

  init();

  std::vector<double> aa, bb;
  __m256d a, b;
  std::cout << "sizeof a " << sizeof(a) << std::endl;
  std::cout << "elements in a " << sizeof(a) / sizeof(double) << std::endl;

  for (int i = 0; i < 4; i++) {
    double x;
    std::cin >> x;
    aa.push_back(x);
  }
  for (int i = 0; i < 4; i++) {
    double x;
    std::cin >> x;
    bb.push_back(x);
  }

  a = _mm256_loadu_pd(aa.data());
  b = _mm256_loadu_pd(bb.data());

  test_avx2_d(a, b);
  test_scalar_d(a[0], b[0]);

  return 0;
}