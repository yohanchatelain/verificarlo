#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "src/eft.hpp"
#include "src/main.hpp"
#include "src/sr.hpp"
#include "src/utils.hpp"
#include "tests/helper.hpp"

using ::testing::AllOf;
using ::testing::Ge;
using ::testing::Le;
using ::testing::Lt;

constexpr auto default_alpha = 0.001;
#ifdef DEBUG
constexpr auto default_repetitions = 10;
#else
constexpr auto default_repetitions = 10'000;
#endif
namespace reference {
// return pred(|s|)

// twosum reference
// compute in double precision if the input type is float
// compute in quad precision if the input type is double
template <typename T, typename H = typename helper::IEEE754<T>::H>
H sr_add(T a, T b) {
  return static_cast<H>(a) + static_cast<H>(b);
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
H sr_sub(T a, T b) {
  return static_cast<H>(a) - static_cast<H>(b);
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
H sr_mul(T a, T b) {
  return static_cast<H>(a) * static_cast<H>(b);
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
H sr_div(T a, T b) {
  return static_cast<H>(a) / static_cast<H>(b);
}

template <typename T, typename Op, typename H = typename helper::IEEE754<T>::H>
std::function<H(T, T)> get_operator() {
  static_assert(std::is_base_of_v<helper::Operator<T>, Op>);
  if constexpr (std::is_same_v<helper::AddOp<T>, Op>) {
    return sr_add<T>;
  }
  if constexpr (std::is_same_v<Op, helper::SubOp<T>>) {
    return sr_sub<T>;
  }
  if constexpr (std::is_same_v<Op, helper::MulOp<T>>) {
    return sr_mul<T>;
  }
  if constexpr (std::is_same_v<Op, helper::DivOp<T>>) {
    return sr_div<T>;
  }
}

}; // namespace reference

template <typename T, typename Op> typename Op::function get_target_operator() {
  static_assert(std::is_base_of_v<helper::Operator<T>, Op>);

  if constexpr (std::is_same_v<Op, helper::AddOp<T>>) {
    return sr_add<T>;
  }
  if constexpr (std::is_same_v<Op, helper::SubOp<T>>) {
    return sr_sub<T>;
  }
  if constexpr (std::is_same_v<Op, helper::MulOp<T>>) {
    return sr_mul<T>;
  }
  if constexpr (std::is_same_v<Op, helper::DivOp<T>>) {
    return sr_div<T>;
  }
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
std::pair<H, H> compute_distance_error(T a, T b, H reference) {
  T ref_cast = static_cast<T>(reference);

  H distance_to_fp =
      helper::absolute_distance(reference, static_cast<H>(ref_cast));
  H ulp = helper::get_ulp(ref_cast);

  H probability = distance_to_fp / ulp;
  H probability_c = 1 - probability;

  if (distance_to_fp == 0 or ulp == 0 or
      helper::abs(reference) < helper::IEEE754<T>::min_subnormal) {
    probability = 0;
    probability_c = 1;
  }

  if (ref_cast > reference)
    return {probability, probability_c};
  else
    return {probability_c, probability};
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
std::string compute_distance_error_str(T a, T b, H reference) {
  T ref_cast = static_cast<T>(reference);

  H distance_to_fp =
      helper::absolute_distance(reference, static_cast<H>(ref_cast));
  H ulp = helper::get_ulp(ref_cast);

  // TODO: fix corner case when probability is > 1
  H probability = distance_to_fp / ulp;
  H probability_c = 1 - probability;

  if (distance_to_fp == 0 or ulp == 0 or
      helper::abs(reference) < helper::IEEE754<T>::min_subnormal) {
    probability = 0;
    probability_c = 1;
  }

  H next, prev;
  if (reference + distance_to_fp == ref_cast) {
    next = ref_cast;
    prev = reference - distance_to_fp;
  } else {
    next = reference + distance_to_fp;
    prev = ref_cast;
  }

  std::ostringstream os;
  os << std::hexfloat << std::setprecision(13);
  os << "-- compute_distance_error --" << std::endl;
  os << "       reference: " << reference << std::endl;
  os << "  reference_cast: " << ref_cast << std::endl;
  os << "  distance_to_fp: " << distance_to_fp << std::endl;
  os << "             ulp: " << ulp << std::endl;
  os << "     previous_fp: " << prev << std::endl;
  os << "         next_fp: " << next << std::endl;
  os << std::defaultfloat;
  os << "     probability: " << probability << std::endl;
  os << "   probability_c: " << probability_c << std::endl;
  return os.str();
}

template <typename T, typename Op>
helper::Counter<T> eval_op(T a, T b, const int repetitions) {

  auto op = get_target_operator<T, Op>();
  helper::Counter<T> c;
  for (int i = 0; i < repetitions; i++) {
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

template <typename T> bool is_special(T a) {
  return std::isnan(a) or std::isinf(a);
}

template <typename T> bool isnan(const T &a) {
  if constexpr (std::is_same_v<T, Float128>) {
    return a.is_nan();
  } else {
    return std::isnan(a);
  }
}

template <typename T> bool isinf(const T &a) {
  if constexpr (std::is_same_v<T, Float128>) {
    return a.is_inf();
  } else {
    return std::isinf(a);
  }
}

template <typename T, typename Op>
void check_distribution_match(T a, T b,
                              const int repetitions = default_repetitions,
                              const float alpha = default_alpha) {
  using H = typename helper::IEEE754<T>::H;

  auto reference_op = reference::get_operator<T, Op>();
  H reference = reference_op(a, b);
  auto [probability_down, probability_up] =
      compute_distance_error(a, b, reference);

  auto counter = eval_op<T, Op>(a, b, repetitions);
  auto count_down = counter.down_count();
  auto count_up = counter.up_count();
  auto probability_down_estimated = count_down / (double)repetitions;
  auto probability_up_estimated = count_up / (double)repetitions;

  if (is_special(counter.up()) or is_special(counter.down())) {
    if (isnan(counter.up()) and isnan(reference))
      return;
    if (std::isnan(counter.down()) and isnan(reference))
      return;
    if (isinf(counter.up()) and isinf(reference)) {
      EXPECT_EQ(counter.up(), reference);
    }
    if (isinf(counter.down()) and isinf(reference)) {
      EXPECT_EQ(counter.down(), reference);
    }
    return;
  }

  EXPECT_THAT(static_cast<double>(probability_down), AllOf(Ge(0.0), Le(1.0)))
      << "Probability down is not in [0, 1] range\n"
      << "probability_down: " << probability_down << "\n"
      << "probability_up: " << probability_up << "\n"
      << "probability_down_estimated: " << probability_down_estimated << "\n"
      << "probability_up_estimated: " << probability_up_estimated << "\n"
      << "count_down: " << count_down << "\n"
      << "count_up: " << count_up << "\n"
      << "repetitions: " << repetitions << "\n"
      << compute_distance_error_str(a, b, reference);

  auto test = helper::binomial_test(repetitions, count_down,
                                    static_cast<double>(probability_down));

  const auto op_name = Op::name;
  const auto ftype = Op::ftype;

  if (test.pvalue == 0)
    return;

  EXPECT_THAT(alpha / 2, Lt(test.pvalue))
      << "Null hypotheis rejected!\n"
      << "            type: " << ftype << "\n"
      << "              op: " << op_name << "\n"
      << "           alpha: " << alpha << "\n"
      << std::hexfloat << std::setprecision(13) << ""
      << "               a: " << a << "\n"
      << "               b: " << b << "\n"
      << "             a+b: " << reference << "\n"
      << std::defaultfloat << ""
      << "-- theoretical -\n"
      << "   probability ↓: " << fmt_proba(probability_down) << "\n"
      << "   probability ↑: " << fmt_proba(probability_up) << "\n"
      << "--- estimated --\n"
      << "     sample size: " << repetitions << "\n"
      << "              #↓: " << count_down << " ("
      << fmt_proba(probability_down_estimated) << ")\n"
      << "              #↑: " << count_up << " ("
      << fmt_proba(probability_up_estimated) << ")\n"
      << "         p-value: " << test.pvalue << "\n"
      << std::hexfloat << ""
      << "              ↓: " << counter.down() << "\n"
      << "              ↑: " << counter.up() << "\n"
      << compute_distance_error_str(a, b, reference) << std::defaultfloat;
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

template <typename T, typename Op> void do_run_test_random01() {
  helper::RNG rng;

  for (int i = 0; i < 1000; i++) {
    T a = rng();
    T b = rng();
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
  do_run_test_random01<float, helper::AddOp<float>>();
  do_run_test_random01<double, helper::AddOp<double>>();
}

// TEST(GetTwoSumTest, Random01Assertions) {
//   RNG rng;

//   for (int i = 0; i < 1000; i++) {
//     float a = rng();
//     float b = rng();
//     test_equality(a, b);
//     test_equality(a, -b);
//     test_equality(-a, b);
//     test_equality(-a, -b);
//   }

//   for (int i = 0; i < 1000; i++) {
//     double a = rng();
//     double b = rng();
//     test_equality(a, b);
//     test_equality(a, -b);
//     test_equality(-a, b);
//     test_equality(-a, -b);
//   }
// }

// TEST(GetTwoSumTest, RandomNoOverlapAssertions) {
//   RNG rng_0v1_float(0, 1);
//   RNG rng_24v25_float(0x1p24, 0x1p25);

//   for (int i = 0; i < 1000; i++) {
//     float a = rng_0v1_float();
//     float b = rng_24v25_float();
//     test_equality(a, b);
//     test_equality(a, -b);
//     test_equality(-a, b);
//     test_equality(-a, -b);
//   }

//   RNG rng_0v1_double(0, 1);
//   RNG rng_53v54_double(0x1p53, 0x1p54);

//   for (int i = 0; i < 1000; i++) {
//     double a = rng_0v1_double();
//     double b = rng_53v54_double();
//     test_equality(a, b);
//     test_equality(a, -b);
//     test_equality(-a, b);
//     test_equality(-a, -b);
//   }
// }

// TEST(GetTwoSumTest, RandomLastBitOverlapAssertions) {
//   RNG rng_1v2_float(1, 2);
//   RNG rng_23v24_float(0x1p23, 0x1p24);

//   for (int i = 0; i < 1000; i++) {
//     float a = rng_1v2_float();
//     float b = rng_23v24_float();
//     test_equality(a, b);
//     test_equality(a, -b);
//     test_equality(-a, b);
//     test_equality(-a, -b);
//   }

//   RNG rng_1v2_double(1, 2);
//   RNG rng_52v53_double(0x1p52, 0x1p53);

//   for (int i = 0; i < 1000; i++) {
//     double a = rng_1v2_double();
//     double b = rng_52v53_double();
//     test_equality(a, b);
//     test_equality(a, -b);
//     test_equality(-a, b);
//     test_equality(-a, -b);
//   }
// }

// TEST(GetTwoSumTest, RandomMidOverlapAssertions) {
//   RNG rng_0v1_float(0, 1);
//   RNG rng_12v13_float(0x1p12, 0x1p13);

//   for (int i = 0; i < 1000; i++) {
//     float a = rng_0v1_float();
//     float b = rng_12v13_float();
//     test_equality(a, b);
//     test_equality(a, -b);
//     test_equality(-a, b);
//     test_equality(-a, -b);
//   }

//   RNG rng_0v1_double(0, 1);
//   RNG rng_26v27_double(0x1p26, 0x1p27);

//   for (int i = 0; i < 1000; i++) {
//     double a = rng_0v1_double();
//     double b = rng_26v27_double();
//     test_equality(a, b);
//     test_equality(a, -b);
//     test_equality(-a, b);
//     test_equality(-a, -b);
//   }
// }

// TEST(GetTwoSumTest, BinadeAssertions) {
//   for (int i = -126; i < 127; i++) {
//     testBinade<float>(i);
//     testBinade<double>(i);
//   }
// }

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  init();
  return RUN_ALL_TESTS();
}