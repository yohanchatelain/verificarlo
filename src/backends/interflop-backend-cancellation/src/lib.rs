#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]

use interflop_stdlib::{
    interflop_backend_interface_t, interflop_panic_t, rng_state_t, IBool, IFalse, ITrue,
};
use libc::{c_char, c_int, c_void, pid_t, FILE};
use std::cell::RefCell;
use std::ffi::{CStr, CString};

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct cancellation_context_t {
    pub seed: u64,
    pub tolerance: c_int,
    pub choose_seed: IBool,
    pub warning: IBool,
}

static BACKEND_NAME: &[u8] = b"cancellation\0";
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
pub extern "C" fn interflop_cancellation_get_backend_name() -> *const c_char {
    BACKEND_NAME.as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn interflop_cancellation_get_backend_version() -> *const c_char {
    BACKEND_VERSION.as_ptr() as *const c_char
}

fn get_exp_f32(x: f32) -> i32 {
    let bits = x.to_bits();
    (((bits >> 23) & 0xFF) as i32) - 127
}

fn get_exp_f64(x: f64) -> i32 {
    let bits = x.to_bits();
    (((bits >> 52) & 0x7FF) as i32) - 1023
}

fn noise_binary64(exp: i32, rng_state: &mut rng_state_t, tid: *mut pid_t) -> f64 {
    let d_rand = unsafe { interflop_stdlib::get_rand_double01(rng_state, tid) } - 0.5;
    let mut bits = d_rand.to_bits();
    let old_exp = ((bits >> 52) & 0x7FF) as i32;
    let new_exp = (old_exp + exp) as u64;
    bits = (bits & !(0x7FFu64 << 52)) | ((new_exp & 0x7FF) << 52);
    f64::from_bits(bits)
}

fn cancell_f32(a: f32, b: f32, res: &mut f32, ctx: &cancellation_context_t) {
    if !res.is_finite() {
        return;
    }
    let e_z = get_exp_f32(*res);
    let max_op_exp = get_exp_f32(a).max(get_exp_f32(b));
    let cancellation = max_op_exp - e_z;
    if cancellation >= ctx.tolerance {
        if ctx.warning != IFalse {
            unsafe {
                interflop_stdlib::logger_info(
                    b"cancellation of size %d detected\n\0".as_ptr() as *const c_char,
                    cancellation,
                );
            }
        }
        let e_n = e_z - (cancellation - 1);
        THREAD_RNG_STATE.with(|cell| {
            let mut rng = cell.borrow_mut();
            unsafe {
                interflop_stdlib::_init_rng_state_struct(
                    &mut *rng,
                    ctx.choose_seed != IFalse,
                    ctx.seed,
                    false,
                );
                let noise = noise_binary64(e_n, &mut *rng, &raw mut GLOBAL_TID);
                *res = ((*res as f64) + noise) as f32;
            }
        });
    }
}

fn cancell_f64(a: f64, b: f64, res: &mut f64, ctx: &cancellation_context_t) {
    if !res.is_finite() {
        return;
    }
    let e_z = get_exp_f64(*res);
    let max_op_exp = get_exp_f64(a).max(get_exp_f64(b));
    let cancellation = max_op_exp - e_z;
    if cancellation >= ctx.tolerance {
        if ctx.warning != IFalse {
            unsafe {
                interflop_stdlib::logger_info(
                    b"cancellation of size %d detected\n\0".as_ptr() as *const c_char,
                    cancellation,
                );
            }
        }
        let e_n = e_z - (cancellation - 1);
        THREAD_RNG_STATE.with(|cell| {
            let mut rng = cell.borrow_mut();
            unsafe {
                interflop_stdlib::_init_rng_state_struct(
                    &mut *rng,
                    ctx.choose_seed != IFalse,
                    ctx.seed,
                    false,
                );
                let noise = noise_binary64(e_n, &mut *rng, &raw mut GLOBAL_TID);
                *res += noise;
            }
        });
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_add_float(
    a: f32,
    b: f32,
    res: *mut f32,
    context: *mut c_void,
) {
    if res.is_null() {
        return;
    }
    *res = a + b;
    if !context.is_null() {
        let ctx = &*(context as *const cancellation_context_t);
        cancell_f32(a, b, &mut *res, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_sub_float(
    a: f32,
    b: f32,
    res: *mut f32,
    context: *mut c_void,
) {
    if res.is_null() {
        return;
    }
    *res = a - b;
    if !context.is_null() {
        let ctx = &*(context as *const cancellation_context_t);
        cancell_f32(a, b, &mut *res, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_mul_float(
    a: f32,
    b: f32,
    res: *mut f32,
    _context: *mut c_void,
) {
    if !res.is_null() {
        *res = a * b;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_div_float(
    a: f32,
    b: f32,
    res: *mut f32,
    _context: *mut c_void,
) {
    if !res.is_null() {
        *res = a / b;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_add_double(
    a: f64,
    b: f64,
    res: *mut f64,
    context: *mut c_void,
) {
    if res.is_null() {
        return;
    }
    *res = a + b;
    if !context.is_null() {
        let ctx = &*(context as *const cancellation_context_t);
        cancell_f64(a, b, &mut *res, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_sub_double(
    a: f64,
    b: f64,
    res: *mut f64,
    context: *mut c_void,
) {
    if res.is_null() {
        return;
    }
    *res = a - b;
    if !context.is_null() {
        let ctx = &*(context as *const cancellation_context_t);
        cancell_f64(a, b, &mut *res, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_mul_double(
    a: f64,
    b: f64,
    res: *mut f64,
    _context: *mut c_void,
) {
    if !res.is_null() {
        *res = a * b;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_div_double(
    a: f64,
    b: f64,
    res: *mut f64,
    _context: *mut c_void,
) {
    if !res.is_null() {
        *res = a / b;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_fma_float(
    a: f32,
    b: f32,
    c: f32,
    res: *mut f32,
    context: *mut c_void,
) {
    if res.is_null() {
        return;
    }
    *res = a.mul_add(b, c);
    if !context.is_null() {
        let ctx = &*(context as *const cancellation_context_t);
        cancell_f32(a * b, c, &mut *res, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_fma_double(
    a: f64,
    b: f64,
    c: f64,
    res: *mut f64,
    context: *mut c_void,
) {
    if res.is_null() {
        return;
    }
    *res = a.mul_add(b, c);
    if !context.is_null() {
        let ctx = &*(context as *const cancellation_context_t);
        cancell_f64(a * b, c, &mut *res, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_pre_init(
    panic: interflop_panic_t,
    stream: *mut FILE,
    context: *mut *mut c_void,
) {
    interflop_stdlib::logger_init(panic, stream, BACKEND_NAME.as_ptr() as *const c_char);
    if !context.is_null() {
        let ctx = Box::new(cancellation_context_t {
            seed: 0,
            tolerance: 1,
            choose_seed: IFalse,
            warning: IFalse,
        });
        *context = Box::into_raw(ctx) as *mut c_void;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_cli(
    argc: c_int,
    argv: *mut *mut c_char,
    context: *mut c_void,
) {
    if context.is_null() || argv.is_null() {
        return;
    }
    let ctx = &mut *(context as *mut cancellation_context_t);
    let mut i = 1;
    while i < argc as usize {
        let ptr = *argv.add(i);
        if ptr.is_null() {
            i += 1;
            continue;
        }
        if let Ok(arg) = CStr::from_ptr(ptr).to_str() {
            if arg.starts_with("--tolerance=") || arg.starts_with("-t=") {
                let val_str = arg.split('=').nth(1).unwrap_or("");
                if let Ok(val) = val_str.parse::<i32>() {
                    ctx.tolerance = val;
                }
            } else if arg == "--tolerance" || arg == "-t" {
                if i + 1 < argc as usize {
                    let next_ptr = *argv.add(i + 1);
                    if !next_ptr.is_null() {
                        if let Ok(val_str) = CStr::from_ptr(next_ptr).to_str() {
                            if let Ok(val) = val_str.parse::<i32>() {
                                ctx.tolerance = val;
                                i += 1;
                            }
                        }
                    }
                }
            } else if arg.starts_with("--warning") || arg.starts_with("-w") {
                ctx.warning = ITrue;
            } else if arg.starts_with("--seed=") || arg.starts_with("-s=") {
                let val_str = arg.split('=').nth(1).unwrap_or("");
                if let Ok(val) = val_str.parse::<u64>() {
                    ctx.seed = val;
                    ctx.choose_seed = ITrue;
                }
            } else if arg == "--seed" || arg == "-s" {
                if i + 1 < argc as usize {
                    let next_ptr = *argv.add(i + 1);
                    if !next_ptr.is_null() {
                        if let Ok(val_str) = CStr::from_ptr(next_ptr).to_str() {
                            if let Ok(val) = val_str.parse::<u64>() {
                                ctx.seed = val;
                                ctx.choose_seed = ITrue;
                                i += 1;
                            }
                        }
                    }
                }
            }
        }
        i += 1;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_configure(
    configure: *mut c_void,
    context: *mut c_void,
) {
    if !configure.is_null() && !context.is_null() {
        let conf = &*(configure as *const cancellation_context_t);
        let ctx = &mut *(context as *mut cancellation_context_t);
        ctx.seed = conf.seed;
        ctx.tolerance = conf.tolerance;
        ctx.warning = conf.warning;
        ctx.choose_seed = conf.choose_seed;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cancellation_init(
    _context: *mut c_void,
) -> interflop_backend_interface_t {
    interflop_backend_interface_t {
        interflop_add_float: Some(interflop_cancellation_add_float),
        interflop_sub_float: Some(interflop_cancellation_sub_float),
        interflop_mul_float: Some(interflop_cancellation_mul_float),
        interflop_div_float: Some(interflop_cancellation_div_float),
        interflop_cmp_float: None,
        interflop_add_double: Some(interflop_cancellation_add_double),
        interflop_sub_double: Some(interflop_cancellation_sub_double),
        interflop_mul_double: Some(interflop_cancellation_mul_double),
        interflop_div_double: Some(interflop_cancellation_div_double),
        interflop_cmp_double: None,
        interflop_cast_double_to_float: None,
        interflop_fma_float: Some(interflop_cancellation_fma_float),
        interflop_fma_double: Some(interflop_cancellation_fma_double),
        interflop_enter_function: None,
        interflop_exit_function: None,
        interflop_user_call: None,
        interflop_finalize: None,
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_init(context: *mut c_void) -> interflop_backend_interface_t {
    interflop_cancellation_init(context)
}

#[no_mangle]
pub unsafe extern "C" fn interflop_pre_init(
    panic: interflop_panic_t,
    stream: *mut FILE,
    context: *mut *mut c_void,
) {
    interflop_cancellation_pre_init(panic, stream, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cli(argc: c_int, argv: *mut *mut c_char, context: *mut c_void) {
    interflop_cancellation_cli(argc, argv, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_configure(configure: *mut c_void, context: *mut c_void) {
    interflop_cancellation_configure(configure, context);
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_name() -> *const c_char {
    interflop_cancellation_get_backend_name()
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_version() -> *const c_char {
    interflop_cancellation_get_backend_version()
}
