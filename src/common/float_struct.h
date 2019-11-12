#ifndef __FLOAT_STRUCT_H_
#define __FLOAT_STRUCT_H_

#include <stdint.h>

#include "float_const.h"

typedef union {

  double f64;
  uint64_t u64;
  int64_t s64;
  uint32_t u32[2];

  /* Generic fields */
  double type;
  uint64_t u;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint64_t sign : DOUBLE_SIGN_SIZE;
    uint64_t exponent : DOUBLE_EXP_SIZE;
    uint64_t mantissa : DOUBLE_PMAN_SIZE;
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint64_t mantissa : DOUBLE_PMAN_SIZE;
    uint64_t exponent : DOUBLE_EXP_SIZE;
    uint64_t sign : DOUBLE_SIGN_SIZE;
#endif
  } ieee;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint32_t sign : DOUBLE_SIGN_SIZE;
    uint32_t exponent : DOUBLE_EXP_SIZE;
    uint32_t mantissa_high : DOUBLE_PMAN_HIGH_SIZE;
    uint32_t mantissa_low : DOUBLE_PMAN_LOW_SIZE;
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#if __FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__
    uint32_t mantissa_high : DOUBLE_PMAN_HIGH_SIZE;
    uint32_t exponent : DOUBLE_EXP_SIZE;
    uint32_t sign : DOUBLE_SIGN_SIZE;
    uint32_t mantissa_low : DOUBLE_PMAN_LOW_SIZE;
#else
    uint32_t mantissa_low : DOUBLE_PMAN_LOW_SIZE;
    uint32_t mantissa_high : DOUBLE_PMAN_HIGH_SIZE;
    uint32_t exponent : DOUBLE_EXP_SIZE;
    uint32_t sign : DOUBLE_SIGN_SIZE;
#endif
#endif
  } ieee32;

} binary64;

typedef union {

  float f32;
  uint32_t u32;
  int32_t s32;

  /* Generic fields */
  float type;
  uint32_t u;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint32_t sign : FLOAT_SIGN_SIZE;
    uint32_t exponent : FLOAT_EXP_SIZE;
    uint32_t mantissa : FLOAT_PMAN_SIZE;
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint32_t mantissa : FLOAT_PMAN_SIZE;
    uint32_t exponent : FLOAT_EXP_SIZE;
    uint32_t sign : FLOAT_SIGN_SIZE;
#endif
  } ieee;

} binary32;

#define GET_EXP_MAX(X)                                                         \
  _Generic((X), float : FLOAT_EXP_MAX, double : DOUBLE_EXP_MAX);
#define GET_SIGN_SIZE(X)                                                       \
  _Generic((X), float : FLOAT_SIGN_SIZE, double : DOUBLE_SIGN_SIZE);
#define GET_EXP_SIZE(X)                                                        \
  _Generic((X), float : FLOAT_EXP_SIZE, double : DOUBLE_EXP_SIZE);
#define GET_PMAN_SIZE(X)                                                       \
  _Generic((X), float : FLOAT_PMAN_SIZE, double : DOUBLE_PMAN_SIZE);
#define GET_EXP_MIN(X)                                                         \
  _Generic((X), float : FLOAT_EXP_MIN, double : DOUBLE_EXP_MIN);
#define GET_EXP_COMP(X)                                                        \
  _Generic((X), float : FLOAT_EXP_COMP, double : DOUBLE_EXP_COMP);

#endif /* __FLOAT_STRUCT_H_ */
