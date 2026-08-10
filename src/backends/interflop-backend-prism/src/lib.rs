#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]

use libc::{c_char, c_int, c_void, FILE};
use interflop_stdlib::{interflop_backend_interface_t, interflop_panic_t};

// External C functions from interflop_prism.c
extern "C" {
    pub fn interflop_prism_init(context: *mut c_void) -> interflop_backend_interface_t;
    pub fn interflop_prism_pre_init(panic: interflop_panic_t, stream: *mut FILE, context: *mut *mut c_void);
    pub fn interflop_prism_cli(argc: c_int, argv: *mut *mut c_char, context: *mut c_void);
    pub fn interflop_prism_configure(configure: *mut c_void, context: *mut c_void);
    pub fn interflop_prism_get_backend_name() -> *const c_char;
    pub fn interflop_prism_get_backend_version() -> *const c_char;
}

#[no_mangle]
pub unsafe extern "C" fn interflop_init(context: *mut c_void) -> interflop_backend_interface_t {
    interflop_prism_init(context)
}

#[no_mangle]
pub unsafe extern "C" fn interflop_pre_init(panic: interflop_panic_t, stream: *mut FILE, context: *mut *mut c_void) {
    interflop_prism_pre_init(panic, stream, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_cli(argc: c_int, argv: *mut *mut c_char, context: *mut c_void) {
    interflop_prism_cli(argc, argv, context);
}

#[no_mangle]
pub unsafe extern "C" fn interflop_configure(configure: *mut c_void, context: *mut c_void) {
    interflop_prism_configure(configure, context);
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_name() -> *const c_char {
    unsafe { interflop_prism_get_backend_name() }
}

#[no_mangle]
pub extern "C" fn interflop_get_backend_version() -> *const c_char {
    unsafe { interflop_prism_get_backend_version() }
}
