#include <iostream>

#if defined(HIGHWAY_HWY_VERIFICARLO_SR_INL_H_) == defined(HWY_TARGET_TOGGLE)
#ifdef HIGHWAY_HWY_VERIFICARLO_SR_INL_H_
#undef HIGHWAY_HWY_VERIFICARLO_SR_INL_H_
#else
#define HIGHWAY_HWY_VERIFICARLO_SR_INL_H_
#endif

#include "hwy/highway.h"
#include "hwy/print-inl.h"
#include "src/debug_hwy-inl.h"
#include "src/utils.hpp"
#include "src/xoroshiro256+_hw.hpp"

HWY_BEFORE_NAMESPACE(); // at file scope
namespace sr {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;
// using hn::HWY_NAMESPACE::hn::ScalableTag;

template <class D, class V, typename T = hn::TFromD<D>>
void twosum(V a, V b, V &sigma, V &tau) {
  debug_msg("\n[twosum] START");
  debug_vec<D>("[twosum] a", a);
  debug_vec<D>("[twosum] b", b);

  // using D = hn::ScalableTag<T>;
  // const D d{};

  auto abs_a = hn::Abs(a);
  auto abs_b = hn::Abs(b);
  auto a_lt_b = hn::Lt(abs_a, abs_b);

  // Conditional swap if |a| < |b|
  auto a_new = hn::IfThenElse(a_lt_b, b, a);
  auto b_new = hn::IfThenElse(a_lt_b, a, b);

  sigma = hn::Add(a_new, b_new);
  auto z = hn::Sub(sigma, a_new);
  tau = hn::Add(hn::Sub(a_new, hn::Sub(sigma, z)), hn::Sub(b_new, z));

  debug_vec<D>("[twosum] sigma", sigma);
  debug_vec<D>("[twosum] tau", tau);
  debug_msg("[twosum] END\n");
}

template <class D, class V, typename T = hn::TFromD<D>>
void twoprodfma(V a, V b, V &sigma, V &tau) {
  debug_msg("\n[twoprodfma] START");
  debug_vec<D>("[twoprodfma] a", a);
  debug_vec<D>("[twoprodfma] b", b);

  // using D = hn::ScalableTag<T>;
  const D d{};

  sigma = hn::Mul(a, b);
  tau = hn::MulSub(a, b, sigma); // Highway's MulSub is equivalent to FMA

  debug_vec<D>("[twoprodfma] sigma", sigma);
  debug_vec<D>("[twoprodfma] tau", tau);
  debug_msg("[twoprodfma] END\n");
}

template <class D, class V, typename T = hn::TFromD<D>>
V get_predecessor_abs(D d, V a) {
  T phi = std::is_same<T, float>::value ? 1.0f - 0x1.0p-24f : 1.0 - 0x1.0p-53;
  return hn::Mul(a, hn::Set(d, phi));
}

template <class D, class V, typename T = hn::TFromD<D>,
          typename I = typename sr::utils::IEEE754<T>::I,
          typename VI = hn::Vec<hn::RebindToSigned<D>>>
VI get_exponent(D d, V a) {
  debug_msg("\n[get_exponent] START");

  using U = typename sr::utils::IEEE754<T>::U;
  using DI = hn::RebindToSigned<D>;
  const DI di{};

  debug_vec<D>("[get_exponent] a", a);

  constexpr I mantissa = sr::utils::IEEE754<T>::mantissa;
  constexpr U exponent_mask = sr::utils::IEEE754<T>::exponent_mask_scaled;
  constexpr I bias = sr::utils::IEEE754<T>::bias;

  auto bits = hn::BitCast(di, a);
  auto is_zero = hn::Eq(bits, hn::Zero(di));
  auto raw_exp = hn::And(bits, hn::Set(di, exponent_mask));
  auto exp = raw_exp >> hn::Set(di, mantissa);
  exp = hn::Sub(exp, hn::Set(di, bias));

  debug_mask<DI>("[get_exponent] is_zero", is_zero);
  debug_vec<DI>("[get_exponent] bits", bits);
  debug_vec<DI>("[get_exponent] raw_exp", raw_exp);
  debug_vec<DI>("[get_exponent] exp", exp, false);

  auto res = hn::IfThenZeroElse(is_zero, exp);

  debug_vec<DI>("[get_exponent] res", res, false);
  debug_msg("[get_exponent] END\n");

  return res;
}

template <class D, class V, typename T = hn::TFromD<D>>
hn::Vec<D> pow2(D d, V n) {
  debug_msg("\n[pow2] START");

  using DI = hn::RebindToSigned<D>;
  using I = typename sr::utils::IEEE754<T>::I;
  const DI di;

  debug_vec<DI>("[pow2] n", n, false);

  constexpr I mantissa = sr::utils::IEEE754<T>::mantissa;
  constexpr I min_exponent = sr::utils::IEEE754<T>::min_exponent;
  constexpr I bias = sr::utils::IEEE754<T>::bias;

  auto is_subnormal = hn::Lt(n, hn::Set(di, min_exponent));
  auto precision_loss =
      hn::IfThenElseZero(is_subnormal, hn::Sub(hn::Set(di, min_exponent),
                                               hn::Add(n, hn::Set(di, 1))));

  auto n_adjusted =
      hn::IfThenElse(is_subnormal, hn::Set(di, 0), n + hn::Set(di, bias));

  auto res = hn::IfThenZeroElse(hn::Not(is_subnormal), hn::Set(di, 1));

  debug_vec<DI>("[pow2] res", res);

  auto shift = hn::Sub(hn::Set(di, mantissa), precision_loss);

  debug_mask<DI>("[pow2] is_subnormal", is_subnormal);
  debug_vec<DI>("[pow2] n_adjusted", n_adjusted, false);
  debug_vec<DI>("[pow2] precision_loss", precision_loss, false);
  debug_vec<DI>("[pow2] shift", shift, false);

  res = res + (n_adjusted << shift);

  debug_vec<DI>("[pow2] res", res);
  debug_vec<D>("[pow2] res", hn::BitCast(d, res));
  debug_msg("[pow2] END\n");

  return hn::BitCast(d, res);
}

template <class D, class V, typename T = hn::TFromD<D>> V isnumber(V a, V b) {
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

  auto res = hn::And(hn::And(a_not_zero, a_not_naninf),
                     hn::And(b_not_zero, b_not_naninf));
  return hn::VecFromMask(d, res);
}

template <class D, class V, typename T = hn::TFromD<D>>
HWY_MAYBE_UNUSED V sr_round(V sigma, V tau) {
  debug_msg("\n[sr_round] START");
  debug_vec<D>("[sr_round] sigma", sigma);
  debug_vec<D>("[sr_round] tau", tau);

  const D d{};
  // get tag for int with same number of lanes as T
  using DI = hn::RebindToSigned<D>;
  const DI di;

  constexpr int32_t mantissa = sr::utils::IEEE754<T>::mantissa;

  auto zero = hn::Zero(d);
  auto sign_tau = hn::Lt(tau, zero);
  auto sign_sigma = hn::Lt(sigma, zero);

  auto z = xoroshiro256plus::uniform(d);

  auto pred_sigma = get_predecessor_abs(d, sigma);

  auto sign_diff =
      hn::Xor(sign_tau, sign_sigma); // sign_diff = sign_tau != sign_sigma

  auto sign_diff_int = hn::RebindMask(di, sign_diff);

  auto eta = hn::IfThenElse(sign_diff_int, get_exponent(d, pred_sigma),
                            get_exponent(d, sigma));

  auto ulp_abs = pow2(d, hn::Sub(eta, hn::Set(di, mantissa)));
  auto ulp = hn::IfThenElse(sign_diff, hn::Neg(ulp_abs), ulp_abs);

  auto pi = hn::Mul(ulp, z);
  auto abs_tau_plus_pi = hn::Abs(hn::Add(tau, pi));
  auto abs_ulp = hn::Abs(ulp);
  auto round = hn::IfThenElse(hn::Ge(abs_tau_plus_pi, abs_ulp), ulp, zero);

  debug_vec<D>("[sr_round] z", z);
  debug_vec<DI>("[sr_round] eta", eta, false);
  debug_vec<D>("[sr_round] ulp", ulp);
  debug_vec<D>("[sr_round] pi", pi);
  debug_vec<D>("[sr_round] abs_tau_plus_pi", abs_tau_plus_pi);
  debug_vec<D>("[sr_round] abs_ulp", abs_ulp);
  debug_vec<D>("[sr_round] round", round);

  auto res = hn::IfThenElse(hn::Eq(tau, zero), sigma, round);

  debug_vec<D>("[sr_round] res", res);
  debug_msg("[sr_round] END\n");

  return res;
}

template <class D, class V, typename T = hn::TFromD<D>>
HWY_API V sr_add(V a, V b) {
  debug_msg("\n[sr_add] START");

  // using D = hn::ScalableTag<T>;
  // const D d;

  auto is_number = isnumber<D>(a, b);
  auto normal_sum = hn::Add(a, b);

  // auto z = xoroshiro256plus::rng.Uniform(hn::Lanes(d));
  hn::Vec<D> sigma, tau;
  twosum<D>(a, b, sigma, tau);
  auto round = sr_round<D>(sigma, tau);
  auto stochastic_sum = hn::Add(sigma, round);

  auto is_number_mask = hn::MaskFromVec(is_number);
  auto ret = hn::IfThenElse(is_number_mask, stochastic_sum, normal_sum);
  debug_vec<D>("[sr_add] res", ret);
  debug_msg("[sr_add] END\n");
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
} // namespace sr
HWY_AFTER_NAMESPACE();

#endif // HIGHWAY_HWY_VERIFICARLO_SR_INL_H_