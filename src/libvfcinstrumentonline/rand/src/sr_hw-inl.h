#include <cmath>
#include <iostream>

#if defined(HIGHWAY_HWY_VERIFICARLO_SR_INL_H_) == defined(HWY_TARGET_TOGGLE)
#ifdef HIGHWAY_HWY_VERIFICARLO_SR_INL_H_
#undef HIGHWAY_HWY_VERIFICARLO_SR_INL_H_
#else
#define HIGHWAY_HWY_VERIFICARLO_SR_INL_H_
#endif

// clang-format off
#include "hwy/highway.h"
#include "hwy/print-inl.h"
#include "src/debug_hwy-inl.h"
#include "src/utils.hpp"
#include "src/xoroshiro256+_hw-inl.hpp"
// clang-format on

HWY_BEFORE_NAMESPACE(); // at file scope
namespace prism {
namespace sr {

namespace vector {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;
namespace rng = prism::vector::xoroshiro256plus::HWY_NAMESPACE;
namespace dbg = prism::vector::HWY_NAMESPACE;

template <class D, class V, typename T = hn::TFromD<D>>
void twosum(V a, V b, V &sigma, V &tau) {
  dbg::debug_msg("\n[twosum] START");
  dbg::debug_vec<D>("[twosum] a", a);
  dbg::debug_vec<D>("[twosum] b", b);

  auto abs_a = hn::Abs(a);
  auto abs_b = hn::Abs(b);
  auto a_lt_b = hn::Lt(abs_a, abs_b);

  // Conditional swap if |a| < |b|
  auto a_new = hn::IfThenElse(a_lt_b, b, a);
  auto b_new = hn::IfThenElse(a_lt_b, a, b);

  sigma = hn::Add(a_new, b_new);
  auto z = hn::Sub(sigma, a_new);
  tau = hn::Add(hn::Sub(a_new, hn::Sub(sigma, z)), hn::Sub(b_new, z));

  dbg::debug_vec<D>("[twosum] sigma", sigma);
  dbg::debug_vec<D>("[twosum] tau", tau);
  dbg::debug_msg("[twosum] END\n");
}

/*
"Emulation of the FMA in rounded-to-nearest loating-point arithmetic"
Stef Graillat, Jean-Michel Muller
---
Algorithm 3 – Split(x, s). Veltkamp’s splitting algorithm. Returns a pair
(xh, xℓ) of FP numbers such that the significand of xh fits in s − p bits, the
significand of xℓ fits in s − 1 bits, and xh + xℓ = x.
Require: K = 2^s + 1
Require: 2 ≤ s ≤ p − 2
γ ← RN(K · x)
δ ← RN(x − γ)
xh ← RN(γ + δ)
xℓ ← RN(x − xh)
return (xh, xℓ)
*/
template <class D, class V = hn::TFromD<D>, typename T = hn::TFromD<D>>
void Split(V x, V &xh, V &xl) {
  const D d;
  dbg::debug_msg("\n[Split] START");
  dbg::debug_vec<D>("[Split] x", x);

  const int s = (int)std::ceil((prism::utils::IEEE754<T>::precision) / 2.0);
  const auto K = hn::Set(d, (1 << s) + 1);

  auto gamma = hn::Mul(K, x);
  auto delta = hn::Sub(x, gamma);
  xh = hn::Add(gamma, delta);
  xl = hn::Sub(x, xh);

  dbg::debug_vec<D>("[Split] xh", xh);
  dbg::debug_vec<D>("[Split] xl", xl);
  dbg::debug_msg("[Split] END\n");
}

/*
"Emulation of the FMA in rounded-to-nearest loating-point arithmetic"
Stef Graillat, Jean-Michel Muller
---
Algorithm 4 – DekkerProd(a, b). Dekker’s product. Returns a pair (πh, πℓ)
of FP numbers such that πh = RN(ab) and πh + πℓ = ab.
Require: s = ⌈p/2⌉
(ah, aℓ) ← Split(a, s)
(bh, bℓ) ← Split(b, s)
πh ← RN(a · b)
t1 ← RN(−πh + RN(ah · bh))
t2 ← RN(t1 + RN(ah · bℓ))
t3 ← RN(t2 + RN(aℓ · bh))
πℓ ← RN(t3 + RN(aℓ · bℓ))
return (πh, πℓ)
*/
template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
void DekkerProd(V a, V b, V &pi_h, V &pi_l) {
  dbg::debug_msg("\n[DekkerProd] START");
  dbg::debug_vec<D>("[DekkerProd] a", a);
  dbg::debug_vec<D>("[DekkerProd] b", b);

  V ah, al;
  V bh, bl;
  Split<D>(a, ah, al);
  Split<D>(b, bh, bl);

  pi_h = hn::Mul(a, b);
  auto t1 = hn::Add(hn::Neg(pi_h), hn::Mul(ah, bh));
  auto t2 = hn::Add(t1, hn::Mul(ah, bl));
  auto t3 = hn::Add(t2, hn::Mul(al, bh));
  pi_l = hn::Add(t3, hn::Mul(al, bl));

  dbg::debug_vec<D>("[DekkerProd] pi_h", pi_h);
  dbg::debug_vec<D>("[DekkerProd] pi_l", pi_l);
  dbg::debug_msg("[DekkerProd] END\n");
}

/*
"Emulation of the FMA in rounded-to-nearest loating-point arithmetic"
Stef Graillat, Jean-Michel Muller
---
Algorithm 7 EmulFMA(a, b, c).
Require: P = 2^(p−1) + 1
Require: Q = 2^(p−1)
(πh, πℓ) ← DekkerProd(a, b)
(sh, sℓ) ← 2Sum(πh, c)
(vh, vℓ) ← 2Sum(πℓ, sℓ)
(zh, zℓ) ← Fast2Sum(sh, vh)
w ← RN(vℓ + zℓ)
L ← RN(P · w)
R ← RN(Q · w)
∆ ← RN(L − R)
d_temp_1 ← RN(zh + w)
if ∆ ≠ w then // mask
  return d_temp_1
else
  w' ← RN(3/2 · w)
  d_temp_2 ← RN(zh + w')
  if d_temp_2 = zh then // mask1
    return zh
  else
    δ ← RN(w − zℓ)
    t ← RN(vℓ − δ)
    if t = 0 then // mask2
      return d_temp_2
    else
      g ← RN(t · w)
      if g < 0 then // mask3
        return zh
      else
        return d_temp_2
      end if
    end if
  end if
end if
*/
template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
V fma_emul(V a, V b, V c) {
  dbg::debug_msg("\n[fma] START");
  dbg::debug_vec<D>("[fma] a", a);
  dbg::debug_vec<D>("[fma] b", b);
  dbg::debug_vec<D>("[fma] c", c);

  const D d;

  constexpr auto ulp = std::is_same<T, float>::value ? 0x1.0p-23f : 0x1.0p-52;
  const auto P = hn::Set(d, 1 + ulp);
  const auto Q = hn::Set(d, ulp);
  const auto q3_2 = hn::Set(d, 1.5);

  V pi_h, pi_l;
  V s_h, s_l;
  V v_h, v_l;
  V z_h, z_l;

  DekkerProd<D>(a, b, pi_h, pi_l);
  twosum<D>(pi_h, c, s_h, s_l);
  twosum<D>(pi_l, s_l, v_h, v_l);
  twosum<D>(s_h, v_h, z_h, z_l);

  auto w = hn::Add(v_l, z_l);
  auto L = hn::Mul(P, w);
  auto R = hn::Mul(Q, w);
  auto delta = hn::Sub(L, R);
  auto d_temp_1 = hn::Add(z_h, w);
  auto mask = hn::Ne(delta, w); // if delta != w then
  // else
  auto w_prime = hn::Mul(q3_2, w);
  auto d_temp_2 = hn::Add(z_h, w_prime);
  auto mask1 = hn::Eq(d_temp_2, z_h); // if d_temp_2 = z_h then
  // else
  auto delta_prime = hn::Sub(w, z_l);
  auto t = hn::Sub(v_l, delta_prime);
  auto mask2 = hn::Eq(t, hn::Zero(d)); // if t = 0 then
  // else
  auto g = hn::Mul(t, w);
  auto mask3 = hn::Lt(g, hn::Zero(d)); // if g < 0 then

  auto ret3 = hn::IfThenElse(mask3, z_h, d_temp_2);
  auto ret2 = hn::IfThenElse(mask2, d_temp_2, ret3);
  auto ret1 = hn::IfThenElse(mask1, z_h, ret2);
  auto ret = hn::IfThenElse(mask, d_temp_1, ret1);

  auto res = ret;

  dbg::debug_vec<D>("[fma] res", res);
  dbg::debug_msg("[fma] END\n");

  return res;
}

template <class D, class V, typename T = hn::TFromD<D>>
void twoprodfma(V a, V b, V &sigma, V &tau) {
  dbg::debug_msg("\n[twoprodfma] START");
  dbg::debug_vec<D>("[twoprodfma] a", a);
  dbg::debug_vec<D>("[twoprodfma] b", b);

  sigma = hn::Mul(a, b);
#if HWY_NATIVE_FMA
  tau = hn::MulSub(a, b, sigma); // Highway's MulSub is equivalent to FMA
#else
#if defined(HWY_COMPILE_ONLY_STATIC) or defined(WARN_FMA_EMULATION)
#warning "FMA not supported, using emulation (slow)"
#endif
  tau = fma_emul<D>(a, b, hn::Neg(sigma));
#endif

  dbg::debug_vec<D>("[twoprodfma] sigma", sigma);
  dbg::debug_vec<D>("[twoprodfma] tau", tau);
  dbg::debug_msg("[twoprodfma] END\n");
}

template <class D, class V, typename T = hn::TFromD<D>>
V get_predecessor_abs(D d, V a) {
  T phi = std::is_same<T, float>::value ? 1.0f - 0x1.0p-24f : 1.0 - 0x1.0p-53;
  return hn::Mul(a, hn::Set(d, phi));
}

template <class D, class V, typename T = hn::TFromD<D>,
          typename I = typename prism::utils::IEEE754<T>::I,
          typename VI = hn::Vec<hn::RebindToSigned<D>>>
VI get_exponent(D d, V a) {
  dbg::debug_msg("\n[get_exponent] START");

  using U = typename prism::utils::IEEE754<T>::U;
  using DI = hn::RebindToSigned<D>;
  const DI di{};

  dbg::debug_vec<D>("[get_exponent] a", a);

  constexpr I mantissa = prism::utils::IEEE754<T>::mantissa;
  constexpr U exponent_mask = prism::utils::IEEE754<T>::exponent_mask_scaled;
  constexpr I bias = prism::utils::IEEE754<T>::bias;

  auto bits = hn::BitCast(di, a);
  auto is_zero = hn::Eq(bits, hn::Zero(di));
  auto raw_exp = hn::And(bits, hn::Set(di, exponent_mask));
  auto exp = raw_exp >> hn::Set(di, mantissa);
  exp = hn::Sub(exp, hn::Set(di, bias));

  dbg::debug_mask<DI>("[get_exponent] is_zero", is_zero);
  dbg::debug_vec<DI>("[get_exponent] bits", bits);
  dbg::debug_vec<DI>("[get_exponent] raw_exp", raw_exp);
  dbg::debug_vec<DI>("[get_exponent] exp", exp, false);

  auto res = hn::IfThenZeroElse(is_zero, exp);

  dbg::debug_vec<DI>("[get_exponent] res", res, false);
  dbg::debug_msg("[get_exponent] END\n");

  return res;
}

// Computes 2^x, where x is an integer.
// Only works for x in the range of the exponent of the floating point type.
template <class D, class VI = hn::Vec<hn::Rebind<hwy::MakeSigned<D>, D>>,
          typename T = hn::TFromD<D>>
HWY_INLINE hn::Vec<D> FastPow2I(D d, VI x) {
  constexpr auto kOffsetS = std::is_same<T, float>::value ? 0x7F : 0x3FF;
  constexpr auto mantissa = std::is_same<T, float>::value ? 23 : 52;
  const hn::Rebind<hwy::MakeSigned<D>, D> di;
  const auto kOffset = Set(di, kOffsetS);
  const auto offset = Add(x, kOffset);
  const auto shift = hn::ShiftLeft<mantissa>(offset);
  return BitCast(d, shift);
}

template <class D, class V, typename T = hn::TFromD<D>>
hn::Vec<D> pow2(D d, V n) {
  dbg::debug_msg("\n[pow2] START");

  using DI = hn::RebindToSigned<D>;
  using I = typename prism::utils::IEEE754<T>::I;
  const DI di;

  dbg::debug_vec<DI>("[pow2] n", n, false);

  constexpr I mantissa = prism::utils::IEEE754<T>::mantissa;
  constexpr I min_exponent = prism::utils::IEEE754<T>::min_exponent;
  // constexpr I bias = prism::utils::IEEE754<T>::bias;

  // is_subnormal = n < min_exponent
  auto is_subnormal = hn::Lt(n, hn::Set(di, min_exponent));
  // precision_loss = is_subnormal ? min_exponent - n : 0
  auto precision_loss =
      hn::IfThenElseZero(is_subnormal, hn::Sub(hn::Set(di, min_exponent), n));

  // n_adjusted = is_subnormal ? 1 : n
  auto n_adjusted = hn::IfThenElse(is_subnormal, hn::Set(di, 1), n);

  const T one = 1.0;
  const auto one_as_int = reinterpret_cast<const I &>(one);

  // res = is_subnormal ? 0 : 1
  auto res = hn::IfThenZeroElse(is_subnormal, hn::Set(di, one_as_int));

  dbg::debug_vec<DI>("[pow2] res", res);

  // shift = mantissa - precision_loss
  auto shift = hn::Sub(hn::Set(di, mantissa), precision_loss);

  dbg::debug_mask<DI>("[pow2] is_subnormal", is_subnormal);
  dbg::debug_vec<DI>("[pow2] n_adjusted", n_adjusted, false);
  dbg::debug_vec<DI>("[pow2] precision_loss", precision_loss, false);
  dbg::debug_vec<DI>("[pow2] shift", shift, false);

  // res += res << shift
  res = res + (n_adjusted << shift);

  dbg::debug_vec<DI>("[pow2] res", res);
  dbg::debug_vec<D>("[pow2] res", hn::BitCast(d, res));
  dbg::debug_msg("[pow2] END\n");

  return hn::BitCast(d, res);
}

template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
V round(V sigma, V tau) {
  dbg::debug_msg("\n[sr_round] START");
  dbg::debug_vec<D>("[sr_round] sigma", sigma);
  dbg::debug_vec<D>("[sr_round] tau", tau);

  const D d{};
  // get tag for int with same number of lanes as T
  using DI = hn::RebindToSigned<D>;
  const DI di;

  constexpr int32_t mantissa = prism::utils::IEEE754<T>::mantissa;

  auto zero = hn::Zero(d);
  auto sign_tau = hn::Lt(tau, zero);
  auto sign_sigma = hn::Lt(sigma, zero);

  auto z = rng::uniform(d);
  dbg::debug_vec<D>("[sr_round] z", z);

  auto pred_sigma = get_predecessor_abs(d, sigma);

  auto sign_diff =
      hn::Xor(sign_tau, sign_sigma); // sign_diff = sign_tau != sign_sigma

  auto sign_diff_int = hn::RebindMask(di, sign_diff);

  auto eta = hn::IfThenElse(sign_diff_int, get_exponent(d, pred_sigma),
                            get_exponent(d, sigma));
  dbg::debug_vec<DI>("[sr_round] eta", eta, false);

  auto exp = hn::Sub(eta, hn::Set(di, mantissa));
  auto abs_ulp = pow2(d, exp);
  dbg::debug_vec<D>("[sr_round] |ulp|", abs_ulp);

  auto ulp = hn::CopySign(abs_ulp, tau);
  dbg::debug_vec<D>("[sr_round] ulp", ulp);

  auto pi = hn::Mul(ulp, z);
  dbg::debug_vec<D>("[sr_round] pi", pi);

  auto abs_tau_plus_pi = hn::Abs(hn::Add(tau, pi));
  dbg::debug_vec<D>("[sr_round] |tau|+pi", abs_tau_plus_pi);

  auto round = hn::IfThenElse(hn::Ge(abs_tau_plus_pi, abs_ulp), ulp, zero);
  dbg::debug_vec<D>("[sr_round] round", round);

  dbg::debug_msg("[sr_round] END\n");
  return round;
}

template <class D, class V = hn::VFromD<D>, typename T = hn::TFromD<D>>
HWY_API V add(V a, V b) {
  dbg::debug_msg("\n[sr_add] START");
  V sigma, tau;
  twosum<D>(a, b, sigma, tau);
  auto rounding = round<D>(sigma, tau);
  auto ret = hn::Add(sigma, rounding);
  dbg::debug_vec<D>("[sr_add] res", ret);
  dbg::debug_msg("[sr_add] END\n");
  return ret;
}

template <class D, class V, typename T = hn::TFromD<D>>
HWY_API V sub(V a, V b) {
  return add<D>(a, hn::Neg(b));
}

template <class D, class V, typename T = hn::TFromD<D>>
HWY_API V mul(V a, V b) {
  dbg::debug_msg("\n[sr_add] START");
  V sigma, tau;
  twoprodfma<D>(a, b, sigma, tau);
  auto rounding = round<D>(sigma, tau);
  auto ret = hn::Add(sigma, rounding);
  dbg::debug_vec<D>("[sr_mul] res", ret);
  dbg::debug_msg("[sr_mul] END\n");

  return ret;
}

/*
Algorithm 6.9. Division With Stochastic Rounding Without
the Change of the Rounding Mode

1. Function Div2(a, b)
2. Compute σ = SR(a / b)
3. Z = rand()
4. σ = RN(a / b)
5. τ = RN(-σ * b + a)
6. τ = RN(τ / b)
7. round = SRround(σ, τ, Z)
8. σ = RN(σ + round)
9. return σ
*/
template <class D, class V, typename T = hn::TFromD<D>>
HWY_API V div(V a, V b) {
  dbg::debug_msg("\n[sr_div] START");
  dbg::debug_vec<D>("[sr_div] a", a);
  dbg::debug_vec<D>("[sr_div] b", b);
  auto sigma = hn::Div(a, b);
#if HWY_NATIVE_FMA
  auto taup = hn::NegMulAdd(sigma, b, a);
#else
#if defined(HWY_COMPILE_ONLY_STATIC) or defined(WARN_FMA_EMULATION)
#warning "FMA not supported, using emulation (slow)"
#endif
  auto taup = fma_emul<D>(hn::Neg(sigma), b, a);
#endif
  auto tau = hn::Div(taup, b);
  auto rounding = round<D>(sigma, tau);
  auto ret = hn::Add(sigma, rounding);
  dbg::debug_vec<D>("[sr_div] sigma", sigma);
  dbg::debug_vec<D>("[sr_div] tau'", taup);
  dbg::debug_vec<D>("[sr_div] tau", tau);
  dbg::debug_vec<D>("[sr_div] round", rounding);
  dbg::debug_vec<D>("[sr_div] res", ret);
  dbg::debug_msg("[sr_div] END\n");

  return ret;
}

template <class D, class V, typename T = hn::TFromD<D>> V sqrt(V a) {
  dbg::debug_msg("\n[sr_sqrt] START");

  const D d{};
  auto sigma = hn::Sqrt(a);
  auto tau = hn::Div(hn::MulSub(hn::Neg(sigma), sigma, a),
                     hn::Mul(hn::Set(d, 2), sigma));
  auto rounding = round<D>(sigma, tau);
  auto ret = hn::Add(sigma, rounding);
  dbg::debug_vec<D>("[sr_sqrt] res", ret);
  dbg::debug_msg("[sr_sqrt] END\n");

  return ret;
}

/*
"Exact and Approximated error of the FMA"
Sylvie Boldo, Jean-Michel Muller
---
Algorithm 5 (ErrFmaNearest):
  r1 = ◦(ax + y)
  (u1, u2) = Fast2Mult(a, x)
  (α1, α2) = 2Sum(y, u2)
  (β1, β2) = 2Sum(u1, α1)
  γ = ◦(◦(β1 − r1) + β2)
  r2 = ◦(γ + α2)
*/
template <class D, class V, typename T = hn::TFromD<D>> V fma(V a, V b, V c) {
  dbg::debug_msg("\n[sr_fma] START");
  dbg::debug_vec<D>("[sr_fma] a", a);
  dbg::debug_vec<D>("[sr_fma] b", b);
  dbg::debug_vec<D>("[sr_fma] c", c);

#if HWY_NATIVE_FMA
  auto r1 = hn::MulAdd(a, b, c);
#else
#if defined(HWY_COMPILE_ONLY_STATIC) or defined(WARN_FMA_EMULATION)
#warning "FMA not supported, using emulation (slow)"
#endif
  auto r1 = fma_emul<D>(a, b, c);
#endif
  V u1, u2, alpha1, alpha2, beta1, beta2, gamma, r2;
  twoprodfma<D>(a, b, u1, u2);
  twosum<D>(c, u2, alpha1, alpha2);
  twosum<D>(u1, alpha1, beta1, beta2);
  gamma = hn::Add(hn::Sub(beta1, r1), beta2);
  r2 = hn::Add(gamma, alpha2);
  auto rounding = round<D>(r1, r2);
  auto res = hn::Add(r1, rounding);
  dbg::debug_vec<D>("[sr_fma] res", res);
  dbg::debug_msg("[sr_fma] END\n");

  return res;
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
} // namespace vector
} // namespace sr
} // namespace prism
HWY_AFTER_NAMESPACE();

#endif // HIGHWAY_HWY_VERIFICARLO_SR_INL_H_