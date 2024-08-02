#ifndef __VERIFICARLO_SRLIB_TEST_HELPER_HPP_
#define __VERIFICARLO_SRLIB_TEST_HELPER_HPP_

#include <functional>
#include <map>

#include <boost/math/distributions/binomial.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>

#include "src/utils.hpp"

namespace helper {

template <typename T> struct FTypeName {
  static constexpr auto name = "unknown";
};

template <> struct FTypeName<float> { static constexpr auto name = "float"; };
template <> struct FTypeName<double> { static constexpr auto name = "double"; };

template <typename T> struct Operator {
public:
  enum operator_type {};
  using function = void;
  static constexpr auto ftype = FTypeName<T>::name;
};

template <typename T> struct BinaryOperator : Operator<T> {
public:
  enum operator_type { ADD, SUB, MUL, DIV };
  using function = std::function<T(T, T)>;
  using function_ptr = std::function<T (*)(T, T)>;
};

template <typename T> struct AddOp : public BinaryOperator<T> {
  using function = typename BinaryOperator<T>::function;
  using function_ptr = typename BinaryOperator<T>::function_ptr;
  using operator_type = typename BinaryOperator<T>::operator_type;
  static constexpr auto type = operator_type::ADD;
  static constexpr auto name = "add";
  static constexpr auto symbol = "+";
};

template <typename T> struct SubOp : BinaryOperator<T> {
  using function = typename BinaryOperator<T>::function;
  using function_ptr = typename BinaryOperator<T>::function_ptr;
  using operator_type = typename BinaryOperator<T>::operator_type;
  static constexpr auto type = operator_type::SUB;
  static constexpr auto name = "sub";
};

template <typename T> struct MulOp : BinaryOperator<T> {
  using function = typename BinaryOperator<T>::function;
  using function_ptr = typename BinaryOperator<T>::function_ptr;
  using operator_type = typename BinaryOperator<T>::operator_type;
  static constexpr auto type = operator_type::MUL;
  static constexpr auto name = "mul";
};
template <typename T> struct DivOp : BinaryOperator<T> {
  using function = typename BinaryOperator<T>::function;
  using function_ptr = typename BinaryOperator<T>::function_ptr;
  using operator_type = typename BinaryOperator<T>::operator_type;
  static constexpr auto type = operator_type::DIV;
  static constexpr auto name = "div";
};

using Float128_boost = boost::multiprecision::cpp_bin_float_quad;

template <typename T> struct IEEE754 : sr::utils::IEEE754<T> {
  using H =
      std::conditional_t<std::is_same<T, float>::value, double, Float128_boost>;
};

template <typename T> T absolute_distance(const T a, const T b = 0) {
  if constexpr (std::is_same<T, Float128_boost>::value) {
    return boost::multiprecision::abs(a - b);
    // H diff = a - b;
    // __uint128_t u = reinterpret_cast<__uint128_t &>(diff);
    // __uint128_t mask = 1;
    // mask <<= 127;
    // u &= ~mask;
    // return reinterpret_cast<T &>(u);
  } else {
    return std::abs(a - b);
  }
}

template <typename T> T relative_distance(const T a, const T b) {
  // relative distance between a and b
  // return b if a is zero
  // return a if b is zero
  // return |a - b| / |a| otherwise
  if (a == 0)
    return b;
  if (b == 0)
    return a;
  return absolute_distance(a, b) / absolute_distance(a);
}

// compute ulp(a)
template <typename T, typename H = typename IEEE754<T>::H> H get_ulp(T a) {
  int exponent;
  std::frexp(a, &exponent);
  exponent--;
  H ulp = std::ldexp(1.0, exponent - IEEE754<T>::mantissa);
  return ulp;
}

template <typename T> int get_exponent(T a) {
  int exp = 0;
  if (a == 0)
    return 0;
  if constexpr (std::is_same_v<T, Float128_boost>) {
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

struct RNG {
  std::random_device rd;

private:
  std::mt19937 gen;
  std::uniform_real_distribution<> dis;

public:
  RNG(double a = 0.0, double b = 1.0) : gen(RNG::rd()), dis(a, b){};
  double operator()() { return dis(gen); }
};

template <typename T>
void test_binade(int n, std::function<void(T, T)> &test,
                 int repetitions = 100) {
  auto start = std::ldexp(1.0, n);
  auto end = std::ldexp(1.0, n + 1);
  RNG rng(start, end);

  for (int i = 0; i < repetitions; i++) {
    T a = rng();
    T b = rng();
    test(a, b);
    test(a, -b);
    test(-a, b);
    test(-a, -b);
  }
}

template <typename T> class Counter {
private:
  int _up_count;
  int _down_count;
  T _down;
  T _up;
  std::map<T, int> data;
  bool _is_finalized = false;

public:
  Counter() : _up_count(0), _down_count(0) {}

  int &operator[](const T &key) {
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
    if (keys == data.end()) {
      _is_finalized = true;
      _up = 0;
      _up_count = 0;
      return;
    }
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
  const T &up() {
    finalize();
    return _up;
  }

  const int &down_count() {
    finalize();
    return _down_count;
  }

  const int &up_count() {
    finalize();
    return _up_count;
  }
};

struct BinomialTest {
  double lower;
  double upper;
  double pvalue;
};

BinomialTest binomial_test(const int n, const int k, const double p) {

  boost::math::binomial_distribution<> dist(n, p);
  double lower = boost::math::cdf(dist, k);

  double upper, pvalue;
  if (k > 0) {
    upper = boost::math::cdf(complement(dist, k - 1));
    pvalue = 2 * std::min(lower, upper);
  } else {
    upper = 1;
    pvalue = 2 * lower;
  }

  return BinomialTest{lower, upper, pvalue};
}

}; // namespace helper

#endif // __VERIFICARLO_SRLIB_TEST_HELPER_HPP_