#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]

use libc::{c_char, c_int, c_void, FILE};

extern "C" {
    fn vfprintf(stream: *mut FILE, format: *const c_char, arg: *mut c_void) -> c_int;
    fn fprintf(stream: *mut FILE, format: *const c_char, ...) -> c_int;

    pub fn logger_init(
        panic: Option<unsafe extern "C" fn(*const c_char)>,
        stream: *mut FILE,
        name: *const c_char,
    );
    pub fn logger_info(fmt: *const c_char, ...);
    pub fn logger_warning(fmt: *const c_char, ...);
    pub fn logger_error(fmt: *const c_char, ...);
    pub fn logger_debug(fmt: *const c_char, ...);
}

#[no_mangle]
pub unsafe extern "C" fn __interflop_stdlib_force_link_shims() {
    let _ = logger_init as *const ();
    let _ = logger_info as *const ();
    let _ = logger_warning as *const ();
    let _ = logger_error as *const ();
    let _ = logger_debug as *const ();
}

#[no_mangle]
pub unsafe extern "C" fn vlogger_info(fmt: *const c_char, argp: *mut c_void) {
    vfprintf(libc::fdopen(2, b"w\0".as_ptr() as *const c_char), fmt, argp);
}

#[no_mangle]
pub unsafe extern "C" fn vlogger_warning(fmt: *const c_char, argp: *mut c_void) {
    vfprintf(libc::fdopen(2, b"w\0".as_ptr() as *const c_char), fmt, argp);
}

#[no_mangle]
pub unsafe extern "C" fn vlogger_error(fmt: *const c_char, argp: *mut c_void) {
    vfprintf(libc::fdopen(2, b"w\0".as_ptr() as *const c_char), fmt, argp);
    libc::exit(1);
}

#[no_mangle]
pub unsafe extern "C" fn vlogger_debug(fmt: *const c_char, argp: *mut c_void) {
    vfprintf(libc::fdopen(2, b"w\0".as_ptr() as *const c_char), fmt, argp);
}
