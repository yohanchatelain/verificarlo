
#ifndef __VERIFICARLO_SRLIB_DEBUG_HWY_HPP__
#define __VERIFICARLO_SRLIB_DEBUG_HWY_HPP__

#include <iostream>

#include "hwy/highway.h"

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
void debug_vec(const std::string &msg, const hn::Vec<hn::ScalableTag<T>> &a,
               const bool hex = true) {
  if (not _print_debug())
    return;
  if (std::is_same<T, float>::value) {
    if (hex) {
      hn::Print(hn::ScalableTag<T>(), msg.c_str(), a, 0, 7, " %+.6a");
    } else {
      hn::Print(hn::ScalableTag<T>(), msg.c_str(), a, 0, 7, " %+.7e");
    }
  } else if (std::is_same<T, double>::value) {
    if (hex) {
      hn::Print(hn::ScalableTag<T>(), msg.c_str(), a, 0, 7, " %+.13a");
    } else {
      hn::Print(hn::ScalableTag<T>(), msg.c_str(), a, 0, 7, " %+.17e");
    }
  } else if (std::is_same<T, std::int32_t>::value) {
    if (hex) {
      hn::Print(hn::ScalableTag<T>(), msg.c_str(), a, 0, 7, " %08x");
    } else {
      hn::Print(hn::ScalableTag<T>(), msg.c_str(), a);
    }
  } else if (std::is_same<T, std::int64_t>::value) {
    if (hex) {
      hn::Print(hn::ScalableTag<T>(), msg.c_str(), a, 0, 7, " %016x");
    } else {
      hn::Print(hn::ScalableTag<T>(), msg.c_str(), a);
    }
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

// NOLINTNEXTLINE(google-readability-namespace-comments)
} // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

#endif // __VERIFICARLO_SRLIB_DEBUG_HWY_HPP__