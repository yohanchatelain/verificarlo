#include "interflop/rng/vfc_rng.h"
#include <stdint.h>

pid_t rand_global_tid = 0;
static __thread rng_state_t rng_state;

float get_rand_float() { 
    
    const unint64_t =  get_rand_uint64(&rng_state, rand_global_tid); 
    // update the random state
    
    }

double get_rand_double() { return interflop_get_rand_double(); }