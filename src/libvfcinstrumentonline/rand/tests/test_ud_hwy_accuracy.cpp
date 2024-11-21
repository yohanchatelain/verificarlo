#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iostream>

// clang-format off
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "tests/test_ud_hwy_accuracy.cpp"
#include "hwy/foreach_target.h"

#include "hwy/highway.h"
#include "hwy/base.h"
#include "hwy/tests/test_util-inl.h"
#include "src/debug_hwy-inl.h"
#include "src/ud_hw-inl.h"
// clang-format on

#include "src/ud_hw.h"
#include "src/utils.hpp"
#include "tests/helper.hpp"

HWY_BEFORE_NAMESPACE(); // at file scope

namespace sr {

namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

namespace {

using ::testing::AllOf;
using ::testing::Ge;
using ::testing::Le;
using ::testing::Lt;

constexpr auto default_alpha = 0.0001;
#ifdef SR_DEBUG
constexpr auto default_repetitions = 100;
#else
constexpr auto default_repetitions = 1'000;
#endif

static auto distribution_failed_tests_counter = 0;
static auto distribution_tests_counter = 0;

namespace reference {

// twosum reference
// compute in double precision if the input type is float
// compute in quad precision if the input type is double
template <typename T, typename H = typename helper::IEEE754<T>::H>
H add(T a, T b) {
  return static_cast<H>(a) + static_cast<H>(b);
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
H sub(T a, T b) {
  return static_cast<H>(a) - static_cast<H>(b);
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
H mul(T a, T b) {
  return static_cast<H>(a) * static_cast<H>(b);
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
H div(T a, T b) {
  return static_cast<H>(a) / static_cast<H>(b);
}

}; // namespace reference

namespace udvh = ud::vector::HWY_NAMESPACE;

struct SRAdd {
  static constexpr char name[] = "add";
  static constexpr char symbol[] = "+";

  template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
  V operator()(D d, V a, V b) {
    return udvh::add<D>(a, b);
  }

  template <typename T, typename H = typename helper::IEEE754<T>::H>
  static H reference(T a, T b) {
    return reference::add<T>(a, b);
  }
};

struct SRSub {
  static constexpr char name[] = "sub";
  static constexpr char symbol[] = "-";

  template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
  V operator()(D d, V a, V b) {
    return udvh::sub<D>(a, b);
  }

  template <typename T, typename H = typename helper::IEEE754<T>::H>
  static H reference(T a, T b) {
    return reference::sub<T>(a, b);
  }
};

struct SRMul {
  static constexpr char name[] = "mul";
  static constexpr char symbol[] = "*";

  template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
  V operator()(D d, V a, V b) {
    return udvh::mul<D>(a, b);
  }

  template <typename T, typename H = typename helper::IEEE754<T>::H>
  static H reference(T a, T b) {
    return reference::mul<T>(a, b);
  }
};

struct SRDiv {
  static constexpr char name[] = "div";
  static constexpr char symbol[] = "/";

  template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
  V operator()(D d, V a, V b) {
    return udvh::div<D>(a, b);
  }

  template <typename T, typename H = typename helper::IEEE754<T>::H>
  static H reference(T a, T b) {
    return reference::div<T>(a, b);
  }
};

template <typename T, typename H = typename helper::IEEE754<T>::H>
std::tuple<H, H, H, H, H, std::string> compute_distance_error(T a, T b,
                                                              H reference) {
  T ref_cast = static_cast<T>(reference);

  if (helper::isnan(a) or helper::isnan(b) or helper::isnan(reference) or
      helper::isinf(a) or helper::isinf(b) or helper::isinf(reference) or
      helper::isinf(ref_cast)) {
    return {0, 0, reference, reference, 0, ""};
  }

  H error = 0, error_c = 0;
  H probability_down = 0, probability_up = 0;
  H next = 0, prev = 0;
  H ulp = helper::get_ulp(ref_cast);

  if (ref_cast == reference) {
    error = 0;
    probability_down = 1;
    probability_up = 1;
    prev = reference;
    next = reference;
  } else {
    error = helper::absolute_distance(reference, static_cast<H>(ref_cast));
    error_c = helper::absolute_distance(static_cast<H>(ulp), error);
    prev = (ref_cast < reference) ? ref_cast : (ref_cast - ulp);
    next = (ref_cast < reference) ? (ref_cast + ulp) : ref_cast;
    probability_down = (next - reference) / ulp;
    probability_up = (reference - prev) / ulp;

    if (((error + error_c) != ulp)) {
      std::cerr << "error + error_c != ulp" << "\n"
                << "error:   " << helper::hexfloat(error) << "\n"
                << "error_c: " << helper::hexfloat(error_c) << "\n"
                << "prev:    " << helper::hexfloat(prev) << "\n"
                << "next:    " << helper::hexfloat(next) << "\n"
                << "ulp:     " << helper::hexfloat(ulp) << std::endl;
      HWY_ASSERT(false);
    }
    if (probability_down + probability_up != 1) {
      std::cerr << "probability_down + probability_up != 1" << "\n"
                << "probability_down: " << probability_down << "\n"
                << "probability_up:   " << probability_up << "\n"
                << "reference:        " << helper::hexfloat(reference) << "\n"
                << "prev:             " << helper::hexfloat(prev) << "\n"
                << "next:             " << helper::hexfloat(next) << "\n"
                << "error:            " << helper::hexfloat(error) << "\n"
                << "error_c:          " << helper::hexfloat(error_c) << "\n"
                << "ulp:              " << helper::hexfloat(ulp) << std::endl;
      HWY_ASSERT(false);
    }
  }

  std::ostringstream os;
  os << std::hexfloat << std::setprecision(13);
  os << "-- compute_distance_error --" << std::endl;
  os << "         reference: " << helper::hexfloat(reference) << std::endl;
  if constexpr (std::is_same_v<T, float>) {
    os << "  (float)reference: " << helper::hexfloat(ref_cast) << std::endl;
    os << "  |ref-(float)ref|: " << helper::hexfloat(error) << std::endl;
  }
  if constexpr (std::is_same_v<T, double>) {
    os << " (double)reference: " << helper::hexfloat(ref_cast) << std::endl;
    os << " |ref-(double)ref|: " << helper::hexfloat(error) << std::endl;
  }
  os << "           error_c: " << helper::hexfloat(error_c) << std::endl;
  os << "               ulp: " << helper::hexfloat(ulp) << std::endl;
  os << "       reference ↓: " << helper::hexfloat(prev) << std::endl;
  os << "       reference ↑: " << helper::hexfloat(next) << std::endl;
  os << std::defaultfloat;
  os << "                 p: " << probability_down << std::endl;
  os << "               1-p: " << probability_up << std::endl;
  auto msg = os.str();

  return {probability_down, probability_up, prev, next, error, msg};
}

template <class Op, class D, class V = hn::VFromD<D>,
          typename T = hn::TFromD<D>>
std::vector<helper::Counter<T>> eval_op(D d, V a, V b, const int repetitions) {

  const size_t lanes = hn::Lanes(d);
  auto op = Op();

  std::vector<helper::Counter<T>> c(lanes);
  for (int i = 0; i < repetitions; i++) {
    auto v = op(d, a, b);
    for (size_t j = 0; j < lanes; j++) {
      auto vj = hn::ExtractLane(v, j);
      c[j][vj]++;
    }
  }

  return c;
}

template <typename T> std::string fmt_proba(const T &x) {
  std::ostringstream os;
  os << std::fixed << std::setprecision(5) << x << std::defaultfloat;
  return os.str();
}

std::string flush() {
  debug_flush();
  return "";
}

template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
void assert_equal_inputs(D d, V a) {
  auto a_min = hn::ReduceMin(d, a);
  auto a_max = hn::ReduceMax(d, a);
  if (helper::isnan(a_min) and helper::isnan(a_max))
    return;

#if HWY_TARGET != HWY_EMU128
  if (a_min != a_max) {
    std::cerr << "Vector does not have equal inputs!\n"
              << "min(a): " << helper::hexfloat(a_min) << "\n"
              << "max(a): " << helper::hexfloat(a_max) << std::endl;
    HWY_ASSERT(false);
  }
#endif
}

template <class Op, class D, class V = hn::VFromD<D>,
          typename T = hn::TFromD<D>>
void check_distribution_match(D d, V va, V vb,
                              const long repetitions = default_repetitions,
                              const float alpha = default_alpha) {
  using H = typename helper::IEEE754<T>::H;

  auto a = hn::GetLane(va);
  auto b = hn::GetLane(vb);

  // ensure that we have vector of same value
  assert_equal_inputs(d, va);
  assert_equal_inputs(d, vb);

  auto counters = eval_op<Op>(d, va, vb, repetitions);

  H reference = Op::reference(a, b);
  auto [probability_down, probability_up, down, up, distance_error,
        distance_error_msg] = compute_distance_error(a, b, reference);

  size_t lane = 0;
  for (auto &counter : counters) {

    auto count_down = counter.down_count();
    auto count_up = counter.up_count();
    auto probability_down_estimated = count_down / (double)repetitions;
    auto probability_up_estimated = count_up / (double)repetitions;

    if (not helper::isfinite(counter.up()) or
        not helper::isfinite(counter.down())) {
      if (helper::isnan(counter.up()) and helper::isnan(reference))
        return;
      if (helper::isnan(counter.down()) and helper::isnan(reference))
        return;
      if (helper::isinf(counter.up()) and helper::isinf(reference)) {
        if (counter.up() != reference) {
          std::cerr << "Error in inf comparison\n"
                    << "counter.up() != reference\n"
                    << "counter.up(): " << counter.up() << "\n"
                    << "reference: " << reference << "\n"
                    << distance_error_msg << std::defaultfloat << flush();
          HWY_ASSERT(false);
        }
      }
      if (helper::isinf(counter.down()) and helper::isinf(reference)) {
        if (counter.down() != reference) {
          std::cerr << "Error in inf comparison\n"
                    << "counter.down() != reference\n"
                    << "counter.down(): " << counter.down() << "\n"
                    << "reference: " << reference << "\n"
                    << distance_error_msg << std::defaultfloat << flush();
          HWY_ASSERT(false);
        }
      }
      return;
    }

    /* code */
    if (0.0 > probability_down or probability_down > 1.0) {
      std::cerr << "Probability ↓ is not in [0, 1] range\n"
                << "-- theoretical -\n"
                << "   probability ↓: " << fmt_proba(probability_down) << "\n"
                << "   probability ↑: " << fmt_proba(probability_up) << "\n"
                << "--- estimated --\n"
                << "     sample size: " << repetitions << "\n"
                << "              #↓: " << count_down << " ("
                << fmt_proba(probability_down_estimated) << ")\n"
                << "              #↑: " << count_up << " ("
                << fmt_proba(probability_up_estimated) << ")\n"
                << std::hexfloat << "" << "              ↓: " << counter.down()
                << "\n"
                << "              ↑: " << counter.up() << "\n"
                << distance_error_msg << std::defaultfloat << flush();
      HWY_ASSERT(false);
    }

    // do the test only if the operation is not exact (probability is not zero)
    bool is_exact = probability_down == 1 and probability_up == 1;

    // do the test only if the operation is not exact (probability is not zero)
    // do the test if the distance between the reference and the estimated value
    // is greater than the minimum subnormal
    // do not the test if the probability is lower than 1/repetitions
    bool compare_down_values =
        not is_exact and down != 0 and
        probability_down > (1.0 / repetitions) and
        distance_error > helper::IEEE754<T>::min_subnormal;
    bool compare_up_values = not is_exact and up != 0 and
                             probability_up > (1.0 / repetitions) and
                             distance_error > helper::IEEE754<T>::min_subnormal;

    // 95% relative error
    auto error_down = (probability_down_estimated / probability_down);
    auto error_up = (probability_up_estimated / probability_up);

    // binomial test
    auto test = helper::binomial_test(repetitions, count_down, .5);

    const auto op_name = Op::name;
    const auto op_symbol = Op::symbol;
    const auto ftype = typeid(T).name();

    // Bonferroni correction, divide by the number of lanes
    const auto lanes = hn::Lanes(d);
    const auto alpha_bon = (alpha / 2) / lanes;

    if (compare_down_values and compare_up_values and test.pvalue < alpha_bon) {
      std::cerr << "Null hypotheis rejected!\n"
                << "     Lane/#Lanes: " << lane + 1 << "/" << lanes << "\n"
                << "            type: " << ftype << "\n"
                << "              op: " << op_name << "\n"
                << "           alpha: " << alpha_bon << "\n"
                << std::hexfloat << std::setprecision(13) << ""
                << "               a: " << helper::hexfloat(a) << "\n"
                << "               b: " << helper::hexfloat(b) << "\n"
                << "             a" << op_symbol
                << "b: " << helper::hexfloat(reference) << "\n"
                << std::defaultfloat << "" << "-- theoretical -\n"
                << "   probability ↓: " << fmt_proba(probability_down) << "\n"
                << "   probability ↑: " << fmt_proba(probability_up) << "\n"
                << "--- estimated --\n"
                << "     sample size: " << repetitions << "\n"
                << "              #↓: " << count_down << " ("
                << fmt_proba(probability_down_estimated) << ")\n"
                << "              #↑: " << count_up << " ("
                << fmt_proba(probability_up_estimated) << ")\n"
                << "         p-value: " << test.pvalue << "\n"
                << "            ↓ %: " << error_down * 100 << "\n"
                << "            ↑ %: " << error_up * 100 << "\n"
                << std::hexfloat
                << "              ↓: " << helper::hexfloat(counter.down())
                << "\n"
                << "              ↑: " << helper::hexfloat(counter.up()) << "\n"
                << distance_error_msg << std::defaultfloat << flush();
      HWY_ASSERT(0);
    }
    distribution_tests_counter++;
    lane++;
    debug_reset();
  }
}

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

template <class D, typename T = hn::TFromD<D>> void do_run_test_exact_add(D d) {
  constexpr int32_t mantissa = helper::IEEE754<T>::mantissa;

  for (int i = 0; i <= 5; i++) {
    auto va = hn::Set(d, 1.25);
    auto vb = hn::Set(d, std::ldexp(1.0, -(mantissa + i)));
    check_distribution_match<SRAdd>(d, va, vb);
  }
}

template <class Op, class D, class V = hn::VFromD<D>,
          typename T = hn::TFromD<D>>
void do_run_test_simple_case(D d) {
  auto simple_case = get_simple_case<T>();
  for (auto a : simple_case) {
    for (auto b : simple_case) {
      auto va = hn::Set(d, a);
      auto vb = hn::Set(d, b);
      auto va_neg = hn::Neg(va);
      auto vb_neg = hn::Neg(vb);
      check_distribution_match<Op>(d, va, vb);
      check_distribution_match<Op>(d, va, vb_neg);
      check_distribution_match<Op>(d, va_neg, vb);
      check_distribution_match<Op>(d, va_neg, vb_neg);
    }
  }
}

template <class Op, class D, class V = hn::VFromD<D>,
          typename T = hn::TFromD<D>>
void do_run_test_random(D d, const double start_range_1st = 0.0,
                        const double end_range_1st = 1.0,
                        const double start_range_2nd = 0.0,
                        const double end_range_2nd = 1.0) {
  helper::RNG rng1(start_range_1st, end_range_1st);
  helper::RNG rng2(start_range_2nd, end_range_2nd);

  for (int i = 0; i < 100; i++) {
    T a = rng1();
    T b = rng2();
    auto va = hn::Set(d, a);
    auto vb = hn::Set(d, b);
    auto va_neg = hn::Neg(va);
    auto vb_neg = hn::Neg(vb);
    check_distribution_match<Op>(d, va, vb);
    check_distribution_match<Op>(d, va, vb_neg);
    check_distribution_match<Op>(d, va_neg, vb);
    check_distribution_match<Op>(d, va_neg, vb_neg);
  }
}

void check_failed_tests(float threshold, bool skip = false) {
  const auto failed_ratio =
      distribution_failed_tests_counter / (double)distribution_tests_counter;
  if (failed_ratio > threshold) {
    std::cerr << "Number of failed tests above threshold!\n"
              << "Threshold:       " << threshold << "\n"
              << "Failed tests:    " << distribution_failed_tests_counter
              << " (" << failed_ratio << ")\n"
              << "Number of tests: " << distribution_tests_counter << "\n"
              << std::endl;
    HWY_ASSERT(skip);
  }
  distribution_failed_tests_counter = 0;
  distribution_tests_counter = 0;
}

void assert_exact() { check_failed_tests(0); }
void assert_almost_exact(bool skip = false) { check_failed_tests(0.05, skip); }

struct TestExactOperationsAdd {
  template <typename T, class D> void operator()(T /*unused*/, D d) {
    do_run_test_exact_add(d);
    assert_exact();
  }
};

template <class Op> struct TestBasicAssertions {
  template <typename T, class D> void operator()(T /*unused*/, D d) {
    do_run_test_simple_case<Op>(d);
    assert_almost_exact();
  }
};

using TestBasicAssertionsAdd = TestBasicAssertions<SRAdd>;
using TestBasicAssertionsSub = TestBasicAssertions<SRSub>;
using TestBasicAssertionsMul = TestBasicAssertions<SRMul>;
using TestBasicAssertionsDiv = TestBasicAssertions<SRDiv>;

HWY_NOINLINE void TestAllExactOperationsAdd() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestExactOperationsAdd>());
}

HWY_NOINLINE void TestAllBasicAssertionsAdd() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestBasicAssertionsAdd>());
}

HWY_NOINLINE void TestAllBasicAssertionsSub() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestBasicAssertionsSub>());
}

