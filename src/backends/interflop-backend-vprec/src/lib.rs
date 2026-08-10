#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]

use interflop_stdlib::{interflop_backend_interface_t, interflop_panic_t};
use libc::{c_char, c_int, c_void, FILE};
use std::ffi::CStr;

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum vprec_mode {
    vprecmode_ieee = 0,
    vprecmode_full,
    vprecmode_ib,
    vprecmode_ob,
    _vprecmode_end_,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum vprec_err_mode {
    vprec_err_mode_rel = 0,
    vprec_err_mode_abs,
    vprec_err_mode_all,
    _vprec_err_mode_end_,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct vprec_context_t {
    pub binary32_mantissa: c_int,
    pub binary32_range: c_int,
    pub binary64_mantissa: c_int,
    pub binary64_range: c_int,
    pub mode: vprec_mode,
    pub err_mode: vprec_err_mode,
    pub max_abs_err_exp: c_int,
    pub daz: bool,
    pub ftz: bool,
}

static BACKEND_NAME: &[u8] = b"interflop-vprec\0";
static BACKEND_VERSION: &[u8] = b"1.x-dev\0";

#[no_mangle]
pub extern "C" fn interflop_vprec_get_backend_name() -> *const c_char {
    BACKEND_NAME.as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn interflop_vprec_get_backend_version() -> *const c_char {
    BACKEND_VERSION.as_ptr() as *const c_char
}

fn check_if_binary32_needs_rounding(bits: u32, mantissa_bits: usize) -> bool {
    if mantissa_bits >= 23 {
        return false;
    }
    let shift = 23 - mantissa_bits;
    let trailing_bits_mask = (1u32 << shift) - 1;
    let trailing_bits = bits & trailing_bits_mask;
    let bit_to_round = (bits >> shift) & 1;
    let halfway_point = 1u32 << (shift - 1);
    (trailing_bits > halfway_point) || (trailing_bits == halfway_point && bit_to_round == 1)
}

fn round_binary32(a: f32, ctx: &vprec_context_t) -> f32 {
    if !a.is_finite() || ctx.mode == vprec_mode::vprecmode_ieee {
        return a;
    }
    let mut val = a;
    if ctx.daz && val.is_subnormal() {
        val = if val.is_sign_negative() { -0.0 } else { 0.0 };
    }
    if val == 0.0 {
        return val;
    }

    if ctx.binary32_mantissa < 23 {
        let bits = val.to_bits();
        let raw_exp = ((bits >> 23) & 0xFF) as i32;
        let exp_hulp = raw_exp - ctx.binary32_mantissa - 1;

        let half_ulp_bits = if exp_hulp < 1 {
            let shift = 23 - 1 + exp_hulp;
            if shift < 0 {
                0
            } else {
                (bits & 0x8000_0000) | (1u32 << shift)
            }
        } else {
            (bits & 0x8000_0000) | ((exp_hulp as u32) << 23)
        };
        let half_ulp = f32::from_bits(half_ulp_bits);

        let mut res = val;
        if check_if_binary32_needs_rounding(bits, ctx.binary32_mantissa as usize) {
            res += half_ulp;
        }
        let mask = !((1u32 << (23 - ctx.binary32_mantissa)) - 1);
        val = f32::from_bits(res.to_bits() & mask);
    }

    if ctx.binary32_range < 8 {
        let max_exp = (1 << (ctx.binary32_range - 1)) - 1;
        let min_exp = 1 - max_exp;
        let bits = val.to_bits();
        let sign = bits & 0x8000_0000;
        let raw_exp = ((bits >> 23) & 0xFF) as i32;
        let exp = if raw_exp == 0 { -126 } else { raw_exp - 127 };
        if exp > max_exp {
            val = f32::from_bits(sign | (0xFF << 23));
        } else if exp < min_exp {
            val = f32::from_bits(sign);
        }
    }

    if ctx.ftz && val.is_subnormal() {
        val = if val.is_sign_negative() { -0.0 } else { 0.0 };
    }
    val
}

fn check_if_binary64_needs_rounding(bits: u64, mantissa_bits: usize) -> bool {
    if mantissa_bits >= 52 {
        return false;
    }
    let shift = 52 - mantissa_bits;
    let trailing_bits_mask = (1u64 << shift) - 1;
    let trailing_bits = bits & trailing_bits_mask;
    let bit_to_round = (bits >> shift) & 1;
    let halfway_point = 1u64 << (shift - 1);
    (trailing_bits > halfway_point) || (trailing_bits == halfway_point && bit_to_round == 1)
}

fn round_binary64(a: f64, ctx: &vprec_context_t) -> f64 {
    if !a.is_finite() || ctx.mode == vprec_mode::vprecmode_ieee {
        return a;
    }
    let mut val = a;
    if ctx.daz && val.is_subnormal() {
        val = if val.is_sign_negative() { -0.0 } else { 0.0 };
    }
    if val == 0.0 {
        return val;
    }

    if ctx.binary64_mantissa < 52 {
        let bits = val.to_bits();
        let raw_exp = ((bits >> 52) & 0x7FF) as i32;
        let exp_hulp = raw_exp - ctx.binary64_mantissa - 1;

        let half_ulp_bits = if exp_hulp < 1 {
            let shift = 52 - 1 + exp_hulp;
            if shift < 0 {
                0
            } else {
                (bits & 0x8000_0000_0000_0000) | (1u64 << shift)
            }
        } else {
            (bits & 0x8000_0000_0000_0000) | ((exp_hulp as u64) << 52)
        };
        let half_ulp = f64::from_bits(half_ulp_bits);

        let mut res = val;
        if check_if_binary64_needs_rounding(bits, ctx.binary64_mantissa as usize) {
            res += half_ulp;
        }
        let mask = !((1u64 << (52 - ctx.binary64_mantissa)) - 1);
        val = f64::from_bits(res.to_bits() & mask);
    }

    if ctx.binary64_range < 11 {
        let max_exp = (1 << (ctx.binary64_range - 1)) - 1;
        let min_exp = 1 - max_exp;
        let bits = val.to_bits();
        let sign = bits & 0x8000_0000_0000_0000;
        let raw_exp = ((bits >> 52) & 0x7FF) as i32;
        let exp = if raw_exp == 0 { -1022 } else { raw_exp - 1023 };
        if exp > max_exp {
            val = f64::from_bits(sign | (0x7FFu64 << 52));
        } else if exp < min_exp {
            val = f64::from_bits(sign);
        }
    }

    if ctx.ftz && val.is_subnormal() {
        val = if val.is_sign_negative() { -0.0 } else { 0.0 };
    }
    val
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_add_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    if context.is_null() {
        *c = a + b;
        return;
    }
    let ctx = &*(context as *const vprec_context_t);
    let (in_a, in_b) = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ib => {
            (round_binary32(a, ctx), round_binary32(b, ctx))
        }
        _ => (a, b),
    };
    let res = in_a + in_b;
    *c = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ob => round_binary32(res, ctx),
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_sub_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    if context.is_null() {
        *c = a - b;
        return;
    }
    let ctx = &*(context as *const vprec_context_t);
    let (in_a, in_b) = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ib => {
            (round_binary32(a, ctx), round_binary32(b, ctx))
        }
        _ => (a, b),
    };
    let res = in_a - in_b;
    *c = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ob => round_binary32(res, ctx),
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_mul_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    if context.is_null() {
        *c = a * b;
        return;
    }
    let ctx = &*(context as *const vprec_context_t);
    let (in_a, in_b) = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ib => {
            (round_binary32(a, ctx), round_binary32(b, ctx))
        }
        _ => (a, b),
    };
    let res = in_a * in_b;
    *c = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ob => round_binary32(res, ctx),
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_div_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    if context.is_null() {
        *c = a / b;
        return;
    }
    let ctx = &*(context as *const vprec_context_t);
    let (in_a, in_b) = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ib => {
            (round_binary32(a, ctx), round_binary32(b, ctx))
        }
        _ => (a, b),
    };
    let res = in_a / in_b;
    *c = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ob => round_binary32(res, ctx),
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_add_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    if context.is_null() {
        *c = a + b;
        return;
    }
    let ctx = &*(context as *const vprec_context_t);
    let (in_a, in_b) = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ib => {
            (round_binary64(a, ctx), round_binary64(b, ctx))
        }
        _ => (a, b),
    };
    let res = in_a + in_b;
    *c = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ob => round_binary64(res, ctx),
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_sub_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    if context.is_null() {
        *c = a - b;
        return;
    }
    let ctx = &*(context as *const vprec_context_t);
    let (in_a, in_b) = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ib => {
            (round_binary64(a, ctx), round_binary64(b, ctx))
        }
        _ => (a, b),
    };
    let res = in_a - in_b;
    *c = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ob => round_binary64(res, ctx),
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_mul_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    if context.is_null() {
        *c = a * b;
        return;
    }
    let ctx = &*(context as *const vprec_context_t);
    let (in_a, in_b) = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ib => {
            (round_binary64(a, ctx), round_binary64(b, ctx))
        }
        _ => (a, b),
    };
    let res = in_a * in_b;
    *c = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ob => round_binary64(res, ctx),
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_div_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    if context.is_null() {
        *c = a / b;
        return;
    }
    let ctx = &*(context as *const vprec_context_t);
    let (in_a, in_b) = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ib => {
            (round_binary64(a, ctx), round_binary64(b, ctx))
        }
        _ => (a, b),
    };
    let res = in_a / in_b;
    *c = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ob => round_binary64(res, ctx),
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_cast_double_to_float(
    a: f64,
    b: *mut f32,
    context: *mut c_void,
) {
    if b.is_null() {
        return;
    }
    if context.is_null() {
        *b = a as f32;
        return;
    }
    let ctx = &*(context as *const vprec_context_t);
    let in_a = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ib => round_binary64(a, ctx),
        _ => a,
    };
    let res = in_a as f32;
    *b = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ob => round_binary32(res, ctx),
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_fma_float(
    a: f32,
    b: f32,
    c: f32,
    res: *mut f32,
    context: *mut c_void,
) {
    if res.is_null() {
        return;
    }
    if context.is_null() {
        *res = a.mul_add(b, c);
        return;
    }
    let ctx = &*(context as *const vprec_context_t);
    let (in_a, in_b, in_c) = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ib => (
            round_binary32(a, ctx),
            round_binary32(b, ctx),
            round_binary32(c, ctx),
        ),
        _ => (a, b, c),
    };
    let out = in_a.mul_add(in_b, in_c);
    *res = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ob => round_binary32(out, ctx),
        _ => out,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_fma_double(
    a: f64,
    b: f64,
    c: f64,
    res: *mut f64,
    context: *mut c_void,
) {
    if res.is_null() {
        return;
    }
    if context.is_null() {
        *res = a.mul_add(b, c);
        return;
    }
    let ctx = &*(context as *const vprec_context_t);
    let (in_a, in_b, in_c) = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ib => (
            round_binary64(a, ctx),
            round_binary64(b, ctx),
            round_binary64(c, ctx),
        ),
        _ => (a, b, c),
    };
    let out = in_a.mul_add(in_b, in_c);
    *res = match ctx.mode {
        vprec_mode::vprecmode_full | vprec_mode::vprecmode_ob => round_binary64(out, ctx),
        _ => out,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_pre_init(
    panic: interflop_panic_t,
    stream: *mut FILE,
    context: *mut *mut c_void,
) {
    interflop_stdlib::logger_init(panic, stream, BACKEND_NAME.as_ptr() as *const c_char);
    if !context.is_null() {
        let ctx = Box::new(vprec_context_t {
            binary32_mantissa: 23,
            binary32_range: 8,
            binary64_mantissa: 52,
            binary64_range: 11,
            mode: vprec_mode::vprecmode_ob,
            err_mode: vprec_err_mode::vprec_err_mode_rel,
            max_abs_err_exp: 0,
            daz: false,
            ftz: false,
        });
        *context = Box::into_raw(ctx) as *mut c_void;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_cli(
    argc: c_int,
    argv: *mut *mut c_char,
    context: *mut c_void,
) {
    if context.is_null() || argv.is_null() {
        return;
    }
    let ctx = &mut *(context as *mut vprec_context_t);
    for i in 1..argc as usize {
        let ptr = *argv.add(i);
        if ptr.is_null() {
            continue;
        }
        if let Ok(arg) = CStr::from_ptr(ptr).to_str() {
            if arg.starts_with("--precision-binary32=") {
                if let Ok(val) = arg["--precision-binary32=".len()..].parse::<i32>() {
                    ctx.binary32_mantissa = (val - 1).clamp(0, 23);
                }
            } else if arg.starts_with("--precision-binary64=") {
                if let Ok(val) = arg["--precision-binary64=".len()..].parse::<i32>() {
                    ctx.binary64_mantissa = (val - 1).clamp(0, 52);
                }
            } else if arg.starts_with("--range-binary32=") {
                if let Ok(val) = arg["--range-binary32=".len()..].parse::<i32>() {
                    ctx.binary32_range = val.clamp(2, 8);
                }
            } else if arg.starts_with("--range-binary64=") {
                if let Ok(val) = arg["--range-binary64=".len()..].parse::<i32>() {
                    ctx.binary64_range = val.clamp(2, 11);
                }
            } else if arg.starts_with("--mode=") || arg.starts_with("-m=") {
                let mode_str = arg.split('=').nth(1).unwrap_or("");
                match mode_str {
                    "ieee" => ctx.mode = vprec_mode::vprecmode_ieee,
                    "full" => ctx.mode = vprec_mode::vprecmode_full,
                    "ib" => ctx.mode = vprec_mode::vprecmode_ib,
                    "ob" => ctx.mode = vprec_mode::vprecmode_ob,
                    _ => {}
                }
            } else if arg.starts_with("--preset=") {
                let preset_str = arg["--preset=".len()..].as_ref();
                match preset_str {
                    "binary16" => {
                        ctx.binary32_mantissa = 10;
                        ctx.binary32_range = 5;
                        ctx.binary64_mantissa = 10;
                        ctx.binary64_range = 5;
                    }
                    "binary32" => {
                        ctx.binary32_mantissa = 23;
                        ctx.binary32_range = 8;
                        ctx.binary64_mantissa = 23;
                        ctx.binary64_range = 8;
                    }
                    "bfloat16" => {
                        ctx.binary32_mantissa = 7;
                        ctx.binary32_range = 8;
                        ctx.binary64_mantissa = 7;
                        ctx.binary64_range = 8;
                    }
                    "tensorfloat" => {
                        ctx.binary32_mantissa = 10;
                        ctx.binary32_range = 8;
                        ctx.binary64_mantissa = 10;
                        ctx.binary64_range = 8;
                    }
                    "fp24" => {
                        ctx.binary32_mantissa = 16;
                        ctx.binary32_range = 7;
                        ctx.binary64_mantissa = 16;
                        ctx.binary64_range = 7;
                    }
                    "PXR24" => {
                        ctx.binary32_mantissa = 15;
                        ctx.binary32_range = 8;
                        ctx.binary64_mantissa = 15;
                        ctx.binary64_range = 8;
                    }
                    _ => {}
                }
            } else if arg == "-d" || arg == "--daz" {
                ctx.daz = true;
            } else if arg == "-f" || arg == "--ftz" {
                ctx.ftz = true;
            }
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_configure(configure: *mut c_void, context: *mut c_void) {
    if !configure.is_null() && !context.is_null() {
        let conf = &*(configure as *const vprec_context_t);
        let ctx = &mut *(context as *mut vprec_context_t);
        ctx.binary32_mantissa = conf.binary32_mantissa;
        ctx.binary32_range = conf.binary32_range;
        ctx.binary64_mantissa = conf.binary64_mantissa;
        ctx.binary64_range = conf.binary64_range;
        ctx.mode = conf.mode;
        ctx.err_mode = conf.err_mode;
        ctx.max_abs_err_exp = conf.max_abs_err_exp;
        ctx.daz = conf.daz;
        ctx.ftz = conf.ftz;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_vprec_init(
    _context: *mut c_void,
) -> interflop_backend_interface_t {
    interflop_backend_interface_t {
        interflop_add_float: Some(interflop_vprec_add_float),
        interflop_sub_float: Some(interflop_vprec_sub_float),
        interflop_mul_float: Some(interflop_vprec_mul_float),
        interflop_div_float: Some(interflop_vprec_div_float),
        interflop_cmp_float: None,
        interflop_add_double: Some(interflop_vprec_add_double),
        interflop_sub_double: Some(interflop_vprec_sub_double),
        interflop_mul_double: Some(interflop_vprec_mul_double),
        interflop_div_double: Some(interflop_vprec_div_double),
        interflop_cmp_double: None,
        interflop_cast_double_to_float: Some(interflop_vprec_cast_double_to_float),
        interflop_fma_float: Some(interflop_vprec_fma_float),
        interflop_fma_double: Some(interflop_vprec_fma_double),
        interflop_enter_function: None,
        interflop_exit_function: None,
        interflop_user_call: None,
        interflop_finalize: None,
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_init(context: *mut c_void) -> interflop_backend_interface_t {
    interflop_vprec_init(context)
}

#[no_mangle]
pub unsafe extern "C" fn interflop_pre_init(
    panic: interflop_panic_t,
    stream: *mut FILE,
    context: *mut *mut c_void,
) {
    interflop_vprec_pre_init(panic, stream, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cli(argc: c_int, argv: *mut *mut c_char, context: *mut c_void) {
    interflop_vprec_cli(argc, argv, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_configure(configure: *mut c_void, context: *mut c_void) {
    interflop_vprec_configure(configure, context);
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_name() -> *const c_char {
    interflop_vprec_get_backend_name()
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_version() -> *const c_char {
    interflop_vprec_get_backend_version()
}
