#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "src/eft.hpp"
#include "src/sr.h"
#include "src/utils.hpp"
#include "tests/helper.hpp"

using ::testing::AllOf;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Le;
using ::testing::Lt;

constexpr auto default_alpha = 0.001;
#ifdef SR_DEBUG
constexpr auto default_repetitions = 10;
#else
constexpr auto default_repetitions = 100'000;
#endif
namespace reference {
// return pred(|s|)

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

template <typename T, typename Op, typename H = typename helper::IEEE754<T>::H>
std::function<H(T, T)> get_operator() {
  static_assert(std::is_base_of_v<helper::Operator<T>, Op>);
  if constexpr (std::is_same_v<helper::AddOp<T>, Op>) {
    return add<T>;
  }
  if constexpr (std::is_same_v<Op, helper::SubOp<T>>) {
    return sub<T>;
  }
  if constexpr (std::is_same_v<Op, helper::MulOp<T>>) {
    return mul<T>;
  }
  if constexpr (std::is_same_v<Op, helper::DivOp<T>>) {
    return div<T>;
  }
}

}; // namespace reference

template <typename T, typename Op> typename Op::function get_target_operator() {
  static_assert(std::is_base_of_v<helper::Operator<T>, Op>);

  if constexpr (std::is_same_v<Op, helper::AddOp<T>>) {
    return sr::scalar::add<T>;
  }
  if constexpr (std::is_same_v<Op, helper::SubOp<T>>) {
    return sr::scalar::sub<T>;
  }
  if constexpr (std::is_same_v<Op, helper::MulOp<T>>) {
    return sr::scalar::mul<T>;
  }
  if constexpr (std::is_same_v<Op, helper::DivOp<T>>) {
    return sr::scalar::div<T>;
  }
}

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