HWY_NOINLINE void TestAllBasicAssertionsMul() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestBasicAssertionsMul>());
}

HWY_NOINLINE void TestAllBasicAssertionsDiv() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestBasicAssertionsDiv>());
}

template <class Op> struct TestRandom01Assertions {
  template <typename T, class D> void operator()(T /*unused*/, D d) {
    do_run_test_random<Op>(d);
  }
};

using TestRandom01AssertionsAdd = TestRandom01Assertions<SRAdd>;
using TestRandom01AssertionsSub = TestRandom01Assertions<SRSub>;
using TestRandom01AssertionsMul = TestRandom01Assertions<SRMul>;
using TestRandom01AssertionsDiv = TestRandom01Assertions<SRDiv>;

HWY_NOINLINE void TestAllRandom01AssertionsAdd() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandom01AssertionsAdd>());
}

HWY_NOINLINE void TestAllRandom01AssertionsSub() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandom01AssertionsSub>());
}

HWY_NOINLINE void TestAllRandom01AssertionsMul() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandom01AssertionsMul>());
}

HWY_NOINLINE void TestAllRandom01AssertionsDiv() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandom01AssertionsDiv>());
}

template <class Op, class D, class V = hn::VFromD<D>,
          typename T = hn::TFromD<D>>
void do_random_no_overlap_test(D d) {
  double start_range_1st = 0.0;
  double end_range_1st = 1.0;
  int s2;
  if constexpr (std::is_same_v<T, float>) {
    s2 = -25;
  } else {
    s2 = -54;
  };
  double start_range_2nd = std::ldexp(1.0, s2 + 1);
  double end_range_2nd = std::ldexp(1.0, s2 + 1);
  do_run_test_random<Op>(d, start_range_1st, end_range_1st, start_range_2nd,
                         end_range_2nd);
}

