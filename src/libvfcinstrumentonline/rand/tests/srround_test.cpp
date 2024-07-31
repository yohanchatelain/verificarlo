#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include <boost/math/distributions/binomial.hpp>
#include <gtest/gtest.h>

#include "src/eft.hpp"
#include "src/ud.hpp"
#include "src/utils.hpp"

namespace reference {
// return pred(|s|)

// twosum reference
// compute in double precision if the input type is float
// compute in quad precision if the input type is double
template <typename T, typename R = typename IEEE754<T>::H> R ud_add(T a, T b) {
  using H = typename IEEE754<T>::H;
  return static_cast<H>(a) + static_cast<H>(b);
}

template <typename T, typename R = typename IEEE754<T>::H> R ud_sub(T a, T b) {
  using H = typename IEEE754<T>::H;
  return static_cast<H>(a) - static_cast<H>(b);
}

template <typename T, typename R = typename IEEE754<T>::H> R ud_mul(T a, T b) {
  using H = typename IEEE754<T>::H;
  return static_cast<H>(a) * static_cast<H>(b);
}

template <typename T, typename R = typename IEEE754<T>::H> R ud_div(T a, T b) {
  using H = typename IEEE754<T>::H;
  return static_cast<H>(a) / static_cast<H>(b);
}

template <typename T> T absolute_distance(T a, T b = 0) {
  if constexpr (std::is_same<T, Float128>::value) {
    __uint128_t u = reinterpret_cast<__uint128_t &>(a - b);
    __uint128_t mask = 1;
    mask <<= 127;
    u &= ~mask;
    return reinterpret_cast<T &>(u);
  } else {
    return std::abs(a - b);
  }
}

template <typename T> int get_exponent(T a) {
  int exp = 0;
  if (a == 0)
    return 0;
  if constexpr (std::is_same_v<T, Float128>) {
    __uint128_t u = reinterpret_cast<__uint128_t &>(a);
    __uint128_t mask = 0x7fff;
    u >>= 112;
    exp = (int)(u & mask);
    exp -= 0x3FFF;
  } else {
    using U = typename IEEE754<T>::U;
    U u = reinterpret_cast<U &>(a);
    u >>= IEEE754<T>::mantissa;
    exp = (int)(u & IEEE754<T>::exponent_mask);
    exp -= IEEE754<T>::bias;
  }
  return exp;
}

template <typename T> T relative_distance(T a, T b) {
  if (a == 0 and b == 0)
    return 0;
  if (a == 0)
    return b;
  if (b == 0)
    return a;
  return absolute_distance(a, b) / absolute_distance(a);
}

}; // namespace reference

template <typename T> class Counter {
private:
  int _up_count;
  int _down_count;
  T _down;
  T _up;
  map<T, int> data;
  bool _is_finalized = false;

public:
  Counter() : _up_count(0), _down_count(0) {}

  T &operator[](const T &key) {
    _is_finalized = false;
    return data[key];
  }

  void finalize() {
    if (_is_finalized)
      return;

    auto keys = data.begin();
    _down = keys->first;
    _down_count = keys->second;
    keys++;
    _up = keys->first;
    _up_count = keys->second;

    if (_down > _up) {
      std::swap(_down, _up);
      std::swap(_down_count, _up_count);
    }

    _is_finalized = true;
  }

  const int size() const { return data.size(); }

  const T &down() {
    finalize();
    return _down;
  }
  const T &up() const {
    finalize();
    return _up;
  }

  const int &down_count() const {
    finalize();
    return _down_count;
  }

  const int &up_count() const {
    finalize();
    return _up_count;
  }
};

typedef std::function<T(T, T)> binary_operator;

// compute ulp(a)
template <typename T, typename H = typename IEEE754<T>::H> H get_ulp(T a) {
  int exponent;
  std::frexp(a, &exponent);
  exponent--;
  H ulp = std::ldexp(1.0, exponent - IEEE754<T>::mantissa);
  return ulp;
}

template <typename T, typename H = typename IEEE754<T>::H>
std::pair<float, float> compute_distance_error(T a, T b, H reference) {
  using H = typename IEEE754<T>::H;

  T ref_cast = static_cast<T>(reference);

  H distance_to_fp = reference::absolute_distance(reference, ref_cast);
  H ulp = get_ulp(ref_cast);

  H probability = distance_to_fp / ulp;
  H probability_c = 1 - probability;

  if (ref_cast > ref)
    return {probability_c, probability};
  else
    return {probability, probability_c};
}

template <typename T>
Counter eval_op(T a, T b, binary_operator op, const int repetitions = 1000) {
  Counter c;
  for (int i = 0; i < repetitions; i++) {
    T v = op(a, b);
    c[v]++;
  }

  EXPECT_EQ(c.size(), 2) << "More than two values returned by "
                            "target function";
  EXPECT_EQ(c.count_down, repetitions * c.down) << "Down value count mismatch";
  EXPECT_EQ(c.count_up, repetitions * up) << "Up value count mismatch";
  EXPECT_EQ(c.count_down + c.count_up, repetitions) << "Total count mismatch";

  return c;
}

struct BinomialTest {
  float lower_bound;
  float upper_bound;
  bool result;
};

template <typename T>
BinomialTest binomial_test(const int n, const int k, const float p,
                           const float alpha = 0.05) {
  auto lower_bound = boost::math::binomial_distribution<int>::find_lower_bound(
      n, k, alpha / 2);
  auto upper_bound = boost::math::binomial_distribution<int>::find_upper_bound(
      n, k, alpha / 2);

  auto res = BinomialTest{lower_bound, upper_bound,
                          lower_bound <= p and p <= upper_bound};
  return res;
}

