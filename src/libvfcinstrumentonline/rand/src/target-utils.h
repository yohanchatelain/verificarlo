#include <iostream>
#include <set>

#if defined(HIGHWAY_HWY_SRLIB_RAND_TARGET_UTILS_H_) ==                         \
    defined(HWY_TARGET_TOGGLE) // NOLINT
#ifdef HIGHWAY_HWY_SRLIB_RAND_TARGET_UTILS_H_
#undef HIGHWAY_HWY_SRLIB_RAND_TARGET_UTILS_H_
#else
#define HIGHWAY_HWY_SRLIB_RAND_TARGET_UTILS_H_
#endif

#include "hwy/highway.h"

HWY_BEFORE_NAMESPACE(); // at file scope
namespace prism {

namespace HWY_NAMESPACE {

inline std::set<std::string> GetTargetsAsString(int64_t targets) {
  std::set<std::string> result;
  for (int64_t x = targets & hwy::LimitsMax<int64_t>(); x != 0;
       x = x & (x - 1)) {
    result.insert(hwy::TargetName(x & (~x + 1)));
  }
  return result;
}

// // disable warning for unused function
// HWY_MAYBE_UNUSED void PrintTargetsSupported() {
//   const auto targets = GetTargetsAsString(hwy::SupportedTargets());
//   const auto compiled_targets = GetTargetsAsString(HWY_TARGETS);
//   std::cerr << "Supported HWY_TARGETS:\n";
//   for (const auto &compiled_target : compiled_targets) {
//     const auto is_in = targets.find(compiled_target) != targets.end();
//     std::cerr << compiled_target << " " << is_in << "\n";
//   }
// }

HWY_API bool isCurrentTargetSupported() {
  const auto targets = GetTargetsAsString(hwy::SupportedTargets());
  const auto current_target = hwy::TargetName(HWY_TARGET);
#ifdef SR_DEBUG
  std::cerr << "Current target: " << current_target << " is supported: "
            << (targets.find(current_target) != targets.end()) << "\n ";
#endif
  return targets.find(current_target) != targets.end();
}

} // namespace HWY_NAMESPACE

} // namespace prism
HWY_AFTER_NAMESPACE();

#endif // __SR_TARGET_UTILS_H__