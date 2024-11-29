#ifndef __VERIFICARLO_SRLIB_UD_HPP__
#define __VERIFICARLO_SRLIB_UD_HPP__

#include "debug.hpp"
#include "eft.hpp"
#include "utils.hpp"
#include "xoroshiro256+_hw.h"

namespace prism::ud::scalar {

template <typename T> T udround(T a) {
  debug_start();
  if (a == 0) {
    debug_end();
    return a;
  }
  debug_print("a        = %.13a\n", a);
  using I = typename prism::utils::IEEE754<T>::I;
  I a_bits = *reinterpret_cast<I *>(&a);
  std::uint64_t rand =
      prism::scalar::xoroshiro256plus::static_dispatch::random();
  debug_print("rand     = 0x%02x\n", rand);
  // get the last bit of the random number to get -1 or 1
  a_bits += 1 - ((rand & 1) << 1);
  a = *reinterpret_cast<T *>(&a_bits);
  debug_print("round(a) = %.13a\n", a);
  debug_end();
  return a;
}

template <typename T> T add(T a, T b) { return udround(a + b); }
template <typename T> T sub(T a, T b) { return udround(a - b); }
template <typename T> T mul(T a, T b) { return udround(a * b); }
template <typename T> T div(T a, T b) { return udround(a / b); }
template <typename T> T sqrt(T a) { return udround(std::sqrt(a)); }
template <typename T> T fma(T a, T b, T c) {
  return udround(std::fma(a, b, c));
}

} // namespace prism::ud::scalar

#endif // __UD_HPP__