template <typename T, typename Op>
helper::Counter<T> eval_op(T a, T b, const int repetitions) {

  auto op = get_target_operator<T, Op>();
  helper::Counter<T> c;
  for (int i = 0; i < repetitions; i++) {
    debug_print("*** repetition %d ***\n", i);
    T v = op(a, b);
    c[v]++;
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

template <typename T, typename Op>
void check_distribution_match(T a, T b,
                              const long repetitions = default_repetitions,
                              const float alpha = default_alpha) {
  using H = typename helper::IEEE754<T>::H;

  const auto op_name = Op::name;
  const auto ftype = Op::ftype;

  auto reference_op = reference::get_operator<T, Op>();
  H reference = reference_op(a, b);
  auto [probability_down, probability_up, down, up, distance_error,
        distance_error_msg] = compute_distance_error(a, b, reference);

  auto counter = eval_op<T, Op>(a, b, repetitions);
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
      EXPECT_EQ(counter.up(), reference);
    }
    if (helper::isinf(counter.down()) and helper::isinf(reference)) {
      EXPECT_EQ(counter.down(), reference);
    }
    return;
  }

  EXPECT_THAT(static_cast<double>(probability_down), AllOf(Ge(0.0), Le(1.0)))
      << "Probability ↓ is not in [0, 1] range\n"
      << "-- theoretical -\n"
      << "   probability ↓: " << fmt_proba(probability_down) << "\n"
      << "   probability ↑: " << fmt_proba(probability_up) << "\n"
      << "--- estimated --\n"
      << "     sample size: " << repetitions << "\n"
      << "              #↓: " << count_down << " ("
      << fmt_proba(probability_down_estimated) << ")\n"
      << "              #↑: " << count_up << " ("
      << fmt_proba(probability_up_estimated) << ")\n"
      << std::hexfloat << "" << "              ↓: " << counter.down() << "\n"
      << "              ↑: " << counter.up() << "\n"
      << distance_error_msg << std::defaultfloat << flush();

  // do the test only if the operation is not exact (probability is not zero)
  bool is_exact = probability_down == 1 and probability_up == 1;

  // do
  // do not the test if the probability is lower than 1/repetitions
  bool compare_down_values = not is_exact and down != 0 and
                             probability_down > (1.0 / repetitions) and
                             distance_error > helper::IEEE754<T>::min_subnormal;
  bool compare_up_values = not is_exact and up != 0 and
                           probability_up > (1.0 / repetitions) and
                           distance_error > helper::IEEE754<T>::min_subnormal;

  if (not is_exact and down != 0 and probability_down > (1.0 / repetitions) and
      distance_error > helper::IEEE754<T>::min_subnormal)
    EXPECT_THAT(counter.down(), Eq(static_cast<T>(down)))
        << "Value ↓ is not equal to reference\n"
        << "            type: " << ftype << "\n"
        << "              op: " << op_name << "\n"
        << "           alpha: " << alpha << "\n"
        << std::hexfloat << std::setprecision(13) << ""
        << "               a: " << helper::hexfloat(a) << "\n"
        << "               b: " << helper::hexfloat(b) << "\n"
        << "             a+b: " << helper::hexfloat(reference) << "\n"
        << std::defaultfloat << "" << "-- theoretical -\n"
        << "   probability ↓: " << fmt_proba(probability_down) << "\n"
        << "   probability ↑: " << fmt_proba(probability_up) << "\n"
        << "--- estimated --\n"
        << "     sample size: " << repetitions << "\n"
        << "              #↓: " << count_down << " ("
        << fmt_proba(probability_down_estimated) << ")\n"
        << "              #↑: " << count_up << " ("
        << fmt_proba(probability_up_estimated) << ")\n"
        << std::hexfloat << ""
        << "              ↓: " << helper::hexfloat(counter.down()) << "\n"
        << "              ↑: " << helper::hexfloat(counter.up()) << "\n"
        << distance_error_msg << std::defaultfloat << flush();

  // do not the test if the probability is lower than 1/repetitions
  if (not is_exact and up != 0 and probability_up > (1.0 / repetitions) and
      distance_error > helper::IEEE754<T>::min_subnormal)
    EXPECT_THAT(counter.up(), Eq(static_cast<T>(up)))
        << "Value ↑ is not equal to reference\n"
        << "            type: " << ftype << "\n"
        << "              op: " << op_name << "\n"
        << "           alpha: " << alpha << "\n"
        << std::hexfloat << std::setprecision(13) << ""
        << "               a: " << helper::hexfloat(a) << "\n"
        << "               b: " << helper::hexfloat(b) << "\n"
        << "             a+b: " << helper::hexfloat(reference) << "\n"
        << std::defaultfloat << "" << "-- theoretical -\n"
        << "   probability ↓: " << fmt_proba(probability_down) << "\n"
        << "   probability ↑: " << fmt_proba(probability_up) << "\n"
        << "--- estimated --\n"
        << "     sample size: " << repetitions << "\n"
        << "              #↓: " << count_down << " ("
        << fmt_proba(probability_down_estimated) << ")\n"
        << "              #↑: " << count_up << " ("
        << fmt_proba(probability_up_estimated) << ")\n"
        << std::hexfloat << ""
        << "              ↓: " << helper::hexfloat(counter.down()) << "\n"
        << "              ↑: " << helper::hexfloat(counter.up()) << "\n"
        << distance_error_msg << std::defaultfloat << flush();

  // 95% relative error
  auto error_down = 0.95 *
                    helper::abs(probability_down - probability_down_estimated) /
                    probability_down;
  auto error_up = 0.95 *
                  helper::abs(probability_up - probability_up_estimated) /
                  probability_up;

  // binomial test
  auto test = helper::binomial_test(repetitions, count_down,
                                    static_cast<double>(probability_down));

  // check probability if we compare the values
  if (compare_down_values and compare_up_values)
    EXPECT_THAT(alpha / 2, Lt(test.pvalue))
        << "Null hypotheis rejected!\n"
        << "            type: " << ftype << "\n"
        << "              op: " << op_name << "\n"
        << "           alpha: " << alpha << "\n"
        << std::hexfloat << std::setprecision(13) << ""
        << "               a: " << helper::hexfloat(a) << "\n"
        << "               b: " << helper::hexfloat(b) << "\n"
        << "             a+b: " << helper::hexfloat(reference) << "\n"
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
        << "            ↓ %: " << error_down << "\n"
        << "            ↑ %: " << error_up << "\n"
        << std::hexfloat << ""
        << "              ↓: " << helper::hexfloat(counter.down()) << "\n"
        << "              ↑: " << helper::hexfloat(counter.up()) << "\n"
        << distance_error_msg << std::defaultfloat << flush();
  debug_reset();
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

template <typename T> void do_run_test_exact_add() {
  constexpr int32_t mantissa = helper::IEEE754<T>::mantissa;

  T a = 1.25f;
  T b;
  T p;

  for (int i = 0; i <= 5; i++) {
    b = std::ldexp(1.0, -(mantissa + i));
    p = 1 - (1.0 / (1 << i));
    check_distribution_match<T, helper::AddOp<T>>(a, b);
  }
}

template <typename T, typename Op> void do_run_test_simple_case() {
  auto simple_case = get_simple_case<T>();
  for (auto a : simple_case) {
    for (auto b : simple_case) {
      check_distribution_match<T, Op>(a, b);
      check_distribution_match<T, Op>(a, -b);
      check_distribution_match<T, Op>(-a, b);
      check_distribution_match<T, Op>(-a, -b);
    }
  }
}

template <typename T, typename Op>
void do_run_test_random(const double start_range_1st = 0.0,
                        const double end_range_1st = 1.0,
                        const double start_range_2nd = 0.0,
                        const double end_range_2nd = 1.0) {
  helper::RNG rng1(start_range_1st, end_range_1st);
  helper::RNG rng2(start_range_2nd, end_range_2nd);

  for (int i = 0; i < 100; i++) {
    T a = rng1();
    T b = rng2();
    check_distribution_match<T, Op>(a, b);
    check_distribution_match<T, Op>(a, -b);
    check_distribution_match<T, Op>(-a, b);
    check_distribution_match<T, Op>(-a, -b);
  }
}

TEST(SRRoundTest, ExactOperationsAdd) {
  do_run_test_exact_add<float>();
  do_run_test_exact_add<double>();
}

TEST(SRRoundTest, BasicAssertionsAdd) {
  do_run_test_simple_case<float, helper::AddOp<float>>();
  do_run_test_simple_case<double, helper::AddOp<double>>();
}

TEST(SRRoundTest, BasicAssertionsSub) {
  do_run_test_simple_case<float, helper::SubOp<float>>();
  do_run_test_simple_case<double, helper::SubOp<double>>();
}

TEST(SRRoundTest, BasicAssertionsMul) {
  do_run_test_simple_case<float, helper::MulOp<float>>();
  do_run_test_simple_case<double, helper::MulOp<double>>();
}

TEST(SRRoundTest, BasicAssertionsDiv) {
  do_run_test_simple_case<float, helper::DivOp<float>>();
  do_run_test_simple_case<double, helper::DivOp<double>>();
}

TEST(SRRoundTest, Random01AssertionsAdd) {
  do_run_test_random<float, helper::AddOp<float>>();
  do_run_test_random<double, helper::AddOp<double>>();
}

TEST(SRRoundTest, Random01AssertionsSub) {
  do_run_test_random<float, helper::AddOp<float>>();
  do_run_test_random<double, helper::SubOp<double>>();
}

TEST(SRRoundTest, Random01AssertionsMul) {
  do_run_test_random<float, helper::MulOp<float>>();
  do_run_test_random<double, helper::MulOp<double>>();
}

TEST(SRRoundTest, Random01AssertionsDiv) {
  do_run_test_random<float, helper::DivOp<float>>();
  do_run_test_random<double, helper::DivOp<double>>();
}

template <typename T, typename Op> void do_random_no_overlap_test() {
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
  do_run_test_random<T, Op>(start_range_1st, end_range_1st, start_range_2nd,
                            end_range_2nd);
}

//
TEST(SRRoundTest, RandomNoOverlapAssertionsAdd) {
  using opf = helper::AddOp<float>;
  using opd = helper::AddOp<double>;
  do_random_no_overlap_test<float, opf>();
  do_random_no_overlap_test<double, opd>();
}

TEST(SRRoundTest, RandomNoOverlapAssertionsSub) {
  using opf = helper::SubOp<float>;
  using opd = helper::SubOp<double>;
  do_random_no_overlap_test<float, opf>();
  do_random_no_overlap_test<double, opd>();
}

TEST(SRRoundTest, RandomNoOverlapAssertionsMul) {
  using opf = helper::MulOp<float>;
  using opd = helper::MulOp<double>;
  do_random_no_overlap_test<float, opf>();
  do_random_no_overlap_test<double, opd>();
}

TEST(SRRoundTest, RandomNoOverlapAssertionsDiv) {
  using opf = helper::DivOp<float>;
  using opd = helper::DivOp<double>;
  do_random_no_overlap_test<float, opf>();
  do_random_no_overlap_test<double, opd>();
}

template <typename T, typename Op> void do_random_last_bit_overlap() {
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
  do_run_test_random<T, Op>(start_range_1st, end_range_1st, start_range_2nd,
                            end_range_2nd);
}

TEST(SRRoundTest, RandomLastBitOverlapAddAssertions) {
  using opf = helper::AddOp<float>;
  using opd = helper::AddOp<double>;
  do_random_last_bit_overlap<float, opf>();
  do_random_last_bit_overlap<double, opd>();
}

TEST(SRRoundTest, RandomLastBitOverlapSubAssertions) {
  using opf = helper::SubOp<float>;
  using opd = helper::SubOp<double>;
  do_random_last_bit_overlap<float, opf>();
  do_random_last_bit_overlap<double, opd>();
}

TEST(SRRoundTest, RandomLastBitOverlapMulAssertions) {
  using opf = helper::MulOp<float>;
  using opd = helper::MulOp<double>;
  do_random_last_bit_overlap<float, opf>();
  do_random_last_bit_overlap<double, opd>();
}

TEST(SRRoundTest, RandomLastBitOverlapDivAssertions) {
  using opf = helper::DivOp<float>;
  using opd = helper::DivOp<double>;
  do_random_last_bit_overlap<float, opf>();
  do_random_last_bit_overlap<double, opd>();
}

template <typename T, typename Op> void do_random_mid_overlap_test() {
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
  do_run_test_random<T, Op>(start_range_1st, end_range_1st, start_range_2nd,
                            end_range_2nd);
}

TEST(SRRoundTest, RandomMidOverlapAddAssertions) {
  using opf = helper::AddOp<float>;
  using opd = helper::AddOp<double>;
  do_random_mid_overlap_test<float, opf>();
  do_random_mid_overlap_test<double, opd>();
}

TEST(SRRoundTest, RandomMidOverlapSubAssertions) {
  using opf = helper::SubOp<float>;
  using opd = helper::SubOp<double>;
  do_random_mid_overlap_test<float, opf>();
  do_random_mid_overlap_test<double, opd>();
}

TEST(SRRoundTest, RandomMidOverlapMulAssertions) {
  using opf = helper::MulOp<float>;
  using opd = helper::MulOp<double>;
  do_random_mid_overlap_test<float, opf>();
  do_random_mid_overlap_test<double, opd>();
}

TEST(SRRoundTest, RandomMidOverlapDivAssertions) {
  using opf = helper::DivOp<float>;
  using opd = helper::DivOp<double>;
  do_random_mid_overlap_test<float, opf>();
  do_random_mid_overlap_test<double, opd>();
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}