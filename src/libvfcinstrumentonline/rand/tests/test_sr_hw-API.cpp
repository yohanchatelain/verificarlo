#include <iostream>

// clang-format off
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "tests/test_sr_hw-API.cpp"
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

namespace hn = hwy::HWY_NAMESPACE;

namespace {

struct TestAdd {
  template <typename T, typename D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {

    const char *env_seed = getenv("VFC_DEBUG");
    const bool debug = env_seed && std::string(env_seed) == "1";

    const std::size_t N = (debug) ? 1 : 10000;

    std::map<T, int> results;
    for (size_t i = 0; i < N; i++) {
      auto res = run<T>(d, debug);
      results[res[0]]++;
      results[res[1]]++;
    }

    if (not debug)
      HWY_ASSERT_EQ(results.size(), 2);
  }

  template <typename T> T get_user_input(int id) throw() {
    static bool initialized = false;
    static double value = 0;

    if (initialized) {
      return value;
    }

    const std::string env_name = "SR_INPUT_TEST" + std::to_string(id);

    const char *env = getenv(env_name.c_str());

    if (env) {
      try {
        value = std::strtod(env, nullptr);
      } catch (const std::exception &e) {
        std::cerr << "Error parsing " << env_name << ": ";
        std::cerr << e.what() << '\n';
        std::cerr << "setting value to random\n";
      }
      initialized = true;
    }

    if (not initialized) {
      std::random_device rd;
      value = (double)rd();
      initialized = true;
    }

    return value;
  }

  std::string get_user_operation() throw() {
    static bool initialized = false;
    static std::string value = "add";

    if (initialized) {
      return value;
    }

    const char *env = getenv("SR_OP");

    if (env) {
      value = env;
      initialized = true;
    }

    return value;
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

    const T ai = (debug) ? get_user_input<T>(0) : 1.0;
    const T bi = (debug) ? get_user_input<T>(1) : ulp;

    T aa[elts];
    T bb[elts];

    for (size_t i = 0; i < elts; i++) {
      aa[i] = ai;
      bb[i] = bi;
    }

    auto a = hn::Load(d, aa);
    auto b = hn::Load(d, bb);

    if (debug) {
      hn::Print(d, "a", a, 0, 7, fmt);
      hn::Print(d, "b", b, 0, 7, fmt);
    }

    hn::Vec<D> c;

    auto op = get_user_operation();

    if (op == "add") {
      c = sr::vector::HWY_NAMESPACE::sr_add<D>(a, b);
    } else if (op == "sub") {
      c = sr::vector::HWY_NAMESPACE::sr_sub<D>(a, b);
    } else if (op == "mul") {
      c = sr::vector::HWY_NAMESPACE::sr_mul<D>(a, b);
    } else if (op == "div") {
      c = sr::vector::HWY_NAMESPACE::sr_div<D>(a, b);
    }

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
