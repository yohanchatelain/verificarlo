#include <cstdint>
#include <iostream>

#include "hwy/highway.h"

#if defined(HIGHWAY_HWY_VERIFICARLO_SR_DEBUG_INL_H_) ==                        \
    defined(HWY_TARGET_TOGGLE)
#ifdef HIGHWAY_HWY_VERIFICARLO_SR_DEBUG_INL_H_
#undef HIGHWAY_HWY_VERIFICARLO_SR_DEBUG_INL_H_
#else
#define HIGHWAY_HWY_VERIFICARLO_SR_DEBUG_INL_H_
#endif

HWY_BEFORE_NAMESPACE(); // at file scope
namespace sr {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

bool _print_debug() {
  const char *env_debug = getenv("VFC_DEBUG");
  return env_debug && std::string(env_debug) == "1";
}

HWY_API void debug_msg(const std::string &msg) {
  if (not _print_debug())
    return;
  std::cout << msg << std::endl;
}

template <class D, class V, typename T = hn::TFromD<D>>
HWY_API void debug_vec(const std::string &msg, const V &a,
                       const bool hex = true) {
  const D d;
  if (not _print_debug())
    return;
  if (std::is_same<T, float>::value) {
    if (hex) {
      hn::Print(d, msg.c_str(), a, 0, 15, " %+.6a");
    } else {
      hn::Print(d, msg.c_str(), a, 0, 15, " %+.7e");
    }
  } else if (std::is_same<T, double>::value) {
    if (hex) {
      hn::Print(d, msg.c_str(), a, 0, 15, " %+.13a");
    } else {
      hn::Print(d, msg.c_str(), a, 0, 15, " %+.17e");
    }
  } else if (std::is_same<T, std::int32_t>::value) {
    if (hex) {
      hn::Print(d, msg.c_str(), a, 0, 15, " %08x");
    } else {
      hn::Print(d, msg.c_str(), a);
    }
  } else if (std::is_same<T, std::int64_t>::value) {
    if (hex) {
      hn::Print(d, msg.c_str(), a, 0, 15, " %016x");
    } else {
      hn::Print(d, msg.c_str(), a);
    }
  } else {
    hn::Print(d, msg.c_str(), a);
  }
}

template <class D, class M, typename T = hn::TFromD<D>>
HWY_API void debug_mask(const std::string &msg, const M &a) {
  const D d;
  if (not _print_debug())
    return;
  debug_vec<D>(msg, hn::VecFromMask(d, a));
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
} // namespace sr
HWY_AFTER_NAMESPACE();

#endif // HIGHWAY_HWY_VERIFICARLO_SR_DEBUG_INL_H_