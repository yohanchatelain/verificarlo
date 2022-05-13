/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *                                                                           *\
 *  Copyright (c) 2015                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *     CMLA, Ecole Normale Superieure de Cachan                              *\
 *                                                                           *\
 *  Copyright (c) 2018                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *                                                                           *\
 *  Copyright (c) 2019-2021                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h> // for getting the thread id
#include <sys/time.h>
#include <sys/types.h>
/* Modern solution for working with threads, since C11 */
/*  currently, support for threads.h is available from glibc2.28 onwards */
// #include <threads.h>
#include <unistd.h>

#include "options.h"

uint64_t next_seed(uint64_t seed_state) {
  uint64_t z = (seed_state += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

static inline uint64_t rotl(const uint64_t x, int k) {
  return (x << k) | (x >> (64 - k));
}

uint64_t next(rng_state_t *rng_state) {
  const uint64_t s0 = rng_state->random_state[0];
  uint64_t s1 = rng_state->random_state[1];
  const uint64_t result = rotl(s0 + s1, 17) + s0;

  s1 ^= s0;
  rng_state->random_state[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21); // a, b
  rng_state->random_state[1] = rotl(s1, 28);                   // c

  return result;
}

static inline double to_double(uint64_t x) {
  const union {
    uint64_t i;
    double d;
  } u = {.i = UINT64_C(0x3FF) << 52 | x >> 12};
  return u.d - 1.0;
}

/* Generic set_seed function which is common for most of the backends */
/* @param random state pointer to the internal state of the RNG */
/* @param choose_seed whether to set the seed to a user-provided value */
/* @param seed the user-provided seed for the RNG */
static void _set_seed(rng_state_t *random_state, const bool choose_seed,
                      const uint64_t seed) {
  if (choose_seed) {
    random_state->seed = seed;
  } else {
    /* TODO: specialized for Intel arch with rdseed */
    struct timeval t1;
    gettimeofday(&t1, NULL);
    /* Hopefully the following seed is good enough for Montercarlo */
    random_state->seed = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);
  }
  random_state->random_state[0] = next_seed(random_state->seed);
  random_state->random_state[1] = next_seed(random_state->seed);
}

/* Output a floating point number r (0.0 < r < 1.0) */
static double _generate_random_double(rng_state_t *random_state) {
  return to_double(next(random_state));
}

/* Initialize a data structure used to hold the information required */
/* by the RNG */
void _init_rng_state_struct(rng_state_t *rng_state, bool choose_seed,
                            uint64_t seed, bool random_state_valid) {
  if (rng_state->random_state_valid == false) {
    rng_state->choose_seed = choose_seed;
    rng_state->seed = seed;
    rng_state->random_state_valid = random_state_valid;
  }
}

#define _INIT_RANDOM_STATE(RANDOM_STATE, GLOBAL_TID)                           \
  {                                                                            \
    if (RANDOM_STATE->random_state_valid == false) {                           \
      if (RANDOM_STATE->choose_seed == true) {                                 \
        _set_seed(RANDOM_STATE, RANDOM_STATE->choose_seed,                     \
                  RANDOM_STATE->seed ^ _get_new_tid(GLOBAL_TID));              \
      } else {                                                                 \
        _set_seed(RANDOM_STATE, false, 0);                                     \
      }                                                                        \
      RANDOM_STATE->random_state_valid = true;                                 \
    }                                                                          \
  }

/* Get a new identifier for the calling thread  */
/* Generic threads can have inconsistent identifiers, assigned by the system, */
/* we therefore need to set an order between threads, for the case */
/* when the seed is fixed, to insure some repeatability between executions */
unsigned long long int _get_new_tid(pid_t *global_tid) {
  return __atomic_add_fetch(global_tid, 1, __ATOMIC_SEQ_CST);
}

/* Returns a 32-bit unsigned integer r (0 <= r < 2^32) */
uint32_t _get_rand_uint32(rng_state_t *rng_state, pid_t *global_tid) {
  _INIT_RANDOM_STATE(rng_state, global_tid);
  const union {
    uint64_t u64;
    uint32_t u32[2];
  } u = {.u64 = next(rng_state)};
  return u.u32[1];
}

/* Returns a 64-bit unsigned integer r (0 <= r < 2^64) */
uint64_t _get_rand_uint64(rng_state_t *rng_state, pid_t *global_tid) {
  _INIT_RANDOM_STATE(rng_state, global_tid);
  return next(rng_state);
}

/* Returns a random double in the (0,1) open interval */
double _get_rand(rng_state_t *rng_state, pid_t *global_tid) {
  _INIT_RANDOM_STATE(rng_state, global_tid);
  return _generate_random_double(rng_state);
}

/* Returns a bool for determining whether an operation should skip */
/* perturbation. false -> perturb; true -> skip. */
/* e.g. for sparsity=0.1, all random values > 0.1 = true -> no MCA*/
bool _mca_skip_eval(const float sparsity, rng_state_t *rng_state,
                    pid_t *global_tid) {
  if (sparsity >= 1.0f) {
    return false;
  }

  return (_get_rand(rng_state, global_tid) > sparsity);
}
