#ifndef __SR_HW_INL_H__
#define __SR_HW_INL_H__

#include "hwy/highway.h"
#include "hwy/print-inl.h"
#include "src/utils.hpp"
#include "src/xoroshiro256+_hw.hpp"

#include "hwy/base.h"

template <typename T>
HWY_DLLEXPORT hn::Vec<hn::ScalableTag<T>> sr_add(hn::Vec<hn::ScalableTag<T>> a,
                                                 hn::Vec<hn::ScalableTag<T>> b);

template <typename T>
HWY_DLLEXPORT hn::Vec<hn::ScalableTag<T>> sr_mul(hn::Vec<hn::ScalableTag<T>> a,
                                                 hn::Vec<hn::ScalableTag<T>> b);
template <typename T>
HWY_DLLEXPORT hn::Vec<hn::ScalableTag<T>> sr_div(hn::Vec<hn::ScalableTag<T>> a,
                                                 hn::Vec<hn::ScalableTag<T>> b);
template <typename T>
HWY_DLLEXPORT hn::Vec<hn::ScalableTag<T>>
sr_sqrt(hn::Vec<hn::ScalableTag<T>> a);

#endif // __SR_HW_INL_H__