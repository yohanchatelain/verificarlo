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
#define HWY_TARGET_INCLUDE "tests/test_sr_hwy_accuracy.cpp"
#include "hwy/foreach_target.h"

#include "hwy/highway.h"
#include "hwy/base.h"
#include "hwy/tests/test_util-inl.h"
#include "src/debug_hwy-inl.h"
#include "src/sr_hw-inl.h"
// clang-format on

#include "src/sr_hw.h"
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

constexpr auto default_alpha = 0.001;
#ifdef SR_DEBUG
constexpr auto default_repetitions = 100;
#else
constexpr auto default_repetitions = 10'000;
#endif

static auto distribution_failed_tests_counter = 0;
static auto distribution_tests_counter = 0;

namespace reference {
// return pred(|s|)

// twosum reference
// compute in double precision if the input type is float
// compute in quad precision if the input type is double
template <typename T, typename H = typename helper::IEEE754<T>::H>
H add(const std::vector<T> &args) {
  auto a = static_cast<H>(args[0]);
  auto b = static_cast<H>(args[1]);
  return a + b;
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
H sub(const std::vector<T> &args) {
  auto a = static_cast<H>(args[0]);
  auto b = static_cast<H>(args[1]);
  return a - b;
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
H mul(const std::vector<T> &args) {
  auto a = static_cast<H>(args[0]);
  auto b = static_cast<H>(args[1]);
  return a * b;
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
H div(const std::vector<T> &args) {
  auto a = static_cast<H>(args[0]);
  auto b = static_cast<H>(args[1]);
  return a / b;
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
H sqrt(const std::vector<T> &args) {
  auto a = static_cast<H>(args[0]);
  return helper::sqrt<H>(a);
}

template <typename T, typename H = typename helper::IEEE754<T>::H>
H fma(const std::vector<T> &args) {
  auto a = static_cast<H>(args[0]);
  auto b = static_cast<H>(args[1]);
  auto c = static_cast<H>(args[2]);
  return helper::fma<H>(a, b, c);
}

}; // namespace reference

namespace srvh = sr::vector::HWY_NAMESPACE;

struct SRAdd {
  static constexpr char name[] = "add";
  static constexpr char symbol[] = "+";
  static constexpr int arity = 2;

  template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
  V operator()(D d, V a, V b) {
    return srvh::sr_add<D>(a, b);
  }

  template <typename T, typename H = typename helper::IEEE754<T>::H>
  static H reference(const std::vector<T> &args) {
    return reference::add<T>(args);
  }
};

struct SRSub {
  static constexpr char name[] = "sub";
  static constexpr char symbol[] = "-";
  static constexpr int arity = 2;

  template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
  V operator()(D d, V a, V b) {
    return srvh::sr_sub<D>(a, b);
  }

  template <typename T, typename H = typename helper::IEEE754<T>::H>
  static H reference(const std::vector<T> &args) {
    return reference::sub<T>(args);
  }
};

struct SRMul {
  static constexpr char name[] = "mul";
  static constexpr char symbol[] = "*";
  static constexpr int arity = 2;

  template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
  V operator()(D d, V a, V b) {
    return srvh::sr_mul<D>(a, b);
  }

  template <typename T, typename H = typename helper::IEEE754<T>::H>
  static H reference(const std::vector<T> &args) {
    return reference::mul<T>(args);
  }
};

struct SRDiv {
  static constexpr char name[] = "div";
  static constexpr char symbol[] = "/";
  static constexpr int arity = 2;

  template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
  V operator()(D d, V a, V b) {
    return srvh::sr_div<D>(a, b);
  }

  template <typename T, typename H = typename helper::IEEE754<T>::H>
  static H reference(const std::vector<T> &args) {
    return reference::div<T>(args);
  }
};

struct SRSqrt {
  static constexpr char name[] = "sqrt";
  static constexpr char symbol[] = "√";
  static constexpr int arity = 1;

  template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
  V operator()(D d, V a) {
    return srvh::sr_sqrt<D>(a);
  }

  template <typename T, typename H = typename helper::IEEE754<T>::H>
  static H reference(const std::vector<T> &args) {
    return reference::sqrt<T>(args);
  }
};

struct SRFma {
  static constexpr char name[] = "fma";
  static constexpr char symbol[] = "fma";
  static constexpr int arity = 3;

  template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
  V operator()(D d, V a, V b, V c) {
    return srvh::sr_fma<D>(a, b, c);
  }

  template <typename T, typename H = typename helper::IEEE754<T>::H>
  static H reference(const std::vector<T> &args) {
    return reference::fma<T>(args);
  }
};

template <typename T, typename H = typename helper::IEEE754<T>::H>
std::tuple<H, H, H, H, H, std::string>
compute_distance_error(const std::vector<T> &args, H reference) {
  T ref_cast = static_cast<T>(reference);

  bool is_finite = true;
  for (const auto &a : args) {
    if (helper::isnan(a) or helper::isinf(a)) {
      is_finite = false;
      break;
    }
  }

  bool is_nan = helper::isnan(reference) or helper::isnan(ref_cast);
  bool is_inf = helper::isinf(reference) or helper::isinf(ref_cast);

  if (not is_finite or is_inf or is_nan) {
    return {0, 0, reference, reference, 0, ""};
  }

  H error = 0, error_c = 0, rel_error = 0;
  H probability_down = 0, probability_up = 0;
  H next = 0, prev = 0;
  H ulp = helper::get_ulp(ref_cast);

  if (ref_cast == reference) {
    error = 0;
    rel_error = 0;
    probability_down = 1;
    probability_up = 1;
    prev = reference;
    next = reference;
  } else {
    error = helper::absolute_distance(reference, static_cast<H>(ref_cast));
    rel_error = helper::relative_distance(reference, static_cast<H>(ref_cast));
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

  int exponent_next = helper::get_exponent(next);
  int exponent_prev = helper::get_exponent(prev);

  if (rel_error < helper::IEEE754<T>::ulp) {
    probability_down = 1;
    probability_up = 1;
    prev = reference;
    next = reference;
  } else if (exponent_next != exponent_prev and
             probability_down != probability_up) {
    // if the distance between the reference and the casted reference is
    // exactly
    // a power of 2, and the reference and the casted reference does not
    // belong
    // to the same binade, then the probability of the casted reference
    // being the next representable value is equal to 0.5, not .75/.25
    probability_down = 0.5;
    probability_up = 0.5;
    H ulp_prev = (exponent_next < exponent_prev) ? ulp : (ulp / 2);
    H ulp_next = (exponent_next < exponent_prev) ? (ulp / 2) : ulp;
    prev = (ref_cast < reference) ? ref_cast : (ref_cast - ulp_prev);
    next = (ref_cast < reference) ? (ref_cast + ulp_next) : ref_cast;
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
  os << "         rel_error: " << helper::hexfloat(rel_error) << std::endl;
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
std::vector<helper::Counter<T>> eval_op(D d, V a, V x, V y,
                                        const int repetitions) {

#ifdef SR_DEBUG
  const size_t lanes = 1;
#else
  const size_t lanes = hn::Lanes(d);
#endif
  auto op = Op();

  std::vector<helper::Counter<T>> c(lanes);
  V v;
  for (int i = 0; i < repetitions; i++) {
    if constexpr (Op::arity == 1) {
      v = op(d, a);
    } else if constexpr (Op::arity == 2) {
      v = op(d, a, x);
    } else if constexpr (Op::arity == 3) {
      v = op(d, a, x, y);
    }
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

template <typename T, typename Op, typename H = typename helper::IEEE754<T>::H>
std::string get_args_str(const std::vector<T> &args, H reference) {
  const auto symbol = std::string(Op::symbol);

  std::string symbol_op = "";
  std::string args_str = "";
  if constexpr (Op::arity == 2) {
    auto a = args[0];
    auto b = args[1];
    symbol_op = "             a" + symbol + "b: ";
    args_str = "               a: " + helper::hexfloat(a) + "\n" +
               "               b: " + helper::hexfloat(b) + "\n" + symbol_op +
               helper::hexfloat(reference) + "\n";
  } else if constexpr (Op::arity == 1) {
    auto a = args[0];
    symbol_op = "     " + symbol + "a: ";
    args_str = "               a: " + helper::hexfloat(a) + "\n" + symbol_op +
               helper::hexfloat(reference) + "\n";
  } else if constexpr (Op::arity == 3) {
    auto a = args[0];
    auto b = args[1];
    auto c = args[2];
    symbol_op = "     " + symbol + "(a, b, c): ";
    args_str = "               a: " + helper::hexfloat(a) + "\n" +
               "               b: " + helper::hexfloat(b) + "\n" +
               "               c: " + helper::hexfloat(c) + "\n" + symbol_op +
               helper::hexfloat(reference) + "\n";
  }
  return args_str;
}

template <class Op, class D, class V = hn::VFromD<D>,
          typename T = hn::TFromD<D>>
void check_distribution_match(D d, V va, V vb = hn::Zero(D{}),
                              V vc = hn::Zero(D{}),
                              const long repetitions = default_repetitions,
                              const float alpha = default_alpha) {
  using H = typename helper::IEEE754<T>::H;

  auto a = hn::GetLane(va);
  auto b = hn::GetLane(vb);
  auto c = hn::GetLane(vc);

  const std::vector<T> args = {a, b, c};

  // ensure that we have vector of same value
  assert_equal_inputs(d, va);
  assert_equal_inputs(d, vb);
  assert_equal_inputs(d, vc);

  auto counters = eval_op<Op>(d, va, vb, vc, repetitions);

  H reference = Op::reference(args);
  auto [probability_down, probability_up, down, up, distance_error,
        distance_error_msg] = compute_distance_error(args, reference);

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
    const auto args_str = get_args_str<T, Op>(args, reference);

    // binomial test
    auto test = helper::binomial_test(repetitions, count_down,
                                      static_cast<double>(probability_down));

    const auto op_name = Op::name;
    const auto op_symbol = Op::symbol;
    const auto ftype = typeid(T).name();

    // Bonferroni correction, divide by the number of lanes
    const auto lanes = hn::Lanes(d);
    const auto alpha_bon = (alpha / 2) / lanes;

    // do the test only if the operation is not exact (probability is not zero)
    // do the test if the distance between the reference and the estimated value
    // is greater than the minimum subnormal
    // do not the test if the probability is lower than 1/repetitions
    bool estimate_unique_value =
        ((probability_down_estimated == 1) and (probability_down != 1)) or
        ((probability_up_estimated == 1) and (probability_up != 1)) and
            distance_error > helper::IEEE754<T>::min_subnormal;

    if (estimate_unique_value) {
      const auto cond1 =
          (counter.down() != 0 and (counter.down() != static_cast<T>(down)) and
           (counter.down() != static_cast<T>(up)));
      const auto cond2 =
          (counter.up() != 0 and (counter.up() != static_cast<T>(down)) and
           (counter.up() != static_cast<T>(up)));
      const auto conditions = cond1 or cond2;
      if (conditions) {
        std::cerr << "Value ↓ is not equal to reference\n"
                  << "            type: " << ftype << "\n"
                  << "              op: " << op_name << "\n"
                  << "           alpha: " << alpha << "\n"
                  << std::hexfloat << std::setprecision(13) << args_str
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
                  << "              ↓: " << helper::hexfloat(counter.down())
                  << "\n"
                  << "              ↑: " << helper::hexfloat(counter.up())
                  << "\n"
                  << distance_error_msg << std::defaultfloat << flush();
        HWY_ASSERT(false);
      }
    } else {
      if (compare_down_values and static_cast<T>(down) != counter.down()) {
        std::cerr << "Value ↓ is not equal to reference\n"
                  << "     Lane/#Lanes: " << lane + 1 << "/" << lanes << "\n"
                  << "            type: " << ftype << "\n"
                  << "              op: " << op_name << "\n"
                  << "           alpha: " << alpha_bon << "\n"
                  << std::hexfloat << std::setprecision(13) << args_str
                  << std::defaultfloat << "" << "-- theoretical -\n"
                  << "   probability ↓: " << fmt_proba(probability_down) << "\n"
                  << "   probability ↑: " << fmt_proba(probability_up) << "\n"
                  << "--- estimated --\n"
                  << "     sample size: " << repetitions << "\n"
                  << "              #↓: " << count_down << " ("
                  << fmt_proba(probability_down_estimated) << ")\n"
                  << "              #↑: " << count_up << " ("
                  << fmt_proba(probability_up_estimated) << ")\n"
                  << std::hexfloat
                  << "              ↓: " << helper::hexfloat(counter.down())
                  << "\n"
                  << "              ↑: " << helper::hexfloat(counter.up())
                  << "\n"
                  << distance_error_msg << std::defaultfloat << flush();
        HWY_ASSERT(0);
      }

      if (compare_up_values and static_cast<T>(up) != counter.up()) {
        std::cerr << "Value ↑ is not equal to reference\n"
                  << "     Lane/#Lanes: " << lane + 1 << "/" << lanes << "\n"
                  << "            type: " << ftype << "\n"
                  << "              op: " << op_name << "\n"
                  << "           alpha: " << alpha_bon << "\n"
                  << std::hexfloat << std::setprecision(13) << args_str
                  << std::defaultfloat << "" << "-- theoretical -\n"
                  << "   probability ↓: " << fmt_proba(probability_down) << "\n"
                  << "   probability ↑: " << fmt_proba(probability_up) << "\n"
                  << "--- estimated --\n"
                  << "     sample size: " << repetitions << "\n"
                  << "              #↓: " << count_down << " ("
                  << fmt_proba(probability_down_estimated) << ")\n"
                  << "              #↑: " << count_up << " ("
                  << fmt_proba(probability_up_estimated) << ")\n"
                  << std::hexfloat
                  << "              ↓: " << helper::hexfloat(counter.down())
                  << "\n"
                  << "              ↑: " << helper::hexfloat(counter.up())
                  << "\n"
                  << distance_error_msg << std::defaultfloat << flush();
        HWY_ASSERT(0);
      }
    }

    // if (test.pvalue == 0)
    //   return;

    if (compare_down_values and compare_up_values and test.pvalue < alpha_bon) {
      std::cerr << "Null hypotheis rejected!\n"
                << "     Lane/#Lanes: " << lane + 1 << "/" << lanes << "\n"
                << "            type: " << ftype << "\n"
                << "              op: " << op_name << "\n"
                << "           alpha: " << alpha_bon << "\n"
                << std::hexfloat << std::setprecision(13) << args_str
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

  if constexpr (Op::arity == 1) {
    for (auto a : simple_case) {
      auto va = hn::Set(d, a);
      check_distribution_match<Op>(d, va);
    }
  } else if constexpr (Op::arity == 2) {
    for (auto a : simple_case) {
      for (auto b : simple_case) {
        auto va = hn::Set(d, a);
        auto vb = hn::Set(d, b);
        check_distribution_match<Op>(d, va, vb);
        check_distribution_match<Op>(d, va, hn::Neg(vb));
      }
    }
  } else if constexpr (Op::arity == 3) {
    for (auto a : simple_case) {
      for (auto b : simple_case) {
        for (auto c : simple_case) {
          auto va = hn::Set(d, a);
          auto vb = hn::Set(d, b);
          auto vc = hn::Set(d, c);
          check_distribution_match<Op>(d, va, vb, vc);
          check_distribution_match<Op>(d, va, vb, hn::Neg(vc));
          check_distribution_match<Op>(d, va, hn::Neg(vb), vc);
        }
      }
    }
  }
}

template <class Op, class D, class V = hn::VFromD<D>,
          typename T = hn::TFromD<D>>
void do_run_test_random(D d, const double start_range_1st = 0.0,
                        const double end_range_1st = 1.0,
                        const double start_range_2nd = 0.0,
                        const double end_range_2nd = 1.0,
                        const double start_range_3rd = 0.0,
                        const double end_range_3rd = 1.0) {
  helper::RNG rng1(start_range_1st, end_range_1st);
  helper::RNG rng2(start_range_2nd, end_range_2nd);
  helper::RNG rng3(start_range_3rd, end_range_3rd);

  if constexpr (Op::arity == 1) {
    for (int i = 0; i < 100; i++) {
      T a = rng1();
      auto va = hn::Set(d, a);
      check_distribution_match<Op>(d, va);
    }
  } else if constexpr (Op::arity == 2) {
    for (int i = 0; i < 100; i++) {
      T a = rng1();
      T b = rng2();
      auto va = hn::Set(d, a);
      auto vb = hn::Set(d, b);
      check_distribution_match<Op>(d, va, vb);
    }
  } else if constexpr (Op::arity == 3) {
    for (int i = 0; i < 100; i++) {
      T a = rng1();
      T b = rng2();
      T c = rng3();
      auto va = hn::Set(d, a);
      auto vb = hn::Set(d, b);
      auto vc = hn::Set(d, c);
      check_distribution_match<Op>(d, va, vb, vc);
      check_distribution_match<Op>(d, va, vb, hn::Neg(vc));
    }
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
using TestBasicAssertionsSqrt = TestBasicAssertions<SRSqrt>;
using TestBasicAssertionsFma = TestBasicAssertions<SRFma>;

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

HWY_NOINLINE void TestAllBasicAssertionsSqrt() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestBasicAssertionsSqrt>());
}

HWY_NOINLINE void TestAllBasicAssertionsFma() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestBasicAssertionsFma>());
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
using TestRandom01AssertionsSqrt = TestRandom01Assertions<SRSqrt>;
using TestRandom01AssertionsFma = TestRandom01Assertions<SRFma>;

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

HWY_NOINLINE void TestAllRandom01AssertionsSqrt() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandom01AssertionsSqrt>());
}

HWY_NOINLINE void TestAllRandom01AssertionsFma() {
  hn::ForFloat3264Types(hn::ForPartialVectors<TestRandom01AssertionsFma>());
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
  double start_range_3rd = std::ldexp(1.0, s2 + 1);
  double end_range_3rd = std::ldexp(1.0, s2 + 1);
  do_run_test_random<Op>(d, start_range_1st, end_range_1st, start_range_2nd,
                         end_range_2nd, start_range_3rd, end_range_3rd);
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
using TestRandomNoOverlapAssertionsSqrt = TestRandomNoOverlapAssertions<SRSqrt>;
using TestRandomNoOverlapAssertionsFma = TestRandomNoOverlapAssertions<SRFma>;

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
// HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllExactOperationsAdd);
// HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllBasicAssertionsAdd);
// HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllBasicAssertionsSub);
// HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllBasicAssertionsMul);
// HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllBasicAssertionsDiv);
// HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllBasicAssertionsSqrt);
HWY_EXPORT_AND_TEST_P(SRRoundTest, TestAllBasicAssertionsFma);
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
