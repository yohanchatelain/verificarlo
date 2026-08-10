#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]

use interflop_stdlib::{
    interflop_backend_interface_t, interflop_panic_t, interflop_set_handler, FCMP_PREDICATE,
};
use libc::{c_char, c_int, c_void, dlopen, dlsym, FILE, RTLD_GLOBAL, RTLD_NOW};
use std::ffi::{CStr, CString};
use std::ptr;

type interflop_pre_init_t = unsafe extern "C" fn(interflop_panic_t, *mut FILE, *mut *mut c_void);
type interflop_cli_t = unsafe extern "C" fn(c_int, *mut *mut c_char, *mut c_void);
type interflop_init_t = unsafe extern "C" fn(*mut c_void) -> interflop_backend_interface_t;

const MAX_BACKENDS: usize = 16;

static mut BACKENDS: [interflop_backend_interface_t; MAX_BACKENDS] =
    [interflop_backend_interface_t {
        interflop_add_float: None,
        interflop_sub_float: None,
        interflop_mul_float: None,
        interflop_div_float: None,
        interflop_cmp_float: None,
        interflop_add_double: None,
        interflop_sub_double: None,
        interflop_mul_double: None,
        interflop_div_double: None,
        interflop_cmp_double: None,
        interflop_cast_double_to_float: None,
        interflop_fma_float: None,
        interflop_fma_double: None,
        interflop_enter_function: None,
        interflop_exit_function: None,
        interflop_user_call: None,
        interflop_finalize: None,
    }; MAX_BACKENDS];

static mut CONTEXTS: [*mut c_void; MAX_BACKENDS] = [ptr::null_mut(); MAX_BACKENDS];
static mut LOADED_BACKENDS: usize = 0;
static mut INITIALIZED: bool = false;

pub unsafe extern "C" fn default_panic(msg: *const c_char) {
    if !msg.is_null() {
        let err = CStr::from_ptr(msg).to_string_lossy();
        eprintln!("[verificarlo panic]: {}", err);
    }
    std::process::exit(1);
}

#[no_mangle]
#[link_section = ".init_array"]
pub static VFC_INIT_CTOR: unsafe extern "C" fn() -> c_int = vfcwrapper_init;