template <typename T>
void check_exact_proportion(T a, T b, binary_operator op,
                            const float proportion,
                            const int repetitions = 1000) {
  auto counter = eval_op(a, b, op, repetitions);
  auto down_test = binomial_test(repetitions, counter.down_count(), proportion);
  auto up_test = binomial_test(repetitions, counter.up_count(), 1 - proportion);

  EXPECT_THAT(proportion,
              ::testing::AllOf(::testing::Ge(down_test.lower_bound),
                               ::testing::Le(down_test.upper_bound)))
      << "Propability of down value mismatch!\n"
      << "down: " << counter.down() << "\n"
      << "lower_bound_down: " << down_test.lower_bound << "\n"
      << "upper_bound_down: " << down_test.upper_bound
      << "sample size: " << repetitions << "\n"
      << "count_down: " << counter.down_count() << "\n"
      << "alpha: " << alpha << "\n";

  EXPECT_THAT(1 - proportion,
              ::testing::AllOf(::testing::Ge(up_test.lower_bound),
                               ::testing::Le(up_test.upper_bound)))
      << "Propability of up value mismatch\n"
      << "up: " << counter.up() << "\n"
      << "lower_bound_up: " << up_test.lower_bound << "\n"
      << "upper_bound_up: " << up_test.upper_bound
      << "sample size: " << repetitions << "\n"
      << "count_up: " << counter.up_count() << "\n"
      << "alpha: " << alpha << "\n";
}

template <typename T>
void check_distribution_match(T a, T b, binary_operator reference_op,
                              binary_operator target_op,
                              const int repetitions = 1000,
                              const float alpha = 0.05) {
  using H = typename IEEE754<T>::H;

  H reference = reference_op(a, b);
  auto [down, up] = compute_distance_error(a, b, reference);

  auto target_values = eval_op(a, b, target_op, repetitions);
  auto count_down = target_values.down_count();
  auto count_up = target_values.up_count();

  EXPECT_EQ(count_down, repetitions * down) << "Down value count mismatch";
  EXPECT_EQ(count_up, repetitions * up) << "Up value count mismatch";
  EXPECT_EQ(count_down + count_up, repetitions) << "Total count mismatch";

  auto down_test = binomial_test(repetitions, count_down, down, alpha);

  EXPECT_THAT(down, ::testing::AllOf(::testing::Ge(down_test.lower_bound),
                                     ::testing::Le(down_test.upper_bound)))
      << "Propability of down value mismatch!\n"
      << "down: " << down << "\n"
      << "lower_bound_down: " << down_test.lower_bound << "\n"
      << "upper_bound_down: " << down_test.upper_bound
      << "sample size: " << repetitions << "\n"
      << "count_down: " << counter.down_count() << "\n"
      << "alpha: " << alpha << "\n";

  auto up_test = binomial_test(repetitions, count_up, up, alpha);

  EXPECT_THAT(up, ::testing::AllOf(::testing::Ge(up_test.lower_bound),
                                   ::testing::Le(up_test.upper_bound)))
      << "Propability of up value mismatch\n"
      << "up: " << counter.up() << "\n"
      << "lower_bound_up: " << up_test.lower_bound << "\n"
      << "upper_bound_up: " << up_test.upper_bound
      << "sample size: " << repetitions << "\n"
      << "count_up: " << counter.up_count() << "\n"
      << "alpha: " << alpha << "\n";
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

TEST(GetTwoSumTest, BasicAssertionsSum) {

  float a = 1.25f;
  float b;
  float p;

  for (int i = 0; i <= 3; i++) {
    b = std::ldexp(1.0, -(23 + i));
    p = (i == 0) ? 1 : 1 - 1 / i;
    check_exact_proportion(a, b, reference::ud_add, p);
  }
}

TEST(GetTwoSumTest, Random01Assertions) {
  RNG rng;

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
  RNG rng_0v1_float(0, 1);
  RNG rng_24v25_float(0x1p24, 0x1p25);

  for (int i = 0; i < 1000; i++) {
    float a = rng_0v1_float();
    float b = rng_24v25_float();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }

  RNG rng_0v1_double(0, 1);
  RNG rng_53v54_double(0x1p53, 0x1p54);

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
  RNG rng_1v2_float(1, 2);
  RNG rng_23v24_float(0x1p23, 0x1p24);

  for (int i = 0; i < 1000; i++) {
    float a = rng_1v2_float();
    float b = rng_23v24_float();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }

  RNG rng_1v2_double(1, 2);
  RNG rng_52v53_double(0x1p52, 0x1p53);

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
  RNG rng_0v1_float(0, 1);
  RNG rng_12v13_float(0x1p12, 0x1p13);

  for (int i = 0; i < 1000; i++) {
    float a = rng_0v1_float();
    float b = rng_12v13_float();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }

  RNG rng_0v1_double(0, 1);
  RNG rng_26v27_double(0x1p26, 0x1p27);

  for (int i = 0; i < 1000; i++) {
    double a = rng_0v1_double();
    double b = rng_26v27_double();
    test_equality(a, b);
    test_equality(a, -b);
    test_equality(-a, b);
    test_equality(-a, -b);
  }
}

TEST(GetTwoSumTest, BinadeAssertions) {
  for (int i = -126; i < 127; i++) {
    testBinade<float>(i);
    testBinade<double>(i);
  }
}