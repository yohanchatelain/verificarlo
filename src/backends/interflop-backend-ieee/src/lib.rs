#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]

use interflop_stdlib::{interflop_backend_interface_t, interflop_panic_t, FCMP_PREDICATE};
use libc::{c_char, c_int, c_void, FILE};
use std::ffi::{CStr, CString};

#[repr(C)]
pub struct ieee_context_t {
    pub mul_count: u64,
    pub div_count: u64,
    pub add_count: u64,
    pub sub_count: u64,
    pub fma_count: u64,
    pub debug: c_int,
    pub debug_binary: c_int,
    pub no_backend_name: c_int,
    pub print_new_line: c_int,
    pub print_subnormal_normalized: c_int,
    pub count_op: c_int,
}

static BACKEND_NAME: &[u8] = b"interflop-ieee\0";
static BACKEND_VERSION: &[u8] = b"1.x-dev\0";

#[no_mangle]
pub extern "C" fn interflop_ieee_get_backend_name() -> *const c_char {
    BACKEND_NAME.as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn interflop_ieee_get_backend_version() -> *const c_char {
    BACKEND_VERSION.as_ptr() as *const c_char
}

unsafe fn debug_print_arithmetic(op: &str, a: f64, b: f64, c: f64, context: *mut c_void) {
    if context.is_null() {
        return;
    }
    let ctx = &*(context as *const ieee_context_t);
    if ctx.debug != 0 || ctx.debug_binary != 0 {
        let header = if ctx.debug != 0 {
            "Decimal "
        } else {
            "Binary "
        };
        if ctx.no_backend_name == 0 {
            if ctx.print_new_line != 0 {
                let msg = CString::new(format!("{}\n", header)).unwrap();
                interflop_stdlib::vlogger_info(msg.as_ptr(), std::ptr::null_mut());
            } else {
                let msg = CString::new(header).unwrap();
                interflop_stdlib::vlogger_info(msg.as_ptr(), std::ptr::null_mut());
            }
        }
        let body = if ctx.debug != 0 {
            format!("{} {} {} -> {}\n", a, op, b, c)
        } else {
            format!("{:e} {} {:e} -> {:e}\n", a, op, b, c)
        };
        if let Ok(c_msg) = CString::new(body) {
            interflop_stdlib::vlogger_info(c_msg.as_ptr(), std::ptr::null_mut());
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_add_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    let res = a + b;
    if !c.is_null() {
        *c = res;
    }
    if !context.is_null() {
        let ctx = &mut *(context as *mut ieee_context_t);
        if ctx.count_op != 0 {
            ctx.add_count += 1;
        }
        debug_print_arithmetic("+", a as f64, b as f64, res as f64, context);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_sub_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    let res = a - b;
    if !c.is_null() {
        *c = res;
    }
    if !context.is_null() {
        let ctx = &mut *(context as *mut ieee_context_t);
        if ctx.count_op != 0 {
            ctx.sub_count += 1;
        }
        debug_print_arithmetic("-", a as f64, b as f64, res as f64, context);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_mul_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    let res = a * b;
    if !c.is_null() {
        *c = res;
    }
    if !context.is_null() {
        let ctx = &mut *(context as *mut ieee_context_t);
        if ctx.count_op != 0 {
            ctx.mul_count += 1;
        }
        debug_print_arithmetic("*", a as f64, b as f64, res as f64, context);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_div_float(
    a: f32,
    b: f32,
    c: *mut f32,
    context: *mut c_void,
) {
    let res = a / b;
    if !c.is_null() {
        *c = res;
    }
    if !context.is_null() {
        let ctx = &mut *(context as *mut ieee_context_t);
        if ctx.count_op != 0 {
            ctx.div_count += 1;
        }
        debug_print_arithmetic("/", a as f64, b as f64, res as f64, context);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_cmp_float(
    p: FCMP_PREDICATE,
    a: f32,
    b: f32,
    c: *mut c_int,
    _context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    let res = match p {
        FCMP_PREDICATE::FCMP_FALSE => false,
        FCMP_PREDICATE::FCMP_OEQ => a == b,
        FCMP_PREDICATE::FCMP_OGT => a > b,
        FCMP_PREDICATE::FCMP_OGE => a >= b,
        FCMP_PREDICATE::FCMP_OLT => a < b,
        FCMP_PREDICATE::FCMP_OLE => a <= b,
        FCMP_PREDICATE::FCMP_ONE => a != b && !a.is_nan() && !b.is_nan(),
        FCMP_PREDICATE::FCMP_ORD => !a.is_nan() && !b.is_nan(),
        FCMP_PREDICATE::FCMP_UNO => a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_UEQ => a == b || a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_UGT => a > b || a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_UGE => a >= b || a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_ULT => a < b || a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_ULE => a <= b || a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_UNE => a != b,
        FCMP_PREDICATE::FCMP_TRUE => true,
    };
    *c = if res { 1 } else { 0 };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_add_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    let res = a + b;
    if !c.is_null() {
        *c = res;
    }
    if !context.is_null() {
        let ctx = &mut *(context as *mut ieee_context_t);
        if ctx.count_op != 0 {
            ctx.add_count += 1;
        }
        debug_print_arithmetic("+", a, b, res, context);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_sub_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    let res = a - b;
    if !c.is_null() {
        *c = res;
    }
    if !context.is_null() {
        let ctx = &mut *(context as *mut ieee_context_t);
        if ctx.count_op != 0 {
            ctx.sub_count += 1;
        }
        debug_print_arithmetic("-", a, b, res, context);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_mul_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    let res = a * b;
    if !c.is_null() {
        *c = res;
    }
    if !context.is_null() {
        let ctx = &mut *(context as *mut ieee_context_t);
        if ctx.count_op != 0 {
            ctx.mul_count += 1;
        }
        debug_print_arithmetic("*", a, b, res, context);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_div_double(
    a: f64,
    b: f64,
    c: *mut f64,
    context: *mut c_void,
) {
    let res = a / b;
    if !c.is_null() {
        *c = res;
    }
    if !context.is_null() {
        let ctx = &mut *(context as *mut ieee_context_t);
        if ctx.count_op != 0 {
            ctx.div_count += 1;
        }
        debug_print_arithmetic("/", a, b, res, context);
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_cmp_double(
    p: FCMP_PREDICATE,
    a: f64,
    b: f64,
    c: *mut c_int,
    _context: *mut c_void,
) {
    if c.is_null() {
        return;
    }
    let res = match p {
        FCMP_PREDICATE::FCMP_FALSE => false,
        FCMP_PREDICATE::FCMP_OEQ => a == b,
        FCMP_PREDICATE::FCMP_OGT => a > b,
        FCMP_PREDICATE::FCMP_OGE => a >= b,
        FCMP_PREDICATE::FCMP_OLT => a < b,
        FCMP_PREDICATE::FCMP_OLE => a <= b,
        FCMP_PREDICATE::FCMP_ONE => a != b && !a.is_nan() && !b.is_nan(),
        FCMP_PREDICATE::FCMP_ORD => !a.is_nan() && !b.is_nan(),
        FCMP_PREDICATE::FCMP_UNO => a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_UEQ => a == b || a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_UGT => a > b || a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_UGE => a >= b || a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_ULT => a < b || a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_ULE => a <= b || a.is_nan() || b.is_nan(),
        FCMP_PREDICATE::FCMP_UNE => a != b,
        FCMP_PREDICATE::FCMP_TRUE => true,
    };
    *c = if res { 1 } else { 0 };
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_cast_double_to_float(
    a: f64,
    c: *mut f32,
    _context: *mut c_void,
) {
    if !c.is_null() {
        *c = a as f32;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_fma_float(
    a: f32,
    b: f32,
    c: f32,
    res: *mut f32,
    context: *mut c_void,
) {
    let r = a.mul_add(b, c);
    if !res.is_null() {
        *res = r;
    }
    if !context.is_null() {
        let ctx = &mut *(context as *mut ieee_context_t);
        if ctx.count_op != 0 {
            ctx.fma_count += 1;
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_fma_double(
    a: f64,
    b: f64,
    c: f64,
    res: *mut f64,
    context: *mut c_void,
) {
    let r = a.mul_add(b, c);
    if !res.is_null() {
        *res = r;
    }
    if !context.is_null() {
        let ctx = &mut *(context as *mut ieee_context_t);
        if ctx.count_op != 0 {
            ctx.fma_count += 1;
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_finalize(context: *mut c_void) {
    if !context.is_null() {
        let ctx = &*(context as *const ieee_context_t);
        if ctx.count_op != 0 {
            let msg = format!(
                "operations count:\n\tmul={}\n\tdiv={}\n\tadd={}\n\tsub={}\n\tfma={}\n",
                ctx.mul_count, ctx.div_count, ctx.add_count, ctx.sub_count, ctx.fma_count
            );
            if let Ok(c_msg) = CString::new(msg) {
                interflop_stdlib::vlogger_info(c_msg.as_ptr(), std::ptr::null_mut());
            }
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_pre_init(
    panic: interflop_panic_t,
    stream: *mut FILE,
    context: *mut *mut c_void,
) {
    interflop_stdlib::logger_init(panic, stream, BACKEND_NAME.as_ptr() as *const c_char);
    if !context.is_null() {
        let ctx = Box::new(ieee_context_t {
            mul_count: 0,
            div_count: 0,
            add_count: 0,
            sub_count: 0,
            fma_count: 0,
            debug: 0,
            debug_binary: 0,
            no_backend_name: 0,
            print_new_line: 0,
            print_subnormal_normalized: 0,
            count_op: 0,
        });
        *context = Box::into_raw(ctx) as *mut c_void;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_cli(
    argc: c_int,
    argv: *mut *mut c_char,
    context: *mut c_void,
) {
    if context.is_null() || argv.is_null() {
        return;
    }
    let ctx = &mut *(context as *mut ieee_context_t);
    for i in 1..argc as usize {
        let ptr = *argv.add(i);
        if ptr.is_null() {
            continue;
        }
        if let Ok(arg) = CStr::from_ptr(ptr).to_str() {
            match arg {
                "--debug" | "-d" => ctx.debug = 1,
                "--debug-binary" | "-b" => ctx.debug_binary = 1,
                "--no-backend-name" | "-s" => ctx.no_backend_name = 1,
                "--print-new-line" | "-n" => ctx.print_new_line = 1,
                "--print-subnormal-normalized" => ctx.print_subnormal_normalized = 1,
                "--count-op" | "-o" => ctx.count_op = 1,
                _ => {}
            }
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_configure(configure: *mut c_void, context: *mut c_void) {
    if !configure.is_null() && !context.is_null() {
        let conf = &*(configure as *const ieee_context_t);
        let ctx = &mut *(context as *mut ieee_context_t);
        ctx.debug = conf.debug;
        ctx.debug_binary = conf.debug_binary;
        ctx.no_backend_name = conf.no_backend_name;
        ctx.print_new_line = conf.print_new_line;
        ctx.print_subnormal_normalized = conf.print_subnormal_normalized;
        ctx.count_op = conf.count_op;
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_ieee_init(
    _context: *mut c_void,
) -> interflop_backend_interface_t {
    interflop_backend_interface_t {
        interflop_add_float: Some(interflop_ieee_add_float),
        interflop_sub_float: Some(interflop_ieee_sub_float),
        interflop_mul_float: Some(interflop_ieee_mul_float),
        interflop_div_float: Some(interflop_ieee_div_float),
        interflop_cmp_float: Some(interflop_ieee_cmp_float),
        interflop_add_double: Some(interflop_ieee_add_double),
        interflop_sub_double: Some(interflop_ieee_sub_double),
        interflop_mul_double: Some(interflop_ieee_mul_double),
        interflop_div_double: Some(interflop_ieee_div_double),
        interflop_cmp_double: Some(interflop_ieee_cmp_double),
        interflop_cast_double_to_float: Some(interflop_ieee_cast_double_to_float),
        interflop_fma_float: Some(interflop_ieee_fma_float),
        interflop_fma_double: Some(interflop_ieee_fma_double),
        interflop_enter_function: None,
        interflop_exit_function: None,
        interflop_user_call: None,
        interflop_finalize: Some(interflop_ieee_finalize),
    }
}

#[no_mangle]
pub unsafe extern "C" fn interflop_init(context: *mut c_void) -> interflop_backend_interface_t {
    interflop_ieee_init(context)
}

#[no_mangle]
pub unsafe extern "C" fn interflop_pre_init(
    panic: interflop_panic_t,
    stream: *mut FILE,
    context: *mut *mut c_void,
) {
    interflop_ieee_pre_init(panic, stream, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cli(argc: c_int, argv: *mut *mut c_char, context: *mut c_void) {
    interflop_ieee_cli(argc, argv, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_configure(configure: *mut c_void, context: *mut c_void) {
    interflop_ieee_configure(configure, context);
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_name() -> *const c_char {
    interflop_ieee_get_backend_name()
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_version() -> *const c_char {
    interflop_ieee_get_backend_version()
}
