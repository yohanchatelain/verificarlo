#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "helper.hpp"
#include "src/eft.hpp"
#include "src/utils.hpp"
#include "tests/helper.hpp"

namespace reference {
// return pred(|s|)

// twosum reference
// compute in double precision if the input type is float
// compute in quad precision if the input type is double
template <typename T, typename R = typename helper::IEEE754<T>::H>
R twoprodfma(T a, T b) {
  using H = typename helper::IEEE754<T>::H;
  return static_cast<H>(a) * static_cast<H>(b);
}

}; // namespace reference

template <typename T> void is_close(T a, T b) {
  if (std::isnan(a) or std::isnan(b) or std::isinf(a) or std::isinf(b))
    return;

  using H = typename helper::IEEE754<T>::H;
  H ref = reference::twoprodfma(a, b);
  T ref_cast = static_cast<T>(ref);
  T x = 0, e = 0;
  twoprodfma(a, b, x, e);
  H target = x + e;

  if (std::isnan(x) and std::isnan(ref_cast))
    return;

  if (std::isinf(x) and std::isinf(ref_cast))
    return;

  auto diff = helper::absolute_distance(ref, target);
  auto rel = helper::relative_distance(ref, target);

  auto ulp = helper::IEEE754<T>::ulp;
  auto min_subnormal = helper::IEEE754<T>::min_subnormal;
  auto min_normal = helper::IEEE754<T>::min_normal;

  T error_bound = 0;
  bool correct = false;
  if (diff == 0) {
    correct = true;
  } else if (diff < min_subnormal) {
    correct = true;
  } else if (diff < min_normal) {
    error_bound = ulp;
    correct = rel <= error_bound;
  } else {
    error_bound = .5 * ulp;
    correct = rel <= error_bound;
  }

  EXPECT_TRUE(correct) << std::hexfloat << "Failed for\n"
                       << "type: " << typeid(a).name() << "\n"
                       << "ulp: " << ulp << "\n"
                       << "bound: " << error_bound << "\n"
                       << "lowest: " << min_subnormal << "\n"
                       << "a    : " << a << "\n"
                       << "b    : " << b << "\n"
                       << "target : "
                       << "(" << x << " , " << e << ")\n"
                       << "reference: " << (double)ref << "\n"
                       << "target   : " << (double)target << "\n"
                       << "abs_diff     : " << (double)diff << "\n"
                       << "rel_diff     : " << (double)rel;
}

#define test_equality(a, b) is_close(a, b)

template <typename T> std::vector<T> get_simple_case() {
  std::vector<T> simple_case = {0.0,
                                1.0,
                                2.0,
                                3.0,
                                std::numeric_limits<T>::min(),
                                std::numeric_limits<T>::lowest(),
                                std::numeric_limits<T>::max(),
                                std::numeric_limits<T>::epsilon(),
                                std::numeric_limits<T>::infinity(),
                                std::numeric_limits<T>::denorm_min(),
                                std::numeric_limits<T>::quiet_NaN(),
                                std::numeric_limits<T>::signaling_NaN()};
  return simple_case;
}

TEST(GetTwoSumTest, BasicAssertions) {

  std::vector<float> simple_case_float = get_simple_case<float>();
  for (auto a : simple_case_float) {
    for (auto b : simple_case_float) {
      test_equality(a, b);
      test_equality(a, -b);
      test_equality(-a, b);
      test_equality(-a, -b);
    }
  }

  std::vector<double> simple_case_double = get_simple_case<double>();
  for (auto a : simple_case_double) {
    for (auto b : simple_case_double) {
      test_equality(a, b);
      test_equality(a, -b);
      test_equality(-a, b);
      test_equality(-a, -b);
    }
  }
}

TEST(GetTwoSumTest, Random01Assertions) {
  helper::RNG rng;

  for (int i = 0; i < 1000; i++) {
    float a = rng();
    float b = rng();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }

  for (int i = 0; i < 1000; i++) {
    double a = rng();
    double b = rng();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }
}

TEST(GetTwoSumTest, RandomNoOverlapAssertions) {
  helper::RNG rng_0v1_float(0, 1);
  helper::RNG rng_24v25_float(0x1p24, 0x1p25);

  for (int i = 0; i < 1000; i++) {
    float a = rng_0v1_float();
    float b = rng_24v25_float();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }

  helper::RNG rng_0v1_double(0, 1);
  helper::RNG rng_53v54_double(0x1p53, 0x1p54);

  for (int i = 0; i < 1000; i++) {
    double a = rng_0v1_double();
    double b = rng_53v54_double();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }
}

TEST(GetTwoSumTest, RandomLastBitOverlapAssertions) {
  helper::RNG rng_1v2_float(1, 2);
  helper::RNG rng_23v24_float(0x1p23, 0x1p24);

  for (int i = 0; i < 1000; i++) {
    float a = rng_1v2_float();
    float b = rng_23v24_float();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }

  helper::RNG rng_1v2_double(1, 2);
  helper::RNG rng_52v53_double(0x1p52, 0x1p53);

  for (int i = 0; i < 1000; i++) {
    double a = rng_1v2_double();
    double b = rng_52v53_double();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }
}

TEST(GetTwoSumTest, RandomMidOverlapAssertions) {
  helper::RNG rng_0v1_float(0, 1);
  helper::RNG rng_12v13_float(0x1p12, 0x1p13);

  for (int i = 0; i < 1000; i++) {
    float a = rng_0v1_float();
    float b = rng_12v13_float();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }

  helper::RNG rng_0v1_double(0, 1);
  helper::RNG rng_26v27_double(0x1p26, 0x1p27);

  for (int i = 0; i < 1000; i++) {
    double a = rng_0v1_double();
    double b = rng_26v27_double();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }
}

template <typename T> void run_test_binade() {
  constexpr auto min_exponent = helper::IEEE754<T>::min_exponent_subnormal;
  constexpr auto max_exponent = helper::IEEE754<T>::max_exponent;
  std::function<void(T, T)> test = is_close<T>;
  for (int i = min_exponent; i <= max_exponent; i++)
    helper::test_binade<T>(i, test);
}

TEST(GetTwoSumTest, BinadeAssertions) {
  run_test_binade<float>();
  run_test_binade<double>();
}