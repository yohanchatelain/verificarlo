#ifndef __VERIFICARLO_SRLIB_TEST_HELPER_HPP_
#define __VERIFICARLO_SRLIB_TEST_HELPER_HPP_

#include "src/utils.hpp"

namespace helper {
template <typename T> T absolute_distance(const T a, const T b = 0) {
  if constexpr (std::is_same<T, Float128>::value) {
    T diff = a - b;
    __uint128_t u = reinterpret_cast<__uint128_t &>(diff);
    __uint128_t mask = 1;
    mask <<= 127;
    u &= ~mask;
    return reinterpret_cast<T &>(u);
  } else {
    return std::abs(a - b);
  }
}

template <typename T> T relative_distance(const T a, const T b) {
  if (a == 0)
    return b;
  if (b == 0)
    return a;
  return absolute_distance(a, b) / absolute_distance(a);
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

}; // namespace helper

#endif // __VERIFICARLO_SRLIB_TEST_HELPER_HPP_