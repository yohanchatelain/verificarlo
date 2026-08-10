#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]

use libc::{c_char, c_int, c_long, c_uint, c_void, size_t, FILE};
use std::ffi::CStr;

pub type IBool = c_int;
pub const ITrue: IBool = 1;
pub const IFalse: IBool = 0;

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum FCMP_PREDICATE {
    FCMP_FALSE = 0,
    FCMP_OEQ,
    FCMP_OGT,
    FCMP_OGE,
    FCMP_OLT,
    FCMP_OLE,
    FCMP_ONE,
    FCMP_ORD,
    FCMP_UNO,
    FCMP_UEQ,
    FCMP_UGT,
    FCMP_UGE,
    FCMP_ULT,
    FCMP_ULE,
    FCMP_UNE,
    FCMP_TRUE,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum FTYPES {
    FFLOAT = 0,
    FDOUBLE,
    FQUAD,
    FFLOAT_PTR,
    FDOUBLE_PTR,
    FQUAD_PTR,
    FTYPES_END,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum interflop_call_id {
    INTERFLOP_SET_ROUNDING_MODE = 6,
    INTERFLOP_SET_RANGE_BINARY64 = 5,
    INTERFLOP_SET_RANGE_BINARY32 = 4,
    INTERFLOP_SET_PRECISION_BINARY64 = 3,
    INTERFLOP_SET_PRECISION_BINARY32 = 2,
    INTERFLOP_INEXACT_ID = 1,
    INTERFLOP_CUSTOM_ID = -1,
}

#[repr(C)]
pub struct interflop_function_info_t {
    pub id: *mut c_char,
    pub isLibraryFunction: i16,
    pub isIntrinsicFunction: i16,
    pub useFloat: i16,
    pub useDouble: i16,
}

#[repr(C)]
pub struct interflop_function_stack_t {
    pub array: *mut *mut interflop_function_info_t,
    pub top: c_long,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct interflop_backend_interface_t {
    pub interflop_add_float: Option<unsafe extern "C" fn(f32, f32, *mut f32, *mut c_void)>,
    pub interflop_sub_float: Option<unsafe extern "C" fn(f32, f32, *mut f32, *mut c_void)>,
    pub interflop_mul_float: Option<unsafe extern "C" fn(f32, f32, *mut f32, *mut c_void)>,
    pub interflop_div_float: Option<unsafe extern "C" fn(f32, f32, *mut f32, *mut c_void)>,
    pub interflop_cmp_float:
        Option<unsafe extern "C" fn(FCMP_PREDICATE, f32, f32, *mut c_int, *mut c_void)>,

    pub interflop_add_double: Option<unsafe extern "C" fn(f64, f64, *mut f64, *mut c_void)>,
    pub interflop_sub_double: Option<unsafe extern "C" fn(f64, f64, *mut f64, *mut c_void)>,
    pub interflop_mul_double: Option<unsafe extern "C" fn(f64, f64, *mut f64, *mut c_void)>,
    pub interflop_div_double: Option<unsafe extern "C" fn(f64, f64, *mut f64, *mut c_void)>,
    pub interflop_cmp_double:
        Option<unsafe extern "C" fn(FCMP_PREDICATE, f64, f64, *mut c_int, *mut c_void)>,

    pub interflop_cast_double_to_float: Option<unsafe extern "C" fn(f64, *mut f32, *mut c_void)>,
    pub interflop_fma_float: Option<unsafe extern "C" fn(f32, f32, f32, *mut f32, *mut c_void)>,
    pub interflop_fma_double: Option<unsafe extern "C" fn(f64, f64, f64, *mut f64, *mut c_void)>,

    pub interflop_enter_function:
        Option<unsafe extern "C" fn(*mut interflop_function_stack_t, *mut c_void, c_int, ...)>,
    pub interflop_exit_function:
        Option<unsafe extern "C" fn(*mut interflop_function_stack_t, *mut c_void, c_int, ...)>,
    pub interflop_user_call: Option<unsafe extern "C" fn(*mut c_void, interflop_call_id, ...)>,
    pub interflop_finalize: Option<unsafe extern "C" fn(*mut c_void)>,
}

pub type interflop_malloc_t = Option<unsafe extern "C" fn(size_t) -> *mut c_void>;
pub type interflop_fopen_t =
    Option<unsafe extern "C" fn(*const c_char, *const c_char, *mut c_int) -> *mut FILE>;
pub type interflop_panic_t = Option<unsafe extern "C" fn(*const c_char)>;
pub type interflop_strcmp_t = Option<unsafe extern "C" fn(*const c_char, *const c_char) -> c_int>;
pub type interflop_strcasecmp_t =
    Option<unsafe extern "C" fn(*const c_char, *const c_char) -> c_int>;
pub type interflop_strtol_t =
    Option<unsafe extern "C" fn(*const c_char, *mut *mut c_char, *mut c_int) -> c_long>;
pub type interflop_strtod_t =
    Option<unsafe extern "C" fn(*const c_char, *mut *mut c_char, *mut c_int) -> f64>;
pub type interflop_getenv_t = Option<unsafe extern "C" fn(*const c_char) -> *mut c_char>;
pub type interflop_fprintf_t = Option<unsafe extern "C" fn(*mut FILE, *const c_char, ...) -> c_int>;
pub type interflop_strcpy_t = Option<unsafe extern "C" fn(*mut c_char, *const c_char) -> c_char>;
pub type interflop_strncpy_t =
    Option<unsafe extern "C" fn(*mut c_char, *const c_char, size_t) -> c_char>;
pub type interflop_fclose_t = Option<unsafe extern "C" fn(*mut FILE) -> c_int>;
pub type interflop_gettid_t = Option<unsafe extern "C" fn() -> c_int>;
pub type interflop_strerror_t = Option<unsafe extern "C" fn(c_int) -> *mut c_char>;
pub type interflop_sprintf_t =
    Option<unsafe extern "C" fn(*mut c_char, *const c_char, ...) -> c_int>;
pub type interflop_vwarnx_t = Option<unsafe extern "C" fn(*const c_char, *mut c_void)>;
pub type interflop_vfprintf_t =
    Option<unsafe extern "C" fn(*mut FILE, *const c_char, *mut c_void) -> c_int>;
pub type interflop_exit_t = Option<unsafe extern "C" fn(c_int)>;
pub type interflop_strtok_r_t =
    Option<unsafe extern "C" fn(*mut c_char, *const c_char, *mut *mut c_char) -> *mut c_char>;
pub type interflop_fgets_t =
    Option<unsafe extern "C" fn(*mut c_char, c_int, *mut FILE) -> *mut c_char>;
pub type interflop_free_t = Option<unsafe extern "C" fn(*mut c_void)>;
pub type interflop_calloc_t = Option<unsafe extern "C" fn(size_t, size_t) -> *mut c_void>;
pub type interflop_argp_parse_t = Option<
    unsafe extern "C" fn(
        *mut c_void,
        c_int,
        *mut *mut c_char,
        c_uint,
        *mut c_int,
        *mut c_void,
    ) -> c_int,
>;
pub type interflop_nanHandler_t = Option<unsafe extern "C" fn()>;
pub type interflop_infHandler_t = Option<unsafe extern "C" fn()>;
pub type interflop_maxHandler_t = Option<unsafe extern "C" fn()>;
pub type interflop_cancellationHandler_t = Option<unsafe extern "C" fn(c_int)>;
pub type interflop_denormalHandler_t = Option<unsafe extern "C" fn()>;
pub type interflop_debug_print_op_t =
    Option<unsafe extern "C" fn(c_int, *const c_char, *const f64, *const f64)>;
pub type interflop_gettimeofday_t = Option<unsafe extern "C" fn(*mut c_void, *mut c_void) -> c_int>;
pub type interflop_register_printf_specifier_t =
    Option<unsafe extern "C" fn(c_int, *mut c_void, *mut c_void) -> c_int>;

macro_rules! declare_handler {
    ($name:ident, $type:ty) => {
        #[no_mangle]
        pub static mut $name: $type = None;
    };
}

declare_handler!(interflop_malloc, interflop_malloc_t);
declare_handler!(interflop_fopen, interflop_fopen_t);
declare_handler!(interflop_panic, interflop_panic_t);
declare_handler!(interflop_strcmp, interflop_strcmp_t);
declare_handler!(interflop_strcasecmp, interflop_strcasecmp_t);
declare_handler!(interflop_strtol, interflop_strtol_t);
declare_handler!(interflop_strtod, interflop_strtod_t);
declare_handler!(interflop_getenv, interflop_getenv_t);
declare_handler!(interflop_fprintf, interflop_fprintf_t);
declare_handler!(interflop_strcpy, interflop_strcpy_t);
declare_handler!(interflop_strncpy, interflop_strncpy_t);
declare_handler!(interflop_fclose, interflop_fclose_t);
declare_handler!(interflop_gettid, interflop_gettid_t);
declare_handler!(interflop_strerror, interflop_strerror_t);
declare_handler!(interflop_sprintf, interflop_sprintf_t);
declare_handler!(interflop_vwarnx, interflop_vwarnx_t);
declare_handler!(interflop_vfprintf, interflop_vfprintf_t);
declare_handler!(interflop_exit, interflop_exit_t);
declare_handler!(interflop_strtok_r, interflop_strtok_r_t);
declare_handler!(interflop_fgets, interflop_fgets_t);
declare_handler!(interflop_free, interflop_free_t);
declare_handler!(interflop_calloc, interflop_calloc_t);
declare_handler!(interflop_argp_parse, interflop_argp_parse_t);
declare_handler!(interflop_nanHandler, interflop_nanHandler_t);
declare_handler!(interflop_infHandler, interflop_infHandler_t);
declare_handler!(interflop_maxHandler, interflop_maxHandler_t);
declare_handler!(
    interflop_cancellationHandler,
    interflop_cancellationHandler_t
);
declare_handler!(interflop_denormalHandler, interflop_denormalHandler_t);
declare_handler!(interflop_debug_print_op, interflop_debug_print_op_t);
declare_handler!(interflop_gettimeofday, interflop_gettimeofday_t);
declare_handler!(
    interflop_register_printf_specifier,
    interflop_register_printf_specifier_t
);

#[no_mangle]
pub unsafe extern "C" fn interflop_set_handler(name: *const c_char, function_ptr: *mut c_void) {
    if name.is_null() {
        return;
    }
    let c_str = match CStr::from_ptr(name).to_str() {
        Ok(s) => s,
        Err(_) => return,
    };

    macro_rules! set_h {
        ($fname:expr, $handler:ident, $type:ty) => {
            if c_str == $fname {
                $handler = std::mem::transmute::<*mut c_void, $type>(function_ptr);
                return;
            }
        };
    }

    set_h!("malloc", interflop_malloc, interflop_malloc_t);
    set_h!("fopen", interflop_fopen, interflop_fopen_t);
    set_h!("panic", interflop_panic, interflop_panic_t);
    set_h!("strcmp", interflop_strcmp, interflop_strcmp_t);
    set_h!("strcasecmp", interflop_strcasecmp, interflop_strcasecmp_t);
    set_h!("strtol", interflop_strtol, interflop_strtol_t);
    set_h!("strtod", interflop_strtod, interflop_strtod_t);
    set_h!("getenv", interflop_getenv, interflop_getenv_t);
    set_h!("fprintf", interflop_fprintf, interflop_fprintf_t);
    set_h!("strcpy", interflop_strcpy, interflop_strcpy_t);
    set_h!("strncpy", interflop_strncpy, interflop_strncpy_t);
    set_h!("fclose", interflop_fclose, interflop_fclose_t);
    set_h!("gettid", interflop_gettid, interflop_gettid_t);
    set_h!("strerror", interflop_strerror, interflop_strerror_t);
    set_h!("sprintf", interflop_sprintf, interflop_sprintf_t);
    set_h!("vwarnx", interflop_vwarnx, interflop_vwarnx_t);
    set_h!("vfprintf", interflop_vfprintf, interflop_vfprintf_t);
    set_h!("exit", interflop_exit, interflop_exit_t);
    set_h!("strtok_r", interflop_strtok_r, interflop_strtok_r_t);
    set_h!("fgets", interflop_fgets, interflop_fgets_t);
    set_h!("free", interflop_free, interflop_free_t);
    set_h!("calloc", interflop_calloc, interflop_calloc_t);
    set_h!("argp_parse", interflop_argp_parse, interflop_argp_parse_t);
    set_h!("nanHandler", interflop_nanHandler, interflop_nanHandler_t);
    set_h!("infHandler", interflop_infHandler, interflop_infHandler_t);
    set_h!("maxHandler", interflop_maxHandler, interflop_maxHandler_t);
    set_h!(
        "cancellationHandler",
        interflop_cancellationHandler,
        interflop_cancellationHandler_t
    );
    set_h!(
        "denormalHandler",
        interflop_denormalHandler,
        interflop_denormalHandler_t
    );
    set_h!(
        "debug_print_op",
        interflop_debug_print_op,
        interflop_debug_print_op_t
    );
    set_h!(
        "gettimeofday",
        interflop_gettimeofday,
        interflop_gettimeofday_t
    );
    set_h!(
        "register_printf_specifier",
        interflop_register_printf_specifier,
        interflop_register_printf_specifier_t
    );
}
