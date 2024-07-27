#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "src/utils.hpp"

namespace reference {
template <typename T> int32_t get_exponent(T a) {
  int exp = 0;
  auto cls = std::fpclassify(a);
  switch (cls) {
  case FP_ZERO:
    return 0;
  case FP_NORMAL:
    std::frexp(a, &exp);
    return exp - 1;
    break;
  case FP_SUBNORMAL:
    return std::numeric_limits<T>::min_exponent - 1;
  case FP_INFINITE:
  case FP_NAN:
    return std::numeric_limits<T>::max_exponent;
  default:
    std::cerr << "Error: unknown classification\n";
    std::abort();
    break;
  }
}
}; // namespace reference

struct RNG {

  std::random_device rd;

private:
  std::mt19937 gen;
  std::uniform_real_distribution<> dis;

public:
  RNG(double a = 0.0, double b = 1.0) : gen(RNG::rd()), dis(a, b){};
  double operator()() { return dis(gen); }
};

template <typename T> void testBinade(int n, int repetitions = 100) {
  auto start = std::ldexp(1.0, n);
  auto end = std::ldexp(1.0, n + 1);
  RNG rng(start, end);

  for (int i = 0; i < repetitions; i++) {
    T a = rng();
    EXPECT_EQ(reference::get_exponent(a), get_exponent(a))
        << a << " " << std::hexfloat << a << " failed";
    a = -a;
    EXPECT_EQ(reference::get_exponent(a), get_exponent(a))
        << a << " " << std::hexfloat << a << " failed";
  }
}

TEST(GetExponentTest, BasicAssertions) {

  std::vector<float> simple_case_float = {
      0.0f,
      1.0f,
      2.0f,
      3.0f,
      std::numeric_limits<float>::min(),
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::quiet_NaN()};

  for (auto a : simple_case_float) {
    EXPECT_EQ(reference::get_exponent(a), get_exponent(a))
        << a << " " << std::hexfloat << a << " failed";
    a = -a;
    EXPECT_EQ(reference::get_exponent(a), get_exponent(a))
        << a << " " << std::hexfloat << a << " failed";
  }

  std::vector<double> simple_case_double(simple_case_float.begin(),
                                         simple_case_float.end());

  for (auto a : simple_case_float) {
    EXPECT_EQ(reference::get_exponent(a), get_exponent(a))
        << a << " " << std::hexfloat << a << " failed";
    a = -a;
    EXPECT_EQ(reference::get_exponent(a), get_exponent(a))
        << a << " " << std::hexfloat << a << " failed";
  }
}

TEST(GetExponentTest, RandomAssertions) {
  RNG rng;

  for (int i = 0; i < 1000; i++) {
    float a = rng();
    EXPECT_EQ(reference::get_exponent(a), get_exponent(a))
        << a << " " << std::hexfloat << a << " failed";
    a = -a;
    EXPECT_EQ(reference::get_exponent(a), get_exponent(a))
        << a << " " << std::hexfloat << a << " failed";
  }

  for (int i = 0; i < 1000; i++) {
    double a = rng();
    EXPECT_EQ(reference::get_exponent(a), get_exponent(a))
        << a << " " << std::hexfloat << a << " failed";
    a = -a;
    EXPECT_EQ(reference::get_exponent(a), get_exponent(a))
        << a << " " << std::hexfloat << a << " failed";
  }
}

TEST(GetExponentTest, BinadeAssertions) {
  for (int i = -126; i < 127; i++) {
    testBinade<float>(i);
    testBinade<double>(i);
  }
}