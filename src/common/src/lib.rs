#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]

use libc::{c_char, c_double, c_int, c_uint, c_void};
use std::ffi::{CStr, CString};

#[repr(C)]
pub struct vfc_probe_node {
    pub key: *mut c_char,
    pub value: c_double,
    pub accuracyThreshold: c_double,
    pub mode: *mut c_char,
}

#[repr(C)]
pub struct vfc_probes {
    pub map: interflop_stdlib::vfc_hashmap_t,
}

#[no_mangle]
pub unsafe extern "C" fn vfc_init_probes() -> vfc_probes {
    vfc_probes {
        map: interflop_stdlib::vfc_hashmap_create(),
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_free_probes(probes: *mut vfc_probes) {
    if !probes.is_null() && !(*probes).map.is_null() {
        interflop_stdlib::vfc_hashmap_free((*probes).map);
        (*probes).map = std::ptr::null_mut();
    }
}

#[no_mangle]
pub unsafe extern "C" fn gen_probe_key(
    test_name: *mut c_char,
    var_name: *mut c_char,
) -> *mut c_char {
    if test_name.is_null() || var_name.is_null() {
        return std::ptr::null_mut();
    }
    let t = CStr::from_ptr(test_name).to_string_lossy();
    let v = CStr::from_ptr(var_name).to_string_lossy();
    let key = format!("{},{}", t, v);
    match CString::new(key) {
        Ok(c_key) => c_key.into_raw(),
        Err(_) => {
            eprintln!("Error [verificarlo]: failed to create probe key string");
            std::ptr::null_mut()
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn validate_probe_key(str_ptr: *mut c_char) {
    if str_ptr.is_null() {
        return;
    }
    let bytes = CStr::from_ptr(str_ptr).to_bytes();
    if bytes.contains(&b',') {
        eprintln!("Error: probe key contains forbidden ',' character");
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_probe_kernel(
    probes: *mut vfc_probes,
    test_name: *mut c_char,
    var_name: *mut c_char,
    val: c_double,
    accuracy_threshold: c_double,
    mode: *mut c_char,
) -> c_int {
    if probes.is_null() || (*probes).map.is_null() {
        return -1;
    }

    // Validate inputs
    if test_name.is_null() || var_name.is_null() || mode.is_null() {
        eprintln!("Error [verificarlo]: null pointer passed to vfc_probe_kernel");
        return -1;
    }

    let key_ptr = gen_probe_key(test_name, var_name);
    if key_ptr.is_null() {
        eprintln!("Error [verificarlo]: failed to generate probe key");
        return -1;
    }

    validate_probe_key(key_ptr);

    // Validate mode string
    if mode.is_null() {
        eprintln!("Error [verificarlo]: mode is null");
        libc::free(key_ptr as *mut c_void);
        return -1;
    }

    // Validate that mode is a valid C string (contains only valid UTF-8)
    if !CStr::from_ptr(mode).to_str().is_ok() {
        eprintln!("Error [verificarlo]: invalid mode string encoding");
        libc::free(key_ptr as *mut c_void);
        return -1;
    }

    let node = Box::new(vfc_probe_node {
        key: key_ptr,
        value: val,
        accuracyThreshold: accuracy_threshold,
        mode,
    });
    let key_hash = interflop_stdlib::vfc_hashmap_str_function(key_ptr);
    interflop_stdlib::vfc_hashmap_insert((*probes).map, key_hash, Box::into_raw(node) as *mut _);
    0
}

#[no_mangle]
pub unsafe extern "C" fn vfc_probe(
    probes: *mut vfc_probes,
    test_name: *mut c_char,
    var_name: *mut c_char,
    val: c_double,
) -> c_int {
    vfc_probe_kernel(probes, test_name, var_name, val, 0.0, std::ptr::null_mut())
}

#[no_mangle]
pub unsafe extern "C" fn vfc_probe_check(
    probes: *mut vfc_probes,
    test_name: *mut c_char,
    var_name: *mut c_char,
    val: c_double,
    accuracy_threshold: c_double,
) -> c_int {
    // Use a static string to avoid memory leaks
    static MODE_ABSOLUTE: once_cell::sync::Lazy<CString> =
        once_cell::sync::Lazy::new(|| CString::new("absolute").unwrap());
    vfc_probe_kernel(
        probes,
        test_name,
        var_name,
        val,
        accuracy_threshold,
        MODE_ABSOLUTE.as_ptr() as *mut c_char,
    )
}

#[no_mangle]
pub unsafe extern "C" fn vfc_probe_check_relative(
    probes: *mut vfc_probes,
    test_name: *mut c_char,
    var_name: *mut c_char,
    val: c_double,
    accuracy_threshold: c_double,
) -> c_int {
    // Use a static string to avoid memory leaks
    static MODE_RELATIVE: once_cell::sync::Lazy<CString> =
        once_cell::sync::Lazy::new(|| CString::new("relative").unwrap());
    vfc_probe_kernel(
        probes,
        test_name,
        var_name,
        val,
        accuracy_threshold,
        MODE_RELATIVE.as_ptr() as *mut c_char,
    )
}

#[no_mangle]
pub unsafe extern "C" fn vfc_num_probes(probes: *mut vfc_probes) -> c_uint {
    if probes.is_null() || (*probes).map.is_null() {
        0
    } else {
        interflop_stdlib::vfc_hashmap_num_items((*probes).map) as c_uint
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_dump_probes(probes: *mut vfc_probes) -> c_int {
    if probes.is_null() {
        return -1;
    }
    vfc_free_probes(probes);
    0
}

#[no_mangle]
pub unsafe extern "C" fn vfc_probe_f(
    probes: *mut vfc_probes,
    test_name: *mut c_char,
    var_name: *mut c_char,
    val: *mut c_double,
) -> c_int {
    if val.is_null() {
        -1
    } else {
        vfc_probe(probes, test_name, var_name, *val)
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_probe_check_f(
    probes: *mut vfc_probes,
    test_name: *mut c_char,
    var_name: *mut c_char,
    val: *mut c_double,
    accuracy_threshold: *mut c_double,
) -> c_int {
    if val.is_null() || accuracy_threshold.is_null() {
        -1
    } else {
        vfc_probe_check(probes, test_name, var_name, *val, *accuracy_threshold)
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_probe_check_relative_f(
    probes: *mut vfc_probes,
    test_name: *mut c_char,
    var_name: *mut c_char,
    val: *mut c_double,
    accuracy_threshold: *mut c_double,
) -> c_int {
    if val.is_null() || accuracy_threshold.is_null() {
        -1
    } else {
        vfc_probe_check_relative(probes, test_name, var_name, *val, *accuracy_threshold)
    }
}