template <class Op> struct TestRandomNoOverlapAssertions {
  template <typename T, class D> void operator()(T /*unused*/, D d) {
    do_random_no_overlap_test<Op>(d);
  }
};

using TestRandomNoOverlapAssertionsAdd = TestRandomNoOverlapAssertions<SRAdd>;
using TestRandomNoOverlapAssertionsSub = TestRandomNoOverlapAssertions<SRSub>;
using TestRandomNoOverlapAssertionsMul = TestRandomNoOverlapAssertions<SRMul>;
using TestRandomNoOverlapAssertionsDiv = TestRandomNoOverlapAssertions<SRDiv>;

HWY_NOINLINE void TestAllRandomNoOverlapAssertionsAdd() {
  hn::ForFloat3264Types(
      hn::ForPartialVectors<TestRandomNoOverlapAssertionsAdd>());
}

HWY_NOINLINE void TestAllRandomNoOverlapAssertionsSub() {
  hn::ForFloat3264Types(
      hn::ForPartialVectors<TestRandomNoOverlapAssertionsSub>());
}

HWY_NOINLINE void TestAllRandomNoOverlapAssertionsMul() {
  hn::ForFloat3264Types(
      hn::ForPartialVectors<TestRandomNoOverlapAssertionsMul>());
}

HWY_NOINLINE void TestAllRandomNoOverlapAssertionsDiv() {
  hn::ForFloat3264Types(
      hn::ForPartialVectors<TestRandomNoOverlapAssertionsDiv>());
}

