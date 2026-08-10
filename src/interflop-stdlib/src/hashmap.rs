use libc::{c_char, c_void, size_t};
use std::collections::HashMap;
use std::ffi::CStr;

#[repr(C)]
pub struct vfc_hashmap_st {
    pub nbits: size_t,
    pub mask: size_t,
    pub capacity: size_t,
    pub items: *mut size_t,
    pub nitems: size_t,
    pub n_deleted_items: size_t,
    // Internal Rust map storage backing FFI pointer handle
    map: HashMap<size_t, *mut c_void>,
}

pub type vfc_hashmap_t = *mut vfc_hashmap_st;

#[no_mangle]
pub unsafe extern "C" fn vfc_hashmap_create() -> vfc_hashmap_t {
    let map = Box::new(vfc_hashmap_st {
        nbits: 0,
        mask: 0,
        capacity: 0,
        items: std::ptr::null_mut(),
        nitems: 0,
        n_deleted_items: 0,
        map: HashMap::new(),
    });
    Box::into_raw(map)
}

#[no_mangle]
pub unsafe extern "C" fn vfc_hashmap_destroy(map: vfc_hashmap_t) {
    if !map.is_null() {
        let _ = Box::from_raw(map);
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_hashmap_free(map: vfc_hashmap_t) {
    if !map.is_null() {
        let map_ref = &mut *map;
        for (_, ptr) in map_ref.map.drain() {
            if !ptr.is_null() && ptr as usize > 1 {
                libc::free(ptr);
            }
        }
        map_ref.nitems = 0;
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_hashmap_insert(map: vfc_hashmap_t, key: size_t, item: *mut c_void) {
    if !map.is_null() {
        let map_ref = &mut *map;
        map_ref.map.insert(key, item);
        map_ref.nitems = map_ref.map.len();
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_hashmap_remove(map: vfc_hashmap_t, key: size_t) {
    if !map.is_null() {
        let map_ref = &mut *map;
        map_ref.map.remove(&key);
        map_ref.nitems = map_ref.map.len();
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_hashmap_have(map: vfc_hashmap_t, key: size_t) -> c_char {
    if !map.is_null() {
        if (*map).map.contains_key(&key) {
            1
        } else {
            0
        }
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_hashmap_get(map: vfc_hashmap_t, key: size_t) -> *mut c_void {
    if !map.is_null() {
        (*map)
            .map
            .get(&key)
            .copied()
            .unwrap_or(std::ptr::null_mut())
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_hashmap_num_items(map: vfc_hashmap_t) -> size_t {
    if !map.is_null() {
        (*map).map.len()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn vfc_hashmap_str_function(id: *const c_char) -> size_t {
    if id.is_null() {
        return 0;
    }
    let bytes = CStr::from_ptr(id).to_bytes();
    let mut hash: size_t = 5381;
    for &b in bytes {
        hash = ((hash << 5).wrapping_add(hash)).wrapping_add(b as size_t);
    }
    hash
}

#[no_mangle]
pub unsafe extern "C" fn get_value_at(items: *mut size_t, i: size_t) -> size_t {
    if items.is_null() {
        0
    } else {
        *items.add(i * 2 + 1)
    }
}

#[no_mangle]
pub unsafe extern "C" fn get_key_at(items: *mut size_t, i: size_t) -> size_t {
    if items.is_null() {
        0
    } else {
        *items.add(i * 2)
    }
}

#[no_mangle]
pub unsafe extern "C" fn set_value_at(items: *mut size_t, value: size_t, i: size_t) {
    if !items.is_null() {
        *items.add(i * 2 + 1) = value;
    }
}

#[no_mangle]
pub unsafe extern "C" fn set_key_at(items: *mut size_t, key: size_t, i: size_t) {
    if !items.is_null() {
        *items.add(i * 2) = key;
    }
}
