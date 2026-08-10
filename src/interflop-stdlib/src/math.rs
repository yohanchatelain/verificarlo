use libc::c_int;

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum InterflopFpClassify {
    IFP_NAN,
    IFP_INFINITE,
    IFP_ZERO,
    IFP_SUBNORMAL,
    IFS_NORMAL,
}

const FLOAT_EXP_MAX: u32 = 0x7F800000;
const FLOAT_GET_EXP: u32 = 0x7F800000;
const FLOAT_GET_PMAN: u32 = 0x007FFFFF;
const FLOAT_EXP_COMP: i32 = 127;
const FLOAT_PMAN_SIZE: i32 = 23;
const FLOAT_PLUS_INF: u32 = 0x7F800000;
const FLOAT_EXP_INF: i32 = 255;

const DOUBLE_GET_EXP: u64 = 0x7FF0000000000000;
const DOUBLE_GET_PMAN: u64 = 0x000FFFFFFFFFFFFF;
const DOUBLE_EXP_COMP: i64 = 1023;
const DOUBLE_PMAN_SIZE: i64 = 52;
const DOUBLE_PLUS_INF: u64 = 0x7FF0000000000000;
const DOUBLE_EXP_INF: i64 = 2047;

#[no_mangle]
pub extern "C" fn interflop_fpclassifyf(x: f32) -> InterflopFpClassify {
    let bits = x.to_bits();
    let exp = bits & FLOAT_GET_EXP;
    let mant = bits & FLOAT_GET_PMAN;
    if exp == 0 && mant == 0 {
        InterflopFpClassify::IFP_ZERO
    } else if exp == FLOAT_EXP_MAX && mant == 0 {
        InterflopFpClassify::IFP_INFINITE
    } else if exp == FLOAT_EXP_MAX && mant != 0 {
        InterflopFpClassify::IFP_NAN
    } else if exp == 0 && mant != 0 {
        InterflopFpClassify::IFP_SUBNORMAL
    } else {
        InterflopFpClassify::IFS_NORMAL
    }
}

#[no_mangle]
pub extern "C" fn interflop_fpclassifyd(x: f64) -> InterflopFpClassify {
    let bits = x.to_bits();
    let exp = bits & DOUBLE_GET_EXP;
    let mant = bits & DOUBLE_GET_PMAN;
    if exp == 0 && mant == 0 {
        InterflopFpClassify::IFP_ZERO
    } else if exp == DOUBLE_GET_EXP && mant == 0 {
        InterflopFpClassify::IFP_INFINITE
    } else if exp == DOUBLE_GET_EXP && mant != 0 {
        InterflopFpClassify::IFP_NAN
    } else if exp == 0 && mant != 0 {
        InterflopFpClassify::IFP_SUBNORMAL
    } else {
        InterflopFpClassify::IFS_NORMAL
    }
}

#[no_mangle]
pub extern "C" fn fpow2i(i: c_int) -> f32 {
    let exp = i + FLOAT_EXP_COMP;
    if exp <= -FLOAT_PMAN_SIZE {
        0.0f32
    } else if exp >= FLOAT_EXP_INF {
        f32::from_bits(FLOAT_PLUS_INF)
    } else if exp <= 0 {
        let shift = FLOAT_PMAN_SIZE - 1 + exp;
        if shift >= 0 {
            f32::from_bits(1u32 << shift)
        } else {
            0.0f32
        }
    } else {
        f32::from_bits((exp as u32) << FLOAT_PMAN_SIZE)
    }
}

#[no_mangle]
pub extern "C" fn pow2i(i: c_int) -> f64 {
    let exp = (i as i64) + DOUBLE_EXP_COMP;
    if exp <= -DOUBLE_PMAN_SIZE {
        0.0f64
    } else if exp >= DOUBLE_EXP_INF {
        f64::from_bits(DOUBLE_PLUS_INF)
    } else if exp <= 0 {
        let shift = DOUBLE_PMAN_SIZE - 1 + exp;
        if shift >= 0 {
            f64::from_bits(1u64 << shift)
        } else {
            0.0f64
        }
    } else {
        f64::from_bits((exp as u64) << DOUBLE_PMAN_SIZE)
    }
}

#[no_mangle]
pub extern "C" fn interflop_isnanf(x: f32) -> c_int {
    if x.is_nan() {
        1
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn interflop_isnand(x: f64) -> c_int {
    if x.is_nan() {
        1
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn interflop_isinff(x: f32) -> c_int {
    if x.is_infinite() {
        1
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn interflop_isinfd(x: f64) -> c_int {
    if x.is_infinite() {
        1
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn interflop_floorf(x: f32) -> f32 {
    x.floor()
}

#[no_mangle]
pub extern "C" fn interflop_floord(x: f64) -> f64 {
    x.floor()
}

#[no_mangle]
pub extern "C" fn interflop_ceilf(x: f32) -> f32 {
    x.ceil()
}

#[no_mangle]
pub extern "C" fn interflop_ceild(x: f64) -> f64 {
    x.ceil()
}