template <class Op, class D, class V = hn::VFromD<D>,
          typename T = hn::TFromD<D>>
void do_random_last_bit_overlap(D d) {
  double start_range_1st = 1.0;
  double end_range_1st = 2.0;
  int s2;
  if constexpr (std::is_same_v<T, float>) {
    s2 = -24;
  } else {
    s2 = -53;
  }
  double start_range_2nd = std::ldexp(1.0, s2 + 1);
  double end_range_2nd = std::ldexp(1.0, s2 + 1);
  do_run_test_random<Op>(d, start_range_1st, end_range_1st, start_range_2nd,
                         end_range_2nd);
}

template <class Op> struct TestRandomLastBitOverlap {
  template <typename T, class D> void operator()(T /*unused*/, D d) {
    do_random_last_bit_overlap<Op>(d);
  }
};

using TestRandomLastBitOverlapAdd = TestRandomLastBitOverlap<SRAdd>;
using TestRandomLastBitOverlapSub = TestRandomLastBitOverlap<SRSub>;
using TestRandomLastBitOverlapMul = TestRandomLastBitOverlap<SRMul>;
using TestRandomLastBitOverlapDiv = TestRandomLastBitOverlap<SRDiv>;

HWY_NOINLINE void TestAllRandomLastBitOverlapAdd() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandomLastBitOverlapAdd>());
}

