#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]

pub mod fma;
pub mod hashmap;
pub mod interflop_ffi;
pub mod logger;
pub mod math;
pub mod rng;

pub use fma::*;
pub use hashmap::*;
pub use interflop_ffi::*;
pub use logger::*;
pub use math::*;
pub use rng::*;

extern "C" {
    fn interflop_stdlib_logger_anchor();
}

#[no_mangle]
pub unsafe extern "C" fn __interflop_stdlib_keep_shims() {
    interflop_stdlib_logger_anchor();
}
