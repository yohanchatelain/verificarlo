#include <iostream>

#include "hwy/highway.h"
#include "hwy/print-inl.h"
#include "src/utils.hpp"
#include "src/xoroshiro256+_hw.hpp"

HWY_BEFORE_NAMESPACE(); // at file scope
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;
// using hn::HWY_NAMESPACE::hn::ScalableTag;

bool _print_debug() {
  const char *env_debug = getenv("VFC_DEBUG");
  return env_debug && std::string(env_debug) == "1";
}

void debug_msg(const std::string &msg) {
  if (not _print_debug())
    return;
  std::cout << msg << std::endl;
}

template <typename T>
void debug_vec(const std::string &msg, const hn::Vec<hn::ScalableTag<T>> &a) {
  if (not _print_debug())
    return;
  if (std::is_same<T, float>::value) {
    hn::Print(hn::ScalableTag<T>(), msg.c_str(), a, 0, 7, " %+.6a");
  } else if (std::is_same<T, double>::value) {
    hn::Print(hn::ScalableTag<T>(), msg.c_str(), a, 0, 7, " %+.13a");
  } else {
    hn::Print(hn::ScalableTag<T>(), msg.c_str(), a);
  }
}

template <typename T>
void debug_mask(const std::string &msg, const hn::Mask<hn::ScalableTag<T>> &a) {
  if (not _print_debug())
    return;
  debug_vec<T>(msg, hn::VecFromMask(a));
}

template <class D, class V> void twosum(V a, V b, V &sigma, V &tau) {
  debug_msg("[twosum] START");
  // using D = hn::ScalableTag<T>;
  const D d{};

  auto abs_a = hn::Abs(a);
  auto abs_b = hn::Abs(b);
  auto a_lt_b = hn::Lt(abs_a, abs_b);

  // Conditional swap if |a| < |b|
  auto a_new = hn::IfThenElse(a_lt_b, b, a);
  auto b_new = hn::IfThenElse(a_lt_b, a, b);

  sigma = hn::Add(a_new, b_new);
  auto z = hn::Sub(sigma, a_new);
  tau = hn::Add(hn::Sub(a_new, hn::Sub(sigma, z)), hn::Sub(b_new, z));
  debug_msg("[twosum] END");
}

template <class D, class V> void twoprodfma(V a, V b, V &sigma, V &tau) {
  // using D = hn::ScalableTag<T>;
  const D d{};

  sigma = hn::Mul(a, b);
  tau = hn::MulSub(a, b, sigma); // Highway's MulSub is equivalent to FMA
}

template <class D, class V> V get_predecessor_abs(V a) {
  const D d{};

  T phi = std::is_same<T, float>::value ? 1.0f - 0x1.0p-24f : 1.0 - 0x1.0p-53;
  return hn::Mul(a, hn::Set(d, phi));
}

template <class D, class V>
hn::Vec<hn::ScalableTag<I>> get_exponent(hn::Vec<hn::ScalableTag<T>> a) {
  debug_msg("[get_exponent] START");

  using D = hn::ScalableTag<T>;
  const D d;
  using U = typename sr::utils::IEEE754<T>::U;
  using DI = hn::ScalableTag<I>;
  const DI di;
  using DU = hn::ScalableTag<std::uint64_t>;
  const DU du;

  debug_vec<T>("[get_exponent] a", a);

  constexpr int32_t mantissa = sr::utils::IEEE754<T>::mantissa;
  constexpr uint32_t exponent_mask = sr::utils::IEEE754<T>::exponent_mask;
  constexpr int32_t bias = sr::utils::IEEE754<T>::bias;

  auto bits = hn::BitCast(di, a);
  auto is_zero = hn::Eq(bits, hn::Zero(di));
  auto raw_exp = bits >> hn::Set(di, mantissa);
  auto exp = hn::And(raw_exp, hn::Set(di, exponent_mask));

  debug_mask<I>("[get_exponent] is_zero", is_zero);
  debug_vec<I>("[get_exponent] bits", bits);
  debug_vec<I>("[get_exponent] raw_exp", raw_exp);
  debug_vec<I>("[get_exponent] exp", exp);

  auto res = hn::IfThenZeroElse(is_zero, hn::BitCast(DI(), exp));
  debug_vec<I>("[get_exponent] res", res);
  debug_msg("[get_exponent] END");
  return res;
}