HWY_NOINLINE void TestAllRandomLastBitOverlapSub() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandomLastBitOverlapSub>());
}

HWY_NOINLINE void TestAllRandomLastBitOverlapMul() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandomLastBitOverlapMul>());
}

HWY_NOINLINE void TestAllRandomLastBitOverlapDiv() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandomLastBitOverlapDiv>());
}

template <class Op, class D, class V = hn::VFromD<D>,
          typename T = hn::TFromD<D>>
void do_random_mid_overlap_test(D d) {
  double start_range_1st = 1.0;
  double end_range_1st = 2.0;
  int s2;
  if constexpr (std::is_same_v<T, float>) {
    s2 = -13;
  } else {
    s2 = -27;
  };
  double start_range_2nd = std::ldexp(1.0, s2 + 1);
  double end_range_2nd = std::ldexp(1.0, s2 + 1);
  do_run_test_random<Op>(d, start_range_1st, end_range_1st, start_range_2nd,
                         end_range_2nd);
}

template <class Op> struct TestRandomMidOverlap {
  template <typename T, class D> void operator()(T /*unused*/, D d) {
    do_random_mid_overlap_test<Op>(d);
  }
};

using TestRandomMidOverlapAdd = TestRandomMidOverlap<SRAdd>;
using TestRandomMidOverlapSub = TestRandomMidOverlap<SRSub>;
using TestRandomMidOverlapMul = TestRandomMidOverlap<SRMul>;
using TestRandomMidOverlapDiv = TestRandomMidOverlap<SRDiv>;

