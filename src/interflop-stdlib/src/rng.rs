use libc::pid_t;
use std::sync::atomic::{AtomicI32, Ordering};

pub type XoroshiroState = [u64; 2];

#[repr(C)]
pub struct rng_state_t {
    pub choose_seed: bool,
    pub seed: u64,
    pub random_state_valid: bool,
    pub random_state: XoroshiroState,
}

#[inline]
fn rotl(x: u64, k: i32) -> u64 {
    (x << k) | (x >> (64 - k))
}

pub fn next(s: &mut XoroshiroState) -> u64 {
    let s0 = s[0];
    let mut s1 = s[1];
    let result = rotl(s0.wrapping_add(s1), 17).wrapping_add(s0);

    s1 ^= s0;
    s[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21);
    s[1] = rotl(s1, 28);

    result
}

pub fn next_double(s: &mut XoroshiroState) -> f64 {
    let val = next(s);
    let i = (0x3FFu64 << 52) | (val >> 12);
    f64::from_bits(i) - 1.0
}

pub fn next_seed(seed_state: u64) -> u64 {
    let mut z = seed_state.wrapping_add(0x9E3779B97F4A7C15);
    z = (z ^ (z >> 30)).wrapping_mul(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)).wrapping_mul(0x94D049BB133111EB);
    z ^ (z >> 31)
}

fn set_seed(random_state: &mut rng_state_t, choose_seed: bool, seed: u64) {
    if choose_seed {
        random_state.seed = seed;
    } else {
        let (tv_sec, tv_usec) = unsafe {
            let mut tv = libc::timeval {
                tv_sec: 0,
                tv_usec: 0,
            };
            libc::gettimeofday(&mut tv, std::ptr::null_mut());
            (tv.tv_sec as i64, tv.tv_usec as i64)
        };
        let tid = unsafe { libc::syscall(libc::SYS_gettid) as u64 };
        random_state.seed = (tv_sec as u64) ^ (tv_usec as u64) ^ tid;
    }
    random_state.random_state[0] = next_seed(random_state.seed);
    random_state.random_state[1] = next_seed(random_state.seed);
}

#[no_mangle]
pub unsafe extern "C" fn _get_new_tid(global_tid: *mut pid_t) -> pid_t {
    if global_tid.is_null() {
        return 0;
    }
    let atomic_ptr = global_tid as *const AtomicI32;
    (*atomic_ptr).fetch_add(1, Ordering::SeqCst) + 1
}

#[no_mangle]
pub unsafe extern "C" fn _init_rng_state_struct(
    rng_state: *mut rng_state_t,
    choose_seed: bool,
    seed: u64,
    random_state_valid: bool,
) {
    if rng_state.is_null() {
        return;
    }
    if !(*rng_state).random_state_valid {
        (*rng_state).choose_seed = choose_seed;
        (*rng_state).seed = seed;
        (*rng_state).random_state_valid = random_state_valid;
    }
}

macro_rules! init_random_state {
    ($rng_state:expr, $global_tid:expr) => {
        if !(*$rng_state).random_state_valid {
            if (*$rng_state).choose_seed {
                let seed = (*$rng_state).seed ^ (_get_new_tid($global_tid) as u64);
                set_seed(&mut *$rng_state, true, seed);
            } else {
                set_seed(&mut *$rng_state, false, 0);
            }
            (*$rng_state).random_state_valid = true;
        }
    };
}

#[no_mangle]
pub unsafe extern "C" fn get_rand_uint32(
    rng_state: *mut rng_state_t,
    global_tid: *mut pid_t,
) -> u32 {
    init_random_state!(rng_state, global_tid);
    let val = next(&mut (*rng_state).random_state);
    val as u32
}

#[no_mangle]
pub unsafe extern "C" fn get_rand_uint64(
    rng_state: *mut rng_state_t,
    global_tid: *mut pid_t,
) -> u64 {
    init_random_state!(rng_state, global_tid);
    next(&mut (*rng_state).random_state)
}

#[no_mangle]
pub unsafe extern "C" fn get_rand_double01(
    rng_state: *mut rng_state_t,
    global_tid: *mut pid_t,
) -> f64 {
    init_random_state!(rng_state, global_tid);
    next_double(&mut (*rng_state).random_state)
}
