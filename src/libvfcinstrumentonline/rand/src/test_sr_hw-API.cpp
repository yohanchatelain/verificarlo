#include <iostream>

// clang-format off
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "src/test_sr_hw-API.cpp"
#include "hwy/foreach_target.h"

#include "hwy/highway.h"
#include "hwy/base.h"
#include "hwy/tests/test_util-inl.h"
#include "src/debug_hwy-inl.h"
#include "src/sr_hw-inl.h"
// clang-format on

HWY_BEFORE_NAMESPACE(); // at file scope

namespace sr {

namespace HWY_NAMESPACE {
namespace {

struct TestAdd {
  template <typename T, typename D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {

    const char *env_seed = getenv("VFC_DEBUG");
    const bool debug = env_seed && std::string(env_seed) == "1";

    std::map<T, int> results;
    for (size_t i = 0; i < 10000; i++) {
      auto res = run<T>(d, debug);
      results[res[0]]++;
      results[res[1]]++;
    }

    HWY_ASSERT_EQ(results.size(), 2);
  }

  template <typename T, typename D>
  std::vector<T> run(D d, const bool debug = false) {
    constexpr T ulp = std::is_same_v<T, float> ? 0x1.0p-24 : 0x1.0p-53;
    constexpr const char *fmt = std::is_same_v<T, float> ? "%+.6a" : "%+.13a";

    const size_t elts = hn::Lanes(d);

    if (debug) {
      std::cout << "Type: " << typeid(T).name() << std::endl;
      std::cout << "Lanes: " << elts << std::endl;
    }

    T aa[elts] = {1};
    T bb[elts] = {ulp};

    for (size_t i = 0; i < elts; i++) {
      aa[i] = 1;
      bb[i] = ulp;
    }

    auto a = hn::Load(d, aa);
    auto b = hn::Load(d, bb);

    if (debug) {
      hn::Print(d, "a", a, 0, 7, fmt);
      hn::Print(d, "b", b, 0, 7, fmt);
    }

    auto c = sr_add<D>(a, b);

    if (debug) {

      hn::Print(d, "c", c, 0, 7, fmt);
    }

    auto c_min = hn::ReduceMin(d, c);
    auto c_max = hn::ReduceMax(d, c);

    if (debug) {
      std::cout << std::hexfloat;
      std::cout << "Min: " << c_min << std::endl;
      std::cout << "Max: " << c_max << std::endl;
    }

    return {c_min, c_max};
  }
};

HWY_NOINLINE void TestAllAdd() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestAdd>());
}

} // namespace
// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
} // namespace sr
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace sr {
namespace {
HWY_BEFORE_TEST(SRTest);
HWY_EXPORT_AND_TEST_P(SRTest, TestAllAdd);
HWY_AFTER_TEST();
} // namespace
} // namespace sr

HWY_TEST_MAIN();

#endif // HWY_ONCE