HWY_NOINLINE void TestAllRandomMidOverlapAdd() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandomMidOverlapAdd>());
}

HWY_NOINLINE void TestAllRandomMidOverlapSub() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandomMidOverlapSub>());
}

HWY_NOINLINE void TestAllRandomMidOverlapMul() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandomMidOverlapMul>());
}

HWY_NOINLINE void TestAllRandomMidOverlapDiv() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandomMidOverlapDiv>());
}

} // namespace
} // namespace HWY_NAMESPACE
} // namespace sr
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace sr {
namespace HWY_NAMESPACE {

HWY_BEFORE_TEST(SRRoundTest);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllExactOperationsAdd);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllBasicAssertionsAdd);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllBasicAssertionsSub);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllBasicAssertionsMul);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllBasicAssertionsDiv);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandom01AssertionsAdd);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandom01AssertionsSub);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandom01AssertionsMul);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandom01AssertionsDiv);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomNoOverlapAssertionsAdd);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomNoOverlapAssertionsSub);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomNoOverlapAssertionsMul);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomNoOverlapAssertionsDiv);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomLastBitOverlapAdd);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomLastBitOverlapSub);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomLastBitOverlapMul);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomLastBitOverlapDiv);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomMidOverlapAdd);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomMidOverlapSub);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomMidOverlapMul);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllRandomMidOverlapDiv);
HWY_AFTER_TEST();

} // namespace HWY_NAMESPACE
} // namespace sr

HWY_TEST_MAIN();

#endif // HWY_ONCE

// int main(int argc, char **argv) {
//   testing::InitGoogleTest(&argc, argv);
//   init();
//   return RUN_ALL_TESTS();
// }