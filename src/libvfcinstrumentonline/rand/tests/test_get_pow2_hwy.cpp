#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

// clang-format off
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "tests/get_pow2_test-hw.cpp"
#include "hwy/foreach_target.h"
// clang-format on

#include <gtest/gtest.h>

#include "hwy/highway.h"

#include "hwy/tests/test_util-inl.h"
#include "src/sr_hw-inl.h"
#include "src/utils.hpp"
#include "tests/helper.hpp"

HWY_BEFORE_NAMESPACE(); // at file scope

namespace sr {
namespace vector {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

namespace reference {
// return pred(|s|)

// compute 2 ** n
double pow2(int n) { return std::ldexp(1.0, n); }

}; // namespace reference

namespace {

struct TestPow2 {
  template <typename T, typename D> HWY_NOINLINE void operator()(T t, D d) {
    auto start = helper::IEEE754<float>::min_exponent_subnormal;
    auto end = helper::IEEE754<float>::max_exponent;
    for (int i = start; i <= end; i++) {
      test_equality<T, D>(i);
    }
  }

  template <typename T, typename D> void test_equality(int i) {
    const D d{};
    using DI = hn::RebindToSigned<D>;
    const DI di{};
    const auto vi = hn::Set(di, i);
    const auto reference = reference::pow2(i);
    const auto target = sr::vector::HWY_NAMESPACE::pow2(d, vi);

    auto target_min = hn::ReduceMin(d, target);
    auto target_max = hn::ReduceMax(d, target);

    std::hexfloat(std::cerr);

    if (target_min != target_max) {
      std::cerr << "All lane's value should be the same\n"
                << "input    : " << i << "\n"
                << "reference: " << reference << "\n"
                << "target   : " << target_min << " " << target_max << "\n";
      HWY_ASSERT(false);
    }

    if (target_min != reference) {
      std::cerr << "Failed for\n"
                << "input    : " << i << "\n"
                << "reference: " << reference << "\n"
                << "target   : " << target_min << "\n";
      HWY_ASSERT(false);
    }
  }
};

HWY_NOINLINE void TestAllPow2() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestPow2>());
}

} // namespace
// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
} // namespace vector
} // namespace sr
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace sr {
namespace vector {
namespace {

HWY_BEFORE_TEST(SRTest);
HWY_EXPORT_AND_TEST_P(SRTest, TestAllPow2);
HWY_AFTER_TEST();

} // namespace
} // namespace vector
} // namespace sr

HWY_TEST_MAIN();

#endif // HWY_ONCE