template <typename T, typename I = typename sr::utils::IEEE754<T>::I>
hn::Vec<hn::ScalableTag<T>> pow2(hn::Vec<hn::ScalableTag<I>> n) {
  using D = hn::ScalableTag<T>;
  const D d;
  using DI = hn::RebindToSigned<D>;
  const DI di;

  constexpr int32_t mantissa = sr::utils::IEEE754<T>::mantissa;
  constexpr int32_t min_exponent = sr::utils::IEEE754<T>::min_exponent;

  auto is_subnormal = hn::Lt(n, hn::Set(di, min_exponent));
  auto precision_loss =
      hn::IfThenElseZero(is_subnormal, hn::Sub(hn::Set(di, min_exponent),
                                               hn::Add(n, hn::Set(di, 1))));

  auto n_adjusted = hn::IfThenElse(is_subnormal, hn::Set(di, 1), n);

  auto res = hn::IfThenZeroElse(is_subnormal, hn::Set(di, 1));

  auto shift = hn::Set(di, mantissa);
  shift = hn::Sub(shift, precision_loss);

  debug_vec<I>("[pow2] n", n);
  debug_vec<I>("[pow2] n_adjusted", n_adjusted);
  debug_vec<I>("[pow2] precision_loss", precision_loss);
  debug_vec<I>("[pow2] shift", shift);

  res = res + (n_adjusted << shift);

  debug_vec<I>("[pow2] res", res);
  debug_vec<T>("[pow2] res", hn::BitCast(d, res));
  //   auto to_add = hn::ShiftLeft<mantissa>(n_adjusted);
  //   to_add = hn::ShiftRight<to_add>(precision_loss);

  //   res = hn::Or(res, to_add);

  return hn::BitCast(d, res);
}

template <typename T>
hn::Mask<hn::ScalableTag<T>> isnumber(hn::Vec<hn::ScalableTag<T>> a,
                                      hn::Vec<hn::ScalableTag<T>> b) {
  using D = hn::ScalableTag<T>;
  const D d;
  using U = typename hwy::MakeUnsigned<T>;
  constexpr U naninf_mask = sr::utils::IEEE754<T>::inf_nan_mask;

  auto a_bits = hn::BitCast(d, a);
  auto b_bits = hn::BitCast(d, b);
  auto mask = hn::Set(d, naninf_mask);

  auto a_not_zero = hn::Ne(a_bits, hn::Zero(d));
  auto b_not_zero = hn::Ne(b_bits, hn::Zero(d));
  auto a_not_naninf = hn::Ne(hn::And(a_bits, mask), mask);
  auto b_not_naninf = hn::Ne(hn::And(b_bits, mask), mask);

  return hn::And(hn::And(a_not_zero, a_not_naninf),
                 hn::And(b_not_zero, b_not_naninf));
}

template <typename T>
hn::Vec<hn::ScalableTag<T>> sr_round(hn::Vec<hn::ScalableTag<T>> sigma,
                                     hn::Vec<hn::ScalableTag<T>> tau) {
  using D = hn::ScalableTag<T>;
  const D d;
  using I = typename sr::utils::IEEE754<T>::I;
  using U = hn::ScalableTag<I>;
  const U u;

  constexpr int32_t mantissa = sr::utils::IEEE754<T>::mantissa;

  auto zero = hn::Zero(d);
  auto sign_tau = hn::Lt(tau, zero);
  auto sign_sigma = hn::Lt(sigma, zero);

  auto z = xoroshiro256plus::rng.Uniform<hn::Vec<D>>();
  debug_vec<T>("[sr_round] z", z);

  auto pred_sigma = get_predecessor_abs<T>(sigma);
  auto sign_diff =
      hn::Xor(hn::RebindMask(u, sign_tau), hn::RebindMask(u, sign_sigma));

  auto eta = hn::IfThenElse(sign_diff, get_exponent<T>(pred_sigma),
                            get_exponent<T>(sigma));

  debug_vec<I>("[sr_round] eta", eta);

  auto ulp = hn::Mul(hn::IfThenElse(sign_tau, hn::Set(d, -1), hn::Set(d, 1)),
                     pow2<T>(hn::Sub(eta, hn::Set(u, mantissa))));
  debug_vec<T>("[sr_round] ulp", ulp);

  auto pi = hn::Mul(ulp, z);
  auto abs_tau_plus_pi = hn::Abs(hn::Add(tau, pi));
  auto abs_ulp = hn::Abs(ulp);
  auto round = hn::IfThenElse(hn::Ge(abs_tau_plus_pi, abs_ulp), ulp, zero);

  return hn::IfThenElse(hn::Eq(tau, zero), sigma, round);
}

template <typename T>
hn::Vec<hn::ScalableTag<T>> sr_add(hn::Vec<hn::ScalableTag<T>> a,
                                   hn::Vec<hn::ScalableTag<T>> b) {
  debug_msg("[sr_add] START");

  using D = hn::ScalableTag<T>;
  const D d;

  auto is_number = isnumber<T>(a, b);
  auto normal_sum = hn::Add(a, b);

  // auto z = xoroshiro256plus::rng.Uniform(hn::Lanes(d));
  hn::Vec<D> sigma, tau;
  twosum<T>(a, b, sigma, tau);
  auto round = sr_round<T>(sigma, tau);
  auto stochastic_sum = hn::Add(sigma, round);

  auto ret = hn::IfThenElse(is_number, stochastic_sum, normal_sum);
  debug_vec<T>("[sr_add] res", ret);
  debug_msg("[sr_add] END");
  return ret;
}

template <typename T>
hn::Vec<hn::ScalableTag<T>> sr_mul(hn::Vec<hn::ScalableTag<T>> a,
                                   hn::Vec<hn::ScalableTag<T>> b) {
  using D = hn::ScalableTag<T>;
  const D d;

  auto is_number = isnumber(a, b);
  auto normal_product = hn::Mul(a, b);

  // auto z = get_rand_double01_vec(d);
  hn::Vec<D> sigma, tau;
  twoprodfma(a, b, sigma, tau);
  auto round = sr_round<T>(sigma, tau);
  auto stochastic_product = hn::Add(sigma, round);

  return hn::IfThenElse(is_number, stochastic_product, normal_product);
}

template <typename T>
hn::Vec<hn::ScalableTag<T>> sr_div(hn::Vec<hn::ScalableTag<T>> a,
                                   hn::Vec<hn::ScalableTag<T>> b) {
  using D = hn::ScalableTag<T>;
  const D d;

  auto is_number = isnumber(a, b);
  auto normal_quotient = hn::Div(a, b);

  // auto z = get_rand_double01_vec(d);
  auto sigma = hn::Div(a, b);
  auto tau = hn::Div(hn::MulSub(hn::Neg(sigma), b, a), b);
  auto round = sr_round(sigma, tau);
  auto stochastic_quotient = hn::Add(sigma, round);

  return hn::IfThenElse(is_number, stochastic_quotient, normal_quotient);
}

template <typename T>
hn::Vec<hn::ScalableTag<T>> sr_sqrt(hn::Vec<hn::ScalableTag<T>> a) {
  using D = hn::ScalableTag<T>;
  const D d;

  auto is_number = isnumber(a, a);
  auto normal_sqrt = hn::Sqrt(a);

  // auto z = get_rand_double01_vec(d);
  auto sigma = hn::Sqrt(a);
  auto tau = hn::Div(hn::MulSub(hn::Neg(sigma), sigma, a),
                     hn::Mul(hn::Set(d, 2), sigma));
  auto round = sr_round(sigma, tau);
  auto stochastic_sqrt = hn::Add(sigma, round);

  return hn::IfThenElse(is_number, stochastic_sqrt, normal_sqrt);
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();