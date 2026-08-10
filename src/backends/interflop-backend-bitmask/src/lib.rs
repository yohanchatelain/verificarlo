#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]

use interflop_stdlib::{
    get_rand_uint64, interflop_backend_interface_t, interflop_panic_t, rng_state_t,
};
use libc::{c_char, c_int, c_void, pid_t, FILE};
use std::cell::RefCell;
use std::ffi::CStr;

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum bitmask_mode {
    bitmask_mode_ieee = 0,
    bitmask_mode_full,
    bitmask_mode_ib,
    bitmask_mode_ob,
    _bitmask_mode_end_,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum bitmask_operator {
    bitmask_operator_zero = 0,
    bitmask_operator_one,
    bitmask_operator_rand,
    _bitmask_operator_end_,
}

#[repr(C)]
pub struct bitmask_context_t {
    pub seed: u64,
    pub binary32_mantissa: c_int,
    pub binary64_mantissa: c_int,
    pub operator: bitmask_operator,
    pub mode: bitmask_mode,
    pub choose_seed: bool,
    pub daz: bool,
    pub ftz: bool,
}

static BACKEND_NAME: &[u8] = b"interflop-bitmask\0";
static BACKEND_VERSION: &[u8] = b"1.x-dev\0";
static mut GLOBAL_TID: pid_t = 0;

thread_local! {
    static THREAD_RNG_STATE: RefCell<rng_state_t> = RefCell::new(rng_state_t {
        choose_seed: false,
        seed: 0,
        random_state_valid: false,
        random_state: [0, 0],
    });
}

#[no_mangle]
pub extern "C" fn interflop_bitmask_get_backend_name() -> *const c_char {
    BACKEND_NAME.as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn interflop_bitmask_get_backend_version() -> *const c_char {
    BACKEND_VERSION.as_ptr() as *const c_char
}

fn apply_bitmask_f32(
    val: f32,
    mantissa_bits: c_int,
    op: bitmask_operator,
    ctx: &bitmask_context_t,
) -> f32 {
    if mantissa_bits >= 23 {
        return val;
    }
    let mut bits = val.to_bits();
    let mask = !((1u32 << (23 - mantissa_bits)) - 1);
    match op {
        bitmask_operator::bitmask_operator_zero => {
            bits &= mask;
        }
        bitmask_operator::bitmask_operator_one => {
            bits |= !mask & 0x007FFFFF;
        }
        bitmask_operator::bitmask_operator_rand => {
            THREAD_RNG_STATE.with(|state_cell| {
                let mut rng = state_cell.borrow_mut();
                if !rng.random_state_valid {
                    rng.choose_seed = ctx.choose_seed;
                    rng.seed = ctx.seed;
                }
                unsafe {
                    let r = get_rand_uint64(&mut *rng, &raw mut GLOBAL_TID) as u32;
                    bits = (bits & mask) | (r & !mask & 0x007FFFFF);
                }
            });
        }
        _ => {}
    }
    f32::from_bits(bits)
}

fn apply_bitmask_f64(
    val: f64,
    mantissa_bits: c_int,
    op: bitmask_operator,
    ctx: &bitmask_context_t,
) -> f64 {
    if mantissa_bits >= 52 {
        return val;
    }
    let mut bits = val.to_bits();
    let mask = !((1u64 << (52 - mantissa_bits)) - 1);
    match op {
        bitmask_operator::bitmask_operator_zero => {
            bits &= mask;
        }
        bitmask_operator::bitmask_operator_one => {
            bits |= !mask & 0x000FFFFFFFFFFFFF;
        }
        bitmask_operator::bitmask_operator_rand => {
            THREAD_RNG_STATE.with(|state_cell| {
                let mut rng = state_cell.borrow_mut();
                if !rng.random_state_valid {
                    rng.choose_seed = ctx.choose_seed;
                    rng.seed = ctx.seed;
                }
                unsafe {
                    let r = get_rand_uint64(&mut *rng, &raw mut GLOBAL_TID);
                    bits = (bits & mask) | (r & !mask & 0x000FFFFFFFFFFFFF);
                }
            });
        }
        _ => {}
    }
    f64::from_bits(bits)
}

#[no_mangle]
pub unsafe extern "C" fn interflop_bitmask_add_float(
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
    let ctx = &*(context as *const bitmask_context_t);
    let (in_a, in_b) = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ib => (
            apply_bitmask_f32(a, ctx.binary32_mantissa, ctx.operator, ctx),
            apply_bitmask_f32(b, ctx.binary32_mantissa, ctx.operator, ctx),
        ),
        _ => (a, b),
    };
    let res = in_a + in_b;
    *c = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ob => {
            apply_bitmask_f32(res, ctx.binary32_mantissa, ctx.operator, ctx)
        }
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_bitmask_sub_float(
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
    let ctx = &*(context as *const bitmask_context_t);
    let (in_a, in_b) = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ib => (
            apply_bitmask_f32(a, ctx.binary32_mantissa, ctx.operator, ctx),
            apply_bitmask_f32(b, ctx.binary32_mantissa, ctx.operator, ctx),
        ),
        _ => (a, b),
    };
    let res = in_a - in_b;
    *c = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ob => {
            apply_bitmask_f32(res, ctx.binary32_mantissa, ctx.operator, ctx)
        }
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_bitmask_mul_float(
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
    let ctx = &*(context as *const bitmask_context_t);
    let (in_a, in_b) = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ib => (
            apply_bitmask_f32(a, ctx.binary32_mantissa, ctx.operator, ctx),
            apply_bitmask_f32(b, ctx.binary32_mantissa, ctx.operator, ctx),
        ),
        _ => (a, b),
    };
    let res = in_a * in_b;
    *c = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ob => {
            apply_bitmask_f32(res, ctx.binary32_mantissa, ctx.operator, ctx)
        }
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_bitmask_div_float(
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
    let ctx = &*(context as *const bitmask_context_t);
    let (in_a, in_b) = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ib => (
            apply_bitmask_f32(a, ctx.binary32_mantissa, ctx.operator, ctx),
            apply_bitmask_f32(b, ctx.binary32_mantissa, ctx.operator, ctx),
        ),
        _ => (a, b),
    };
    let res = in_a / in_b;
    *c = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ob => {
            apply_bitmask_f32(res, ctx.binary32_mantissa, ctx.operator, ctx)
        }
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_bitmask_add_double(
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
    let ctx = &*(context as *const bitmask_context_t);
    let (in_a, in_b) = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ib => (
            apply_bitmask_f64(a, ctx.binary64_mantissa, ctx.operator, ctx),
            apply_bitmask_f64(b, ctx.binary64_mantissa, ctx.operator, ctx),
        ),
        _ => (a, b),
    };
    let res = in_a + in_b;
    *c = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ob => {
            apply_bitmask_f64(res, ctx.binary64_mantissa, ctx.operator, ctx)
        }
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_bitmask_sub_double(
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
    let ctx = &*(context as *const bitmask_context_t);
    let (in_a, in_b) = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ib => (
            apply_bitmask_f64(a, ctx.binary64_mantissa, ctx.operator, ctx),
            apply_bitmask_f64(b, ctx.binary64_mantissa, ctx.operator, ctx),
        ),
        _ => (a, b),
    };
    let res = in_a - in_b;
    *c = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ob => {
            apply_bitmask_f64(res, ctx.binary64_mantissa, ctx.operator, ctx)
        }
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_bitmask_mul_double(
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
    let ctx = &*(context as *const bitmask_context_t);
    let (in_a, in_b) = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ib => (
            apply_bitmask_f64(a, ctx.binary64_mantissa, ctx.operator, ctx),
            apply_bitmask_f64(b, ctx.binary64_mantissa, ctx.operator, ctx),
        ),
        _ => (a, b),
    };
    let res = in_a * in_b;
    *c = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ob => {
            apply_bitmask_f64(res, ctx.binary64_mantissa, ctx.operator, ctx)
        }
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_bitmask_div_double(
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
    let ctx = &*(context as *const bitmask_context_t);
    let (in_a, in_b) = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ib => (
            apply_bitmask_f64(a, ctx.binary64_mantissa, ctx.operator, ctx),
            apply_bitmask_f64(b, ctx.binary64_mantissa, ctx.operator, ctx),
        ),
        _ => (a, b),
    };
    let res = in_a / in_b;
    *c = match ctx.mode {
        bitmask_mode::bitmask_mode_full | bitmask_mode::bitmask_mode_ob => {
            apply_bitmask_f64(res, ctx.binary64_mantissa, ctx.operator, ctx)
        }
        _ => res,
    };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_bitmask_pre_init(
    panic: interflop_panic_t,
    stream: *mut FILE,
    context: *mut *mut c_void,
) {
    interflop_stdlib::logger_init(panic, stream, BACKEND_NAME.as_ptr() as *const c_char);
    if !context.is_null() {
        let ctx = Box::new(bitmask_context_t {
            seed: 0,
            binary32_mantissa: 23,
            binary64_mantissa: 52,
            operator: bitmask_operator::bitmask_operator_zero,
            mode: bitmask_mode::bitmask_mode_ob,
            choose_seed: false,
            daz: false,
            ftz: false,
        });
        *context = Box::into_raw(ctx) as *mut c_void;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_bitmask_cli(
    argc: c_int,
    argv: *mut *mut c_char,
    context: *mut c_void,
) {
    if context.is_null() || argv.is_null() {
        return;
    }
    let ctx = &mut *(context as *mut bitmask_context_t);
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
            } else if arg.starts_with("--operator=") || arg.starts_with("-o=") {
                let op_val = if arg.starts_with("--operator=") {
                    &arg["--operator=".len()..]
                } else {
                    &arg["-o=".len()..]
                };
                if op_val.eq_ignore_ascii_case("zero") {
                    ctx.operator = bitmask_operator::bitmask_operator_zero;
                } else if op_val.eq_ignore_ascii_case("one") {
                    ctx.operator = bitmask_operator::bitmask_operator_one;
                } else if op_val.eq_ignore_ascii_case("rand") {
                    ctx.operator = bitmask_operator::bitmask_operator_rand;
                }
            } else if arg.starts_with("--mode=") || arg.starts_with("-m=") {
                let mode_val = if arg.starts_with("--mode=") {
                    &arg["--mode=".len()..]
                } else {
                    &arg["-m=".len()..]
                };
                if mode_val.eq_ignore_ascii_case("ieee") {
                    ctx.mode = bitmask_mode::bitmask_mode_ieee;
                } else if mode_val.eq_ignore_ascii_case("full") {
                    ctx.mode = bitmask_mode::bitmask_mode_full;
                } else if mode_val.eq_ignore_ascii_case("ib") {
                    ctx.mode = bitmask_mode::bitmask_mode_ib;
                } else if mode_val.eq_ignore_ascii_case("ob") {
                    ctx.mode = bitmask_mode::bitmask_mode_ob;
                }
            } else if arg.starts_with("--seed=") || arg.starts_with("-s=") {
                let seed_val = if arg.starts_with("--seed=") {
                    &arg["--seed=".len()..]
                } else {
                    &arg["-s=".len()..]
                };
                if let Ok(val) = seed_val.parse::<u64>() {
                    ctx.seed = val;
                    ctx.choose_seed = true;
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
pub unsafe extern "C" fn interflop_bitmask_configure(configure: *mut c_void, context: *mut c_void) {
    if !configure.is_null() && !context.is_null() {
        let conf = &*(configure as *const bitmask_context_t);
        let ctx = &mut *(context as *mut bitmask_context_t);
        ctx.seed = conf.seed;
        ctx.binary32_mantissa = conf.binary32_mantissa;
        ctx.binary64_mantissa = conf.binary64_mantissa;
        ctx.operator = conf.operator;
        ctx.mode = conf.mode;
        ctx.choose_seed = conf.choose_seed;
        ctx.daz = conf.daz;
        ctx.ftz = conf.ftz;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_bitmask_init(
    _context: *mut c_void,
) -> interflop_backend_interface_t {
    interflop_backend_interface_t {
        interflop_add_float: Some(interflop_bitmask_add_float),
        interflop_sub_float: Some(interflop_bitmask_sub_float),
        interflop_mul_float: Some(interflop_bitmask_mul_float),
        interflop_div_float: Some(interflop_bitmask_div_float),
        interflop_cmp_float: None,
        interflop_add_double: Some(interflop_bitmask_add_double),
        interflop_sub_double: Some(interflop_bitmask_sub_double),
        interflop_mul_double: Some(interflop_bitmask_mul_double),
        interflop_div_double: Some(interflop_bitmask_div_double),
        interflop_cmp_double: None,
        interflop_cast_double_to_float: None,
        interflop_fma_float: None,
        interflop_fma_double: None,
        interflop_enter_function: None,
        interflop_exit_function: None,
        interflop_user_call: None,
        interflop_finalize: None,
    }
}

// C-ABI Alias exports
#[no_mangle]
pub unsafe extern "C" fn interflop_init(context: *mut c_void) -> interflop_backend_interface_t {
    interflop_bitmask_init(context)
}

#[no_mangle]
pub unsafe extern "C" fn interflop_pre_init(
    panic: interflop_panic_t,
    stream: *mut FILE,
    context: *mut *mut c_void,
) {
    interflop_bitmask_pre_init(panic, stream, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cli(argc: c_int, argv: *mut *mut c_char, context: *mut c_void) {
    interflop_bitmask_cli(argc, argv, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_configure(configure: *mut c_void, context: *mut c_void) {
    interflop_bitmask_configure(configure, context);
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_name() -> *const c_char {
    interflop_bitmask_get_backend_name()
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_version() -> *const c_char {
    interflop_bitmask_get_backend_version()
}
