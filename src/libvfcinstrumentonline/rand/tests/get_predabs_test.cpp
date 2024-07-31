#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "src/utils.hpp"

namespace reference {
// return pred(|s|)

// ulp = SIGN(t) * ldexp(1, get_predecessor_abs(s*(1-ldexp(1, -53))) - 52);
// pred|s| = s * (1 - ldexp(1, -53))
// eta = get_predecessor_abs(pred(|s|))
// ulp = sign(tau) * 2^eta * eps
template <typename T> T get_predecessor_abs(T a) {
  constexpr uint32_t p = IEEE754<T>::precision;
  return a * (1 - std::ldexp(1.0, -p));
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

template <typename T> bool handle_nan_or_inf(T a) {
  using U = typename IEEE754<T>::U;
  auto ref = reference::get_predecessor_abs(a);
  auto target = get_predecessor_abs(a);
  if (std::isnan(a)) {
    EXPECT_TRUE(std::isnan(ref));
    EXPECT_TRUE(std::isnan(target));
    EXPECT_EQ(reinterpret_cast<U &>(ref), reinterpret_cast<U &>(target));
    return true;
  }
  if (std::isinf(a)) {
    EXPECT_TRUE(std::isinf(ref));
    EXPECT_TRUE(std::isinf(target));
    EXPECT_EQ(reinterpret_cast<U &>(ref), reinterpret_cast<U &>(target));
    return true;
  }
  return false;
}

#define test_equality(a)                                                       \
  if (handle_nan_or_inf(a))                                                    \
    return;                                                                    \
  EXPECT_EQ(reference::get_predecessor_abs(a), get_predecessor_abs(a))         \
      << std::hexfloat << "Failed for\n"                                       \
      << "input    : " << a << "\n"                                            \
      << "reference: " << reference::get_predecessor_abs(a) << "\n"            \
      << "target   : " << get_predecessor_abs(a);

template <typename T> void testBinade(int n, int repetitions = 100) {
  auto start = std::ldexp(1.0, n);
  auto end = std::ldexp(1.0, n + 1);
  RNG rng(start, end);

  for (int i = 0; i < repetitions; i++) {
    T a = rng();
    test_equality(a);
    test_equality(-a);
  }
}

TEST(GetPredAbsTest, BasicAssertions) {

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
    test_equality(a);
    test_equality(-a);
  }

  std::vector<double> simple_case_double(simple_case_float.begin(),
                                         simple_case_float.end());

  for (auto a : simple_case_double) {
    test_equality(a);
    test_equality(-a);
  }
}

TEST(GetPredAbsTest, RandomAssertions) {
  RNG rng;

  for (int i = 0; i < 1000; i++) {
    float a = rng();
    test_equality(a);
    test_equality(-a);
  }

  for (int i = 0; i < 1000; i++) {
    double a = rng();
    test_equality(a);
    test_equality(-a);
  }
}

TEST(GetPredAbsTest, BinadeAssertions) {
  for (int i = -126; i < 127; i++) {
    testBinade<float>(i);
    testBinade<double>(i);
  }
}