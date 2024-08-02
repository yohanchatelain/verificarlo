#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "src/utils.hpp"
#include "tests/helper.hpp"

namespace reference {
// return pred(|s|)

// compute 2 ** n
template <typename T> T pow2(int n) { return std::ldexp(1.0, n); }
}; // namespace reference

#define test_equality(T, a)                                                    \
  EXPECT_EQ(reference::pow2<T>(a), sr::utils::pow2<T>(a))                      \
      << std::hexfloat << "Failed for\n"                                       \
      << "input    : " << a << "\n"                                            \
      << "reference: " << reference::pow2<T>(a) << "\n"                        \
      << "target   : " << sr::utils::pow2<T>(a);

TEST(GetPow2Test, FullRangeAssertions) {
  auto start = helper::IEEE754<float>::min_exponent_subnormal;
  auto end = helper::IEEE754<float>::max_exponent;
  for (int i = start - 1; i <= end; i++) {
    test_equality(float, i);
  }

  start = helper::IEEE754<float>::min_exponent_subnormal;
  end = helper::IEEE754<float>::max_exponent;
  for (int i = start - 1; i <= end; i++) {
    test_equality(double, i);
  }
}
