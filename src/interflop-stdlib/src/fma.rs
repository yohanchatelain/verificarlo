#[no_mangle]
pub extern "C" fn interflop_fma_binary32(a: f32, b: f32, c: f32) -> f32 {
    a.mul_add(b, c)
}

#[no_mangle]
pub extern "C" fn interflop_fma_binary64(a: f64, b: f64, c: f64) -> f64 {
    a.mul_add(b, c)
}

#[no_mangle]
pub extern "C" fn interflop_fma_binary128(a: u128, b: u128, c: u128) -> u128 {
    // Basic 128-bit float / quad FMA shim
    a.wrapping_mul(b).wrapping_add(c)
}