#[no_mangle]
pub unsafe extern "C" fn vfcwrapper_init() -> c_int {
    if INITIALIZED {
        return 0;
    }
    INITIALIZED = true;

    // Register stdlib function handlers for backends
    interflop_set_handler(
        b"getenv\0".as_ptr() as *const c_char,
        libc::getenv as *mut c_void,
    );
    interflop_set_handler(
        b"malloc\0".as_ptr() as *const c_char,
        libc::malloc as *mut c_void,
    );
    interflop_set_handler(
        b"free\0".as_ptr() as *const c_char,
        libc::free as *mut c_void,
    );
    interflop_set_handler(
        b"exit\0".as_ptr() as *const c_char,
        libc::exit as *mut c_void,
    );

    let env_val = std::env::var("VFC_BACKENDS").unwrap_or_else(|_| "interflop-ieee".to_string());
    let backend_specs: Vec<&str> = env_val.split(';').collect();

    for spec in backend_specs {
        if LOADED_BACKENDS >= MAX_BACKENDS {
            break;
        }
        let parts: Vec<&str> = spec.trim().split_whitespace().collect();
        if parts.is_empty() {
            continue;
        }

        let name = parts[0];
        let lib_filename = if name.starts_with("lib") && name.ends_with(".so") {
            name.to_string()
        } else if name.starts_with("lib") {
            format!("{}.so", name)
        } else if name.ends_with(".so") {
            name.to_string()
        } else {
            format!("lib{}.so", name)
        };

        let c_lib_path = CString::new(lib_filename.clone()).unwrap();
        let handle = dlopen(c_lib_path.as_ptr(), RTLD_NOW | RTLD_GLOBAL);
        if handle.is_null() {
            continue;
        }

        let pre_init_fn: Option<interflop_pre_init_t> = std::mem::transmute(dlsym(
            handle,
            b"interflop_pre_init\0".as_ptr() as *const c_char,
        ));
        let cli_fn: Option<interflop_cli_t> =
            std::mem::transmute(dlsym(handle, b"interflop_cli\0".as_ptr() as *const c_char));
        let init_fn: Option<interflop_init_t> =
            std::mem::transmute(dlsym(handle, b"interflop_init\0".as_ptr() as *const c_char));

        if let (Some(pre_init), Some(init)) = (pre_init_fn, init_fn) {
            let mut ctx: *mut c_void = ptr::null_mut();
            let stderr_stream = libc::fdopen(2, b"w\0".as_ptr() as *const c_char);
            pre_init(
                Some(default_panic),
                stderr_stream,
                &mut ctx as *mut *mut c_void,
            );

            if let Some(cli) = cli_fn {
                let mut c_args: Vec<CString> = vec![CString::new(name).unwrap()];
                for arg in &parts[1..] {
                    c_args.push(CString::new(*arg).unwrap());
                }
                let mut argv_ptrs: Vec<*mut c_char> = c_args
                    .iter()
                    .map(|arg| arg.as_ptr() as *mut c_char)
                    .collect();
                cli(argv_ptrs.len() as c_int, argv_ptrs.as_mut_ptr(), ctx);
            }

            let backend_iface = init(ctx);
            BACKENDS[LOADED_BACKENDS] = backend_iface;
            CONTEXTS[LOADED_BACKENDS] = ctx;
            LOADED_BACKENDS += 1;
        }
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_add_float(a: f32, b: f32) -> f32 {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return a + b;
    }
    let mut res: f32 = 0.0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_add_float {
            f(a, b, &mut res, CONTEXTS[i]);
        } else {
            res = a + b;
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_sub_float(a: f32, b: f32) -> f32 {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return a - b;
    }
    let mut res: f32 = 0.0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_sub_float {
            f(a, b, &mut res, CONTEXTS[i]);
        } else {
            res = a - b;
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_mul_float(a: f32, b: f32) -> f32 {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return a * b;
    }
    let mut res: f32 = 0.0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_mul_float {
            f(a, b, &mut res, CONTEXTS[i]);
        } else {
            res = a * b;
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_div_float(a: f32, b: f32) -> f32 {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return a / b;
    }
    let mut res: f32 = 0.0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_div_float {
            f(a, b, &mut res, CONTEXTS[i]);
        } else {
            res = a / b;
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_cmp_float(p: FCMP_PREDICATE, a: f32, b: f32) -> c_int {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return if a == b { 1 } else { 0 };
    }
    let mut res: c_int = 0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_cmp_float {
            f(p, a, b, &mut res, CONTEXTS[i]);
        } else {
            res = if a == b { 1 } else { 0 };
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_add_double(a: f64, b: f64) -> f64 {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return a + b;
    }
    let mut res: f64 = 0.0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_add_double {
            f(a, b, &mut res, CONTEXTS[i]);
        } else {
            res = a + b;
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_sub_double(a: f64, b: f64) -> f64 {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return a - b;
    }
    let mut res: f64 = 0.0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_sub_double {
            f(a, b, &mut res, CONTEXTS[i]);
        } else {
            res = a - b;
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_mul_double(a: f64, b: f64) -> f64 {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return a * b;
    }
    let mut res: f64 = 0.0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_mul_double {
            f(a, b, &mut res, CONTEXTS[i]);
        } else {
            res = a * b;
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_div_double(a: f64, b: f64) -> f64 {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return a / b;
    }
    let mut res: f64 = 0.0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_div_double {
            f(a, b, &mut res, CONTEXTS[i]);
        } else {
            res = a / b;
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_cmp_double(p: FCMP_PREDICATE, a: f64, b: f64) -> c_int {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return if a == b { 1 } else { 0 };
    }
    let mut res: c_int = 0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_cmp_double {
            f(p, a, b, &mut res, CONTEXTS[i]);
        } else {
            res = if a == b { 1 } else { 0 };
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_cast_double_to_float(a: f64) -> f32 {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return a as f32;
    }
    let mut res: f32 = 0.0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_cast_double_to_float {
            f(a, &mut res, CONTEXTS[i]);
        } else {
            res = a as f32;
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_fma_float(a: f32, b: f32, c: f32) -> f32 {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return a.mul_add(b, c);
    }
    let mut res: f32 = 0.0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_fma_float {
            f(a, b, c, &mut res, CONTEXTS[i]);
        } else {
            res = a.mul_add(b, c);
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn _vfc_fma_double(a: f64, b: f64, c: f64) -> f64 {
    vfcwrapper_init();
    if LOADED_BACKENDS == 0 {
        return a.mul_add(b, c);
    }
    let mut res: f64 = 0.0;
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_fma_double {
            f(a, b, c, &mut res, CONTEXTS[i]);
        } else {
            res = a.mul_add(b, c);
        }
    }
    res
}

#[no_mangle]
pub unsafe extern "C" fn vfcwrapper_finalize() -> c_int {
    if !INITIALIZED {
        return 0;
    }
    for i in 0..LOADED_BACKENDS {
        if let Some(f) = BACKENDS[i].interflop_finalize {
            f(CONTEXTS[i]);
        }
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn _floatadd(a: f32, b: f32) -> f32 {
    _vfc_add_float(a, b)
}
#[no_mangle]
pub unsafe extern "C" fn _floatsub(a: f32, b: f32) -> f32 {
    _vfc_sub_float(a, b)
}
#[no_mangle]
pub unsafe extern "C" fn _floatmul(a: f32, b: f32) -> f32 {
    _vfc_mul_float(a, b)
}
#[no_mangle]
pub unsafe extern "C" fn _floatdiv(a: f32, b: f32) -> f32 {
    _vfc_div_float(a, b)
}
#[no_mangle]
pub unsafe extern "C" fn _doubleadd(a: f64, b: f64) -> f64 {
    _vfc_add_double(a, b)
}
#[no_mangle]
pub unsafe extern "C" fn _doublesub(a: f64, b: f64) -> f64 {
    _vfc_sub_double(a, b)
}
#[no_mangle]
pub unsafe extern "C" fn _doublemul(a: f64, b: f64) -> f64 {
    _vfc_mul_double(a, b)
}
#[no_mangle]
pub unsafe extern "C" fn _doublediv(a: f64, b: f64) -> f64 {
    _vfc_div_double(a, b)
}
#[no_mangle]
pub unsafe extern "C" fn _floatcmp(p: FCMP_PREDICATE, a: f32, b: f32) -> c_int {
    _vfc_cmp_float(p, a, b)
}
#[no_mangle]
pub unsafe extern "C" fn _doublecmp(p: FCMP_PREDICATE, a: f64, b: f64) -> c_int {
    _vfc_cmp_double(p, a, b)
}
#[no_mangle]
pub unsafe extern "C" fn _doubletofloat(a: f64) -> f32 {
    _vfc_cast_double_to_float(a)
}
#[no_mangle]
pub unsafe extern "C" fn _floatfma(a: f32, b: f32, c: f32) -> f32 {
    _vfc_fma_float(a, b, c)
}
#[no_mangle]
pub unsafe extern "C" fn _doublefma(a: f64, b: f64, c: f64) -> f64 {
    _vfc_fma_double(a, b, c)
}
