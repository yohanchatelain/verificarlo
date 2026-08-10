#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]

use interflop_stdlib::{
    get_rand_uint64, interflop_backend_interface_t, interflop_panic_t, rng_state_t,
};
use libc::{c_char, c_int, c_void, pid_t, FILE};
use std::cell::RefCell;
use std::ffi::CStr;

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum mcaquad_mode {
    mcaquad_mode_ieee = 0,
    mcaquad_mode_mca,
    mcaquad_mode_pb,
    mcaquad_mode_rr,
    _mcaquad_mode_end_,
}

#[repr(C)]
pub struct mcaquad_context_t {
    pub seed: u64,
    pub binary32_precision: c_int,
    pub binary64_precision: c_int,
    pub mode: mcaquad_mode,
    pub choose_seed: bool,
    pub daz: bool,
    pub ftz: bool,
    pub sparsity: f32,
}

static BACKEND_NAME: &[u8] = b"interflop-mcaquad\0";
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
pub extern "C" fn interflop_mca_get_backend_name() -> *const c_char {
    BACKEND_NAME.as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn interflop_mca_get_backend_version() -> *const c_char {
    BACKEND_VERSION.as_ptr() as *const c_char
}

fn get_exp_f64(x: f64) -> i32 {
    let bits = x.to_bits();
    ((bits >> 52) & 0x7FF) as i32 - 1023
}

fn get_exp_f32(x: f32) -> i32 {
    let bits = x.to_bits();
    ((bits >> 23) & 0xFF) as i32 - 127
}

unsafe fn noise_f64(x: &mut f64, exp: i32, ctx: &mcaquad_context_t) {
    if ctx.mode == mcaquad_mode::mcaquad_mode_ieee || !x.is_finite() || *x == 0.0 {
        return;
    }
    THREAD_RNG_STATE.with(|cell| {
        let mut rng = cell.borrow_mut();
        if !rng.random_state_valid {
            rng.choose_seed = ctx.choose_seed;
            rng.seed = ctx.seed;
        }
        let r = get_rand_uint64(&mut *rng, &raw mut GLOBAL_TID) as i64;
        let shift = (1 + 11 - exp).clamp(0, 63);
        let noise = r >> shift;
        let bits = x.to_bits().wrapping_add(noise as u64);
        *x = f64::from_bits(bits);
    });
}

unsafe fn noise_f32(x: &mut f32, exp: i32, ctx: &mcaquad_context_t) {
    if ctx.mode == mcaquad_mode::mcaquad_mode_ieee || !x.is_finite() || *x == 0.0 {
        return;
    }
    let mut d = *x as f64;
    noise_f64(&mut d, exp, ctx);
    *x = d as f32;
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_add_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    let res = a + b;
    *c = res;
    if !context.is_null() {
        let ctx = &*(context as *const mcaquad_context_t);
        let e = get_exp_f32(res) - ctx.binary32_precision;
        noise_f32(&mut *c, e, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_sub_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    let res = a - b;
    *c = res;
    if !context.is_null() {
        let ctx = &*(context as *const mcaquad_context_t);
        let e = get_exp_f32(res) - ctx.binary32_precision;
        noise_f32(&mut *c, e, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_mul_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    let res = a * b;
    *c = res;
    if !context.is_null() {
        let ctx = &*(context as *const mcaquad_context_t);
        let e = get_exp_f32(res) - ctx.binary32_precision;
        noise_f32(&mut *c, e, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_div_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    let res = a / b;
    *c = res;
    if !context.is_null() {
        let ctx = &*(context as *const mcaquad_context_t);
        let e = get_exp_f32(res) - ctx.binary32_precision;
        noise_f32(&mut *c, e, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_add_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    let res = a + b;
    *c = res;
    if !context.is_null() {
        let ctx = &*(context as *const mcaquad_context_t);
        let e = get_exp_f64(res) - ctx.binary64_precision;
        noise_f64(&mut *c, e, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_sub_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    let res = a - b;
    *c = res;
    if !context.is_null() {
        let ctx = &*(context as *const mcaquad_context_t);
        let e = get_exp_f64(res) - ctx.binary64_precision;
        noise_f64(&mut *c, e, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_mul_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    let res = a * b;
    *c = res;
    if !context.is_null() {
        let ctx = &*(context as *const mcaquad_context_t);
        let e = get_exp_f64(res) - ctx.binary64_precision;
        noise_f64(&mut *c, e, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_div_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    let res = a / b;
    *c = res;
    if !context.is_null() {
        let ctx = &*(context as *const mcaquad_context_t);
        let e = get_exp_f64(res) - ctx.binary64_precision;
        noise_f64(&mut *c, e, ctx);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_pre_init(
    panic: interflop_panic_t,
    stream: *mut FILE,
    context: *mut *mut c_void,
) {
    interflop_stdlib::logger_init(panic, stream, BACKEND_NAME.as_ptr() as *const c_char);
    if !context.is_null() {
        let ctx = Box::new(mcaquad_context_t {
            seed: 0,
            binary32_precision: 24,
            binary64_precision: 53,
            mode: mcaquad_mode::mcaquad_mode_mca,
            choose_seed: false,
            daz: false,
            ftz: false,
            sparsity: 1.0,
        });
        *context = Box::into_raw(ctx) as *mut c_void;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_cli(
    argc: c_int,
    argv: *mut *mut c_char,
    context: *mut c_void,
) {
    if context.is_null() || argv.is_null() {
        return;
    }
    let ctx = &mut *(context as *mut mcaquad_context_t);
    for i in 1..argc as usize {
        let ptr = *argv.add(i);
        if ptr.is_null() {
            continue;
        }
        if let Ok(arg) = CStr::from_ptr(ptr).to_str() {
            if arg.starts_with("--precision-binary32=") {
                if let Ok(val) = arg["--precision-binary32=".len()..].parse::<i32>() {
                    ctx.binary32_precision = val.clamp(1, 24);
                }
            } else if arg.starts_with("--precision-binary64=") {
                if let Ok(val) = arg["--precision-binary64=".len()..].parse::<i32>() {
                    ctx.binary64_precision = val.clamp(1, 53);
                }
            } else if arg.starts_with("--mode=") {
                match &arg["--mode=".len()..] {
                    "ieee" => ctx.mode = mcaquad_mode::mcaquad_mode_ieee,
                    "mca" => ctx.mode = mcaquad_mode::mcaquad_mode_mca,
                    "pb" => ctx.mode = mcaquad_mode::mcaquad_mode_pb,
                    "rr" => ctx.mode = mcaquad_mode::mcaquad_mode_rr,
                    _ => {}
                }
            } else if arg.starts_with("--seed=") {
                if let Ok(val) = arg["--seed=".len()..].parse::<u64>() {
                    ctx.seed = val;
                    ctx.choose_seed = true;
                }
            }
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_configure(configure: *mut c_void, context: *mut c_void) {
    if !configure.is_null() && !context.is_null() {
        let conf = &*(configure as *const mcaquad_context_t);
        let ctx = &mut *(context as *mut mcaquad_context_t);
        ctx.seed = conf.seed;
        ctx.binary32_precision = conf.binary32_precision;
        ctx.binary64_precision = conf.binary64_precision;
        ctx.mode = conf.mode;
        ctx.choose_seed = conf.choose_seed;
        ctx.daz = conf.daz;
        ctx.ftz = conf.ftz;
        ctx.sparsity = conf.sparsity;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_mca_init(
    _context: *mut c_void,
) -> interflop_backend_interface_t {
    interflop_backend_interface_t {
        interflop_add_float: Some(interflop_mca_add_float),
        interflop_sub_float: Some(interflop_mca_sub_float),
        interflop_mul_float: Some(interflop_mca_mul_float),
        interflop_div_float: Some(interflop_mca_div_float),
        interflop_cmp_float: None,
        interflop_add_double: Some(interflop_mca_add_double),
        interflop_sub_double: Some(interflop_mca_sub_double),
        interflop_mul_double: Some(interflop_mca_mul_double),
        interflop_div_double: Some(interflop_mca_div_double),
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
    interflop_mca_init(context)
}

#[no_mangle]
pub unsafe extern "C" fn interflop_pre_init(
    panic: interflop_panic_t,
    stream: *mut FILE,
    context: *mut *mut c_void,
) {
    interflop_mca_pre_init(panic, stream, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cli(argc: c_int, argv: *mut *mut c_char, context: *mut c_void) {
    interflop_mca_cli(argc, argv, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_configure(configure: *mut c_void, context: *mut c_void) {
    interflop_mca_configure(configure, context);
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_name() -> *const c_char {
    interflop_mca_get_backend_name()
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_version() -> *const c_char {
    interflop_mca_get_backend_version()
}
