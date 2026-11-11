/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *  Copyright (c) 2026                                                       *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#define _GNU_SOURCE
#include "sentinel.h"

#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/personality.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "funcinstr.h"
#include "interflop/hashmap/vfc_hashmap.h"
#include "interflop/iostream/logger.h"

#ifdef __x86_64__
#define VFC_CALL_OP_SIZE 5
#else
#define VFC_CALL_OP_SIZE 4
#endif

#ifndef ADDR2LINE_PATH
#define ADDR2LINE_PATH "llvm-addr2line"
#endif

unsigned char vfc_sentinel_enabled = 0;
unsigned char vfc_sentinel_sync_func = 0;

static vfc_sentinel_shm_t *g_shm = NULL;
static vfc_sentinel_replica_ring_t *g_ring = NULL;
static int g_replica_id = -1;
static int g_is_coordinator = 0;
static uint64_t g_local_batch_idx = 0;
static uint32_t g_local_rec_idx = 0;
static uint64_t g_local_seq = 0;
static int g_atexit_done = 0;

/* Flight recorder entry for the validator's validated prefix history */
typedef struct {
  uint64_t seq;
  vfc_sync_record_t rec;
} vfc_history_entry_t;

/* =========================================================================
 * IEEE-754 Predicate Evaluation & Formatting Helpers
 * ========================================================================= */

static const char *fcmp_predicate_name(uint16_t p) {
  switch (p) {
  case FCMP_FALSE:
    return "FCMP_FALSE (false)";
  case FCMP_OEQ:
    return "FCMP_OEQ (a == b)";
  case FCMP_OGT:
    return "FCMP_OGT (a > b)";
  case FCMP_OGE:
    return "FCMP_OGE (a >= b)";
  case FCMP_OLT:
    return "FCMP_OLT (a < b)";
  case FCMP_OLE:
    return "FCMP_OLE (a <= b)";
  case FCMP_ONE:
    return "FCMP_ONE (a != b, ordered)";
  case FCMP_ORD:
    return "FCMP_ORD (!isnan(a) && !isnan(b))";
  case FCMP_UNO:
    return "FCMP_UNO (isnan(a) || isnan(b))";
  case FCMP_UEQ:
    return "FCMP_UEQ (unordered || a == b)";
  case FCMP_UGT:
    return "FCMP_UGT (unordered || a > b)";
  case FCMP_UGE:
    return "FCMP_UGE (unordered || a >= b)";
  case FCMP_ULT:
    return "FCMP_ULT (unordered || a < b)";
  case FCMP_ULE:
    return "FCMP_ULE (unordered || a <= b)";
  case FCMP_UNE:
    return "FCMP_UNE (unordered || a != b)";
  case FCMP_TRUE:
    return "FCMP_TRUE (true)";
  default:
    return "FCMP_UNKNOWN";
  }
}

static const char *sync_kind_name(uint8_t kind) {
  switch (kind) {
  case VFC_SYNC_FCMP_FLOAT:
    return "FCMP_FLOAT (binary32)";
  case VFC_SYNC_FCMP_DOUBLE:
    return "FCMP_DOUBLE (binary64)";
  case VFC_SYNC_FCMP_VEC_FLOAT:
    return "FCMP_VEC_FLOAT (SIMD binary32)";
  case VFC_SYNC_FCMP_VEC_DOUBLE:
    return "FCMP_VEC_DOUBLE (SIMD binary64)";
  case VFC_SYNC_FPTOI_FLOAT:
    return "FPTOI_FLOAT (float -> int)";
  case VFC_SYNC_FPTOI_DOUBLE:
    return "FPTOI_DOUBLE (double -> int)";
  case VFC_SYNC_FUNC_ENTER:
    return "FUNC_ENTER";
  case VFC_SYNC_FUNC_EXIT:
    return "FUNC_EXIT";
  case VFC_SYNC_USER_PROBE:
    return "USER_SYNC_PROBE";
  default:
    return "UNKNOWN_SYNC";
  }
}

static const char *divergence_reason_name(vfc_divergence_reason_t reason) {
  switch (reason) {
  case VFC_DIV_NUMERICAL_BRANCH:
    return "NUMERICAL_BRANCH_DIVERGENCE";
  case VFC_DIV_FPTOI_MISMATCH:
    return "FPTOI_CONVERSION_DIVERGENCE";
  case VFC_DIV_USER_PROBE_MISMATCH:
    return "USER_PROBE_VALUE_DIVERGENCE";
  case VFC_DIV_TRACE_SITE_MISMATCH:
    return "CONTROL_FLOW_TRACE_MISMATCH";
  case VFC_DIV_PREMATURE_EOF:
    return "PREMATURE_REPLICA_TERMINATION";
  case VFC_DIV_REPLICA_CRASH:
    return "REPLICA_SIGNAL_CRASH";
  default:
    return "NONE";
  }
}

int vfc_eval_fcmp_double(enum FCMP_PREDICATE p, double a, double b) {
  switch (p) {
  case FCMP_FALSE:
    return 0;
  case FCMP_OEQ:
    return !isunordered(a, b) && (a == b);
  case FCMP_OGT:
    return isgreater(a, b);
  case FCMP_OGE:
    return isgreaterequal(a, b);
  case FCMP_OLT:
    return isless(a, b);
  case FCMP_OLE:
    return islessequal(a, b);
  case FCMP_ONE:
    return islessgreater(a, b);
  case FCMP_ORD:
    return !isunordered(a, b);
  case FCMP_UNO:
    return isunordered(a, b);
  case FCMP_UEQ:
    return isunordered(a, b) || (a == b);
  case FCMP_UGT:
    return isunordered(a, b) || isgreater(a, b);
  case FCMP_UGE:
    return isunordered(a, b) || isgreaterequal(a, b);
  case FCMP_ULT:
    return isunordered(a, b) || isless(a, b);
  case FCMP_ULE:
    return isunordered(a, b) || islessequal(a, b);
  case FCMP_UNE:
    return isunordered(a, b) || islessgreater(a, b);
  case FCMP_TRUE:
    return 1;
  default:
    return 0;
  }
}

int vfc_eval_fcmp_float(enum FCMP_PREDICATE p, float a, float b) {
  return vfc_eval_fcmp_double(p, (double)a, (double)b);
}

static uint64_t ulp_distance_double(double a, double b) {
  if (isnan(a) || isnan(b))
    return UINT64_MAX;
  if (a == b)
    return 0;
  if (isinf(a) || isinf(b))
    return UINT64_MAX - 1;
  union {
    double d;
    int64_t i;
    uint64_t u;
  } ua = {.d = a}, ub = {.d = b};
  int64_t ia = ua.i < 0 ? (int64_t)(0x8000000000000000ULL - ua.u) : ua.i;
  int64_t ib = ub.i < 0 ? (int64_t)(0x8000000000000000ULL - ub.u) : ub.i;
  return (ia >= ib) ? (uint64_t)(ia - ib) : (uint64_t)(ib - ia);
}

static uint64_t ulp_distance_float(float a, float b) {
  if (isnan(a) || isnan(b))
    return UINT64_MAX;
  if (a == b)
    return 0;
  if (isinf(a) || isinf(b))
    return UINT64_MAX - 1;
  union {
    float f;
    int32_t i;
    uint32_t u;
  } ua = {.f = a}, ub = {.f = b};
  int32_t ia = ua.i < 0 ? (int32_t)(0x80000000U - ua.u) : ua.i;
  int32_t ib = ub.i < 0 ? (int32_t)(0x80000000U - ub.u) : ub.i;
  return (ia >= ib) ? (uint64_t)(ia - ib) : (uint64_t)(ib - ia);
}

static uint64_t double_to_raw_bits(double x) {
  union {
    double d;
    uint64_t u;
  } u = {.d = x};
  return u.u;
}

static uint32_t float_to_raw_bits(float x) {
  union {
    float f;
    uint32_t u;
  } u = {.f = x};
  return u.u;
}

/* =========================================================================
 * Address Resolution & Backtrace Capture
 * ========================================================================= */

static void capture_replica_diagnostics(void) {
  if (!g_ring)
    return;
  void *frames[VFC_SENTINEL_MAX_BT_FRAMES];
  int n = backtrace(frames, VFC_SENTINEL_MAX_BT_FRAMES);
  if (n < 0)
    n = 0;
  g_ring->bt_count = (uint32_t)n;
  for (int i = 0; i < n; i++) {
    uintptr_t addr = (uintptr_t)frames[i];
    if (i > 0 && addr > VFC_CALL_OP_SIZE)
      addr -= VFC_CALL_OP_SIZE;
    g_ring->bt_addrs[i] = (uint64_t)addr;
  }
  vfc_format_call_stack(g_ring->call_stack_str, sizeof(g_ring->call_stack_str));
  atomic_store_explicit(&g_ring->bt_ready, 1, memory_order_release);
}

static void sentinel_sigusr1_handler(int sig) {
  (void)sig;
  capture_replica_diagnostics();
}

static void run_addr2line_cmd(const char *binary_path, uint64_t addr,
                              char *out_buf, size_t out_size) {
  out_buf[0] = '\0';
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    snprintf(out_buf, out_size, "0x%" PRIx64, addr);
    return;
  }
  pid_t pid = fork();
  if (pid == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) {
      dup2(devnull, STDERR_FILENO);
      close(devnull);
    }
    close(pipefd[1]);
    char addr_str[32];
    snprintf(addr_str, sizeof(addr_str), "0x%" PRIx64, addr);
    execlp(ADDR2LINE_PATH, ADDR2LINE_PATH, "-fpaCs", "-i", "--no-debuginfod",
           "-e", binary_path, addr_str, (char *)NULL);
    execlp("llvm-addr2line", "llvm-addr2line", "-fpaCs", "-i", "-e",
           binary_path, addr_str, (char *)NULL);
    execlp("addr2line", "addr2line", "-fpaCs", "-i", "-e", binary_path,
           addr_str, (char *)NULL);
    _exit(127);
  } else if (pid > 0) {
    close(pipefd[1]);
    ssize_t total = 0;
    while ((size_t)total + 1 < out_size) {
      ssize_t r =
          read(pipefd[0], out_buf + total, out_size - 1 - (size_t)total);
      if (r <= 0)
        break;
      total += r;
    }
    out_buf[total] = '\0';
    close(pipefd[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    /* Trim trailing newline */
    while (total > 0 &&
           (out_buf[total - 1] == '\n' || out_buf[total - 1] == '\r')) {
      out_buf[--total] = '\0';
    }
  } else {
    close(pipefd[0]);
    close(pipefd[1]);
    snprintf(out_buf, out_size, "0x%" PRIx64, addr);
  }
}

static void resolve_site_location(const char *default_exe, uint64_t addr,
                                  char *out_buf, size_t out_size) {
  if (addr == 0) {
    snprintf(out_buf, out_size, "<none>");
    return;
  }
  const char *bin_path = default_exe;
  Dl_info dli;
  memset(&dli, 0, sizeof(dli));
  if (dladdr((void *)(uintptr_t)addr, &dli) && dli.dli_fname &&
      dli.dli_fname[0]) {
    bin_path = dli.dli_fname;
  }

  run_addr2line_cmd(bin_path, addr, out_buf, out_size);
  if ((strstr(out_buf, "??:0") != NULL || strstr(out_buf, "??:?") != NULL ||
       out_buf[0] == '\0') &&
      dli.dli_fbase != NULL && addr > (uintptr_t)dli.dli_fbase) {
    uint64_t rel_addr = addr - (uintptr_t)dli.dli_fbase;
    char retry_buf[512];
    run_addr2line_cmd(bin_path, rel_addr, retry_buf, sizeof(retry_buf));
    if (retry_buf[0] != '\0' && strstr(retry_buf, "??:0") == NULL &&
        strstr(retry_buf, "??:?") == NULL) {
      snprintf(out_buf, out_size, "%s", retry_buf);
    }
  }
  if (out_buf[0] == '\0') {
    snprintf(out_buf, out_size, "0x%" PRIx64, addr);
  }
}

/* =========================================================================
 * Replica Fast-Path Recording & Batch Publication
 * ========================================================================= */

static inline void sentinel_publish_current_batch(void) {
  if (!g_ring || g_local_rec_idx == 0)
    return;

  uint32_t B = g_shm->num_buffers_b;
  vfc_sentinel_batch_t *batch = &g_ring->batches[g_local_batch_idx % B];
  atomic_store_explicit(&batch->count, g_local_rec_idx, memory_order_release);
  atomic_store_explicit(&batch->published, 1, memory_order_release);
  atomic_store_explicit(&g_ring->write_batch_idx, g_local_batch_idx + 1,
                        memory_order_release);

  g_local_batch_idx++;
  g_local_rec_idx = 0;
}

static inline int32_t sentinel_record_event(vfc_sync_record_t rec) {
  if (!vfc_sentinel_enabled || !g_shm || !g_ring)
    return rec.outcome;

  if (atomic_load_explicit(&g_shm->stop_flag, memory_order_relaxed) != 0) {
    capture_replica_diagnostics();
    _exit(0);
  }

  uint32_t k = g_shm->batch_size_k;
  uint32_t B = g_shm->num_buffers_b;

  if (g_local_rec_idx == 0) {
    /* Wait if all B ring buffers are currently unconsumed (backpressure) */
    while (g_local_batch_idx - atomic_load_explicit(&g_ring->read_batch_idx,
                                                    memory_order_acquire) >=
           B) {
      if (atomic_load_explicit(&g_shm->stop_flag, memory_order_relaxed) != 0) {
        capture_replica_diagnostics();
        _exit(0);
      }
      sched_yield();
    }
    vfc_sentinel_batch_t *batch = &g_ring->batches[g_local_batch_idx % B];
    batch->seq_start = g_local_seq;
    atomic_store_explicit(&batch->published, 0, memory_order_relaxed);
    atomic_store_explicit(&batch->count, 0, memory_order_relaxed);
  }

  uint64_t cur_seq = g_local_seq++;
  vfc_sentinel_batch_t *batch = &g_ring->batches[g_local_batch_idx % B];
  batch->records[g_local_rec_idx] = rec;
  g_local_rec_idx++;

  /* Make count visible for partial-buffer inspection if replica later blocks */
  atomic_store_explicit(&batch->count, g_local_rec_idx, memory_order_release);

  if (g_local_rec_idx >= k) {
    sentinel_publish_current_batch();

    /* In strict lockstep mode (k == 1), wait for validator decision */
    if (k == 1) {
      while (atomic_load_explicit(&g_shm->lockstep_seq, memory_order_acquire) <=
             cur_seq) {
        int sflag =
            atomic_load_explicit(&g_shm->stop_flag, memory_order_acquire);
        if (sflag != 0) {
          capture_replica_diagnostics();
          _exit(0);
        }
        sched_yield();
      }
      if (g_shm->policy == VFC_POLICY_REFERENCE ||
          g_shm->policy == VFC_POLICY_MAJORITY) {
        return atomic_load_explicit(&g_shm->lockstep_outcome,
                                    memory_order_acquire);
      }
    } else if (atomic_load_explicit(&g_shm->stop_flag, memory_order_relaxed) !=
               0) {
      capture_replica_diagnostics();
      _exit(0);
    }
  }

  return rec.outcome;
}

int vfc_sentinel_record_cmp_float(uint64_t site_id, enum FCMP_PREDICATE p,
                                  float a, float b, int outcome) {
  vfc_sync_record_t rec = {
      .site_id = site_id,
      .op_a = (double)a,
      .op_b = (double)b,
      .outcome = outcome ? 1 : 0,
      .predicate = (uint16_t)p,
      .kind = VFC_SYNC_FCMP_FLOAT,
      .vec_size = 1,
  };
  return (int)sentinel_record_event(rec);
}

int vfc_sentinel_record_cmp_double(uint64_t site_id, enum FCMP_PREDICATE p,
                                   double a, double b, int outcome) {
  vfc_sync_record_t rec = {
      .site_id = site_id,
      .op_a = a,
      .op_b = b,
      .outcome = outcome ? 1 : 0,
      .predicate = (uint16_t)p,
      .kind = VFC_SYNC_FCMP_DOUBLE,
      .vec_size = 1,
  };
  return (int)sentinel_record_event(rec);
}

void vfc_sentinel_record_vec_cmp_float(uint64_t site_id, enum FCMP_PREDICATE p,
                                       const float *a, const float *b,
                                       int *outcomes, int size) {
  int32_t mask = 0;
  for (int j = 0; j < size && j < 16; j++) {
    if (outcomes[j])
      mask |= (1 << j);
  }
  vfc_sync_record_t rec = {
      .site_id = site_id,
      .op_a = (double)a[0],
      .op_b = (double)b[0],
      .outcome = mask,
      .predicate = (uint16_t)p,
      .kind = VFC_SYNC_FCMP_VEC_FLOAT,
      .vec_size = (uint8_t)size,
  };
  int32_t synced_mask = sentinel_record_event(rec);
  if (g_shm && g_shm->batch_size_k == 1 &&
      (g_shm->policy == VFC_POLICY_REFERENCE ||
       g_shm->policy == VFC_POLICY_MAJORITY)) {
    for (int j = 0; j < size && j < 16; j++) {
      outcomes[j] = (synced_mask & (1 << j)) ? 1 : 0;
    }
  }
}

void vfc_sentinel_record_vec_cmp_double(uint64_t site_id, enum FCMP_PREDICATE p,
                                        const double *a, const double *b,
                                        int *outcomes, int size) {
  int32_t mask = 0;
  for (int j = 0; j < size && j < 16; j++) {
    if (outcomes[j])
      mask |= (1 << j);
  }
  vfc_sync_record_t rec = {
      .site_id = site_id,
      .op_a = a[0],
      .op_b = b[0],
      .outcome = mask,
      .predicate = (uint16_t)p,
      .kind = VFC_SYNC_FCMP_VEC_DOUBLE,
      .vec_size = (uint8_t)size,
  };
  int32_t synced_mask = sentinel_record_event(rec);
  if (g_shm && g_shm->batch_size_k == 1 &&
      (g_shm->policy == VFC_POLICY_REFERENCE ||
       g_shm->policy == VFC_POLICY_MAJORITY)) {
    for (int j = 0; j < size && j < 16; j++) {
      outcomes[j] = (synced_mask & (1 << j)) ? 1 : 0;
    }
  }
}

int64_t vfc_sentinel_record_fptoi_float(uint64_t site_id, int is_signed,
                                        float a, int64_t result) {
  vfc_sync_record_t rec = {
      .site_id = site_id,
      .op_a = (double)a,
      .op_b = 0.0,
      .outcome = (int32_t)result,
      .predicate = (uint16_t)(is_signed ? 1 : 0),
      .kind = VFC_SYNC_FPTOI_FLOAT,
      .vec_size = 1,
  };
  int32_t synced = sentinel_record_event(rec);
  if (g_shm && g_shm->batch_size_k == 1 &&
      (g_shm->policy == VFC_POLICY_REFERENCE ||
       g_shm->policy == VFC_POLICY_MAJORITY)) {
    return (int64_t)synced;
  }
  return result;
}

int64_t vfc_sentinel_record_fptoi_double(uint64_t site_id, int is_signed,
                                         double a, int64_t result) {
  vfc_sync_record_t rec = {
      .site_id = site_id,
      .op_a = a,
      .op_b = 0.0,
      .outcome = (int32_t)result,
      .predicate = (uint16_t)(is_signed ? 1 : 0),
      .kind = VFC_SYNC_FPTOI_DOUBLE,
      .vec_size = 1,
  };
  int32_t synced = sentinel_record_event(rec);
  if (g_shm && g_shm->batch_size_k == 1 &&
      (g_shm->policy == VFC_POLICY_REFERENCE ||
       g_shm->policy == VFC_POLICY_MAJORITY)) {
    return (int64_t)synced;
  }
  return result;
}

void vfc_sentinel_record_func(uint64_t site_id, const char *func_name,
                              int is_enter) {
  if (!vfc_sentinel_enabled || !vfc_sentinel_sync_func)
    return;
  uint64_t name_hash = func_name ? vfc_hashmap_str_function(func_name) : 0;
  vfc_sync_record_t rec = {
      .site_id = site_id ? site_id : name_hash,
      .op_a = 0.0,
      .op_b = 0.0,
      .outcome = is_enter ? 1 : 0,
      .predicate = 0,
      .kind = (uint8_t)(is_enter ? VFC_SYNC_FUNC_ENTER : VFC_SYNC_FUNC_EXIT),
      .vec_size = 1,
  };
  (void)sentinel_record_event(rec);
}

void vfc_sentinel_record_user_probe(uint64_t site_id, const char *label,
                                    double value) {
  if (!vfc_sentinel_enabled || !g_ring)
    return;
  if (label) {
    strncpy(g_ring->last_probe_label, label,
            sizeof(g_ring->last_probe_label) - 1);
    g_ring->last_probe_label[sizeof(g_ring->last_probe_label) - 1] = '\0';
  }
  uint64_t label_hash = label ? vfc_hashmap_str_function(label) : 0;
  vfc_sync_record_t rec = {
      .site_id = site_id ? site_id : label_hash,
      .op_a = value,
      .op_b = 0.0,
      .outcome = (int32_t)(double_to_raw_bits(value) ^
                           (double_to_raw_bits(value) >> 32)),
      .predicate = (uint16_t)(label_hash & 0xFFFFU),
      .kind = VFC_SYNC_USER_PROBE,
      .vec_size = 1,
  };
  (void)sentinel_record_event(rec);
}

void vfc_sentinel_flush(void) {
  if (!vfc_sentinel_enabled || !g_ring)
    return;
  sentinel_publish_current_batch();
}

void vfc_sync_point(const char *label, double val) {
  uint64_t site_id = (uintptr_t)__builtin_return_address(0) - VFC_CALL_OP_SIZE;
  vfc_sentinel_record_user_probe(site_id, label, val);
}

void vfc_sync_flush(void) { vfc_sentinel_flush(); }

/* =========================================================================
 * Full Provenance Diagnostic Report Generator
 * ========================================================================= */

/* Student t-distribution 95% two-tailed critical values for df = 1..15 */
static double student_t95(uint32_t n_replicas) {
  static const double t_table[] = {
      12.706, /* df=1 (N=2) */
      4.303,  /* df=2 (N=3, CADNA standard) */
      3.182,  /* df=3 (N=4) */
      2.776,  /* df=4 (N=5) */
      2.571,  /* df=5 (N=6) */
      2.447,  /* df=6 (N=7) */
      2.365,  /* df=7 (N=8) */
      2.306,  /* df=8 (N=9) */
      2.262   /* df=9 (N=10) */
  };
  if (n_replicas < 2)
    return 0.0;
  uint32_t df = n_replicas - 1;
  if (df <= 9)
    return t_table[df - 1];
  return 1.96;
}

static void write_json_report(const char *json_path,
                              vfc_divergence_reason_t reason, uint64_t seq,
                              uint64_t site_occurrence,
                              const vfc_sync_record_t *recs,
                              const char *site_loc, double mean_diff,
                              double std_diff, double sig_bits,
                              int is_comp_zero) {
  if (!json_path || !json_path[0])
    return;
  FILE *fp = fopen(json_path, "w");
  if (!fp)
    return;

  uint32_t N = g_shm->num_replicas;
  const char *status_str =
      (reason == VFC_DIV_NONE) ? "OK" : divergence_reason_name(reason);
  fprintf(fp, "{\n");
  fprintf(fp, "  \"status\": \"%s\",\n", status_str);
  fprintf(fp, "  \"diverged\": %s,\n",
          reason != VFC_DIV_NONE ? "true" : "false");
  fprintf(fp, "  \"reason\": \"%s\",\n", divergence_reason_name(reason));
  fprintf(fp, "  \"sequence_number\": %" PRIu64 ",\n", seq);
  fprintf(fp, "  \"divergence_sequence\": %" PRIu64 ",\n",
          (reason != VFC_DIV_NONE) ? (seq + 1) : 0);
  fprintf(fp, "  \"site_occurrence\": %" PRIu64 ",\n", site_occurrence);
  fprintf(fp, "  \"validated_events\": %" PRIu64 ",\n",
          atomic_load(&g_shm->validated_events));
  fprintf(fp, "  \"validated_sync_points\": %" PRIu64 ",\n",
          atomic_load(&g_shm->validated_events));
  fprintf(fp, "  \"disagreement_count\": %" PRIu64 ",\n",
          atomic_load(&g_shm->disagreement_count));
  fprintf(fp, "  \"batch_size_k\": %u,\n", g_shm->batch_size_k);
  fprintf(fp, "  \"num_buffers_b\": %u,\n", g_shm->num_buffers_b);
  fprintf(fp, "  \"num_replicas\": %u%s\n", N,
          (reason != VFC_DIV_NONE && recs != NULL) ? "," : "");

  if (reason != VFC_DIV_NONE && recs != NULL) {
    fprintf(fp, "  \"site_id\": \"0x%" PRIx64 "\",\n", recs[0].site_id);
    fprintf(fp, "  \"location\": \"%s\",\n", site_loc ? site_loc : "");
    fprintf(fp, "  \"sync_kind\": \"%s\",\n", sync_kind_name(recs[0].kind));
    fprintf(fp, "  \"predicate\": \"%s\",\n",
            fcmp_predicate_name(recs[0].predicate));
    fprintf(fp, "  \"ensemble_diff_mean\": %.17g,\n", mean_diff);
    fprintf(fp, "  \"ensemble_diff_std\": %.17g,\n", std_diff);
    fprintf(fp, "  \"significant_bits\": %.4f,\n", sig_bits);
    fprintf(fp, "  \"computational_zero\": %s,\n",
            is_comp_zero ? "true" : "false");
    fprintf(fp, "  \"replicas\": [\n");
    for (uint32_t r = 0; r < N; r++) {
      uint64_t ulp =
          (recs[r].kind == VFC_SYNC_FCMP_FLOAT ||
           recs[r].kind == VFC_SYNC_FCMP_VEC_FLOAT)
              ? ulp_distance_float((float)recs[r].op_a, (float)recs[r].op_b)
              : ulp_distance_double(recs[r].op_a, recs[r].op_b);
      fprintf(fp,
              "    {\"replica\": %u, \"pid\": %d, \"seed\": %" PRIu64
              ", \"site_id\": \"0x%" PRIx64 "\", \"outcome\": %d"
              ", \"op_a\": %.17g, \"op_b\": %.17g, \"diff\": %.17g"
              ", \"ulp_distance\": %" PRIu64 "}%s\n",
              r, (int)g_shm->rings[r].pid, g_shm->rings[r].seed,
              recs[r].site_id, recs[r].outcome, recs[r].op_a, recs[r].op_b,
              recs[r].op_a - recs[r].op_b, ulp, (r + 1 < N) ? "," : "");
    }
    fprintf(fp, "  ]\n");
  }
  fprintf(fp, "}\n");
  fclose(fp);
}

static void report_provenance_diagnostics(
    vfc_divergence_reason_t reason, uint64_t seq, uint64_t site_occurrence,
    const vfc_sync_record_t *recs, const vfc_history_entry_t *history,
    size_t history_count, size_t history_head) {

  uint32_t N = g_shm->num_replicas;

  /* Signal all active replicas to capture their live backtrace and call stack
   */
  atomic_store_explicit(&g_shm->stop_flag, 1, memory_order_release);
  for (uint32_t r = 0; r < N; r++) {
    if (g_shm->rings[r].pid > 0 &&
        atomic_load_explicit(&g_shm->rings[r].eof, memory_order_acquire) == 0) {
      kill(g_shm->rings[r].pid, SIGUSR1);
    }
  }
  /* Wait up to 50 ms for replicas to populate bt_ready */
  for (int wait_iter = 0; wait_iter < 50; wait_iter++) {
    bool all_ready = true;
    for (uint32_t r = 0; r < N; r++) {
      if (atomic_load_explicit(&g_shm->rings[r].eof, memory_order_acquire) ==
              0 &&
          atomic_load_explicit(&g_shm->rings[r].bt_ready,
                               memory_order_acquire) == 0) {
        all_ready = false;
      }
    }
    if (all_ready)
      break;
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000};
    nanosleep(&ts, NULL);
  }

  char primary_loc[512];
  resolve_site_location(g_shm->executable_path, recs[0].site_id, primary_loc,
                        sizeof(primary_loc));

  fprintf(stderr,
          "\n=================================================================="
          "==============\n");
  fprintf(stderr, " [VERIFICARLO SENTINEL] %s DETECTED\n",
          divergence_reason_name(reason));
  fprintf(stderr,
          "===================================================================="
          "============\n");
  fprintf(stderr,
          "  Dynamic Sync Point : #%" PRIu64 " (0-indexed: %" PRIu64 ")\n",
          seq + 1, seq);
  fprintf(stderr,
          "  Site Occurrence    : #%" PRIu64 " at instruction 0x%" PRIx64 "\n",
          site_occurrence, recs[0].site_id);
  fprintf(stderr, "  Source Location    : %s\n", primary_loc);
  fprintf(stderr, "  Sync Point Type    : %s\n", sync_kind_name(recs[0].kind));

  if (recs[0].kind == VFC_SYNC_FCMP_FLOAT ||
      recs[0].kind == VFC_SYNC_FCMP_DOUBLE ||
      recs[0].kind == VFC_SYNC_FCMP_VEC_FLOAT ||
      recs[0].kind == VFC_SYNC_FCMP_VEC_DOUBLE) {
    fprintf(stderr, "  Comparison Pred    : %s\n",
            fcmp_predicate_name(recs[0].predicate));
  } else if (recs[0].kind == VFC_SYNC_FPTOI_FLOAT ||
             recs[0].kind == VFC_SYNC_FPTOI_DOUBLE) {
    fprintf(stderr, "  Conversion Kind    : %s\n",
            recs[0].predicate ? "fptosi (signed int)"
                              : "fptoui (unsigned int)");
  } else if (recs[0].kind == VFC_SYNC_USER_PROBE &&
             g_shm->rings[0].last_probe_label[0]) {
    fprintf(stderr, "  User Probe Label   : %s\n",
            g_shm->rings[0].last_probe_label);
  }

  fprintf(stderr,
          "  Execution Mode     : N=%u replicas, batch k=%u, buffers B=%u, "
          "policy=%s\n",
          N, g_shm->batch_size_k, g_shm->num_buffers_b,
          g_shm->policy == VFC_POLICY_REFERENCE
              ? "reference"
              : (g_shm->policy == VFC_POLICY_MAJORITY ? "majority" : "stop"));

  if (reason == VFC_DIV_TRACE_SITE_MISMATCH ||
      reason == VFC_DIV_PREMATURE_EOF) {
    fprintf(stderr,
            "\n--- Per-Replica Control-Flow State at Sequence #%" PRIu64
            " ---------------------\n",
            seq + 1);
    for (uint32_t r = 0; r < N; r++) {
      char r_loc[512];
      resolve_site_location(g_shm->executable_path, recs[r].site_id, r_loc,
                            sizeof(r_loc));
      fprintf(stderr,
              "  Replica %u (PID %d, seed=%" PRIu64 "): site=0x%" PRIx64
              " [%s]\n"
              "    -> %s\n",
              r, (int)g_shm->rings[r].pid, g_shm->rings[r].seed,
              recs[r].site_id, sync_kind_name(recs[r].kind), r_loc);
    }
  }

  /* Per-Replica Operand & Outcome Provenance Table */
  fprintf(stderr, "\n--- Per-Replica Numerical Provenance "
                  "-------------------------------------------\n");
  uint32_t true_votes = 0;
  uint32_t false_votes = 0;
  double sum_a = 0.0, sum_b = 0.0, sum_d = 0.0;
  for (uint32_t r = 0; r < N; r++) {
    double a = recs[r].op_a;
    double b = recs[r].op_b;
    double d = a - b;
    sum_a += a;
    sum_b += b;
    sum_d += d;
    if (recs[r].outcome)
      true_votes++;
    else
      false_votes++;

    bool is_f32 = (recs[r].kind == VFC_SYNC_FCMP_FLOAT ||
                   recs[r].kind == VFC_SYNC_FCMP_VEC_FLOAT ||
                   recs[r].kind == VFC_SYNC_FPTOI_FLOAT);
    uint64_t ulp = is_f32 ? ulp_distance_float((float)a, (float)b)
                          : ulp_distance_double(a, b);

    const char *role = (r == 0 && g_shm->reference_ieee) ? " [IEEE REF]" : "";
    if (recs[r].kind == VFC_SYNC_FPTOI_FLOAT ||
        recs[r].kind == VFC_SYNC_FPTOI_DOUBLE) {
      fprintf(stderr,
              "  Replica %u%s (PID %d, seed=%" PRIu64 ") => int outcome = %d\n",
              r, role, (int)g_shm->rings[r].pid, g_shm->rings[r].seed,
              recs[r].outcome);
    } else if (recs[r].vec_size > 1) {
      fprintf(stderr,
              "  Replica %u%s (PID %d, seed=%" PRIu64
              ") => lane_mask = 0x%04x (vec_size=%u)\n",
              r, role, (int)g_shm->rings[r].pid, g_shm->rings[r].seed,
              (unsigned)(recs[r].outcome & 0xFFFF), recs[r].vec_size);
    } else {
      fprintf(stderr,
              "  Replica %u%s (PID %d, seed=%" PRIu64
              ") => outcome = %s (%d)\n",
              r, role, (int)g_shm->rings[r].pid, g_shm->rings[r].seed,
              recs[r].outcome ? "TRUE " : "FALSE", recs[r].outcome);
    }

    if (is_f32) {
      float fa = (float)a, fb = (float)b, fd = fa - fb;
      fprintf(stderr,
              "    a     = %+.9g  | hex: %-16a | bits: 0x%08" PRIx32 "\n", fa,
              (double)fa, float_to_raw_bits(fa));
      if (recs[r].kind != VFC_SYNC_FPTOI_FLOAT &&
          recs[r].kind != VFC_SYNC_USER_PROBE) {
        fprintf(stderr,
                "    b     = %+.9g  | hex: %-16a | bits: 0x%08" PRIx32 "\n", fb,
                (double)fb, float_to_raw_bits(fb));
        fprintf(stderr,
                "    a - b = %+.9g  | hex: %-16a | ULP(a,b) = %" PRIu64 "\n",
                fd, (double)fd, ulp);
      }
    } else {
      fprintf(stderr,
              "    a     = %+.17g | hex: %-22a | bits: 0x%016" PRIx64 "\n", a,
              a, double_to_raw_bits(a));
      if (recs[r].kind != VFC_SYNC_FPTOI_DOUBLE &&
          recs[r].kind != VFC_SYNC_USER_PROBE) {
        fprintf(stderr,
                "    b     = %+.17g | hex: %-22a | bits: 0x%016" PRIx64 "\n", b,
                b, double_to_raw_bits(b));
        fprintf(stderr,
                "    a - b = %+.17g | hex: %-22a | ULP(a,b) = %" PRIu64 "\n", d,
                d, ulp);
      }
    }
  }

  /* Ensemble Uncertainty & CADNA Computational Zero Analysis */
  double mean_a = sum_a / N;
  double mean_b = sum_b / N;
  double mean_d = sum_d / N;
  double var_a = 0.0, var_b = 0.0, var_d = 0.0;
  for (uint32_t r = 0; r < N; r++) {
    double da = recs[r].op_a - mean_a;
    double db = recs[r].op_b - mean_b;
    double dd = (recs[r].op_a - recs[r].op_b) - mean_d;
    var_a += da * da;
    var_b += db * db;
    var_d += dd * dd;
  }
  double std_a = (N > 1) ? sqrt(var_a / (N - 1)) : 0.0;
  double std_b = (N > 1) ? sqrt(var_b / (N - 1)) : 0.0;
  double std_d = (N > 1) ? sqrt(var_d / (N - 1)) : 0.0;

  double t95 = student_t95(N);
  double noise_floor = (N > 1) ? (std_d * t95 / sqrt((double)N)) : 0.0;
  int is_comp_zero = (fabs(mean_d) <= noise_floor) ? 1 : 0;
  double sig_bits = 0.0;
  if (std_d == 0.0 && mean_d != 0.0) {
    sig_bits = (recs[0].kind == VFC_SYNC_FCMP_FLOAT) ? 24.0 : 53.0;
  } else if (std_d > 0.0 && fabs(mean_d) > noise_floor) {
    sig_bits = log2(fabs(mean_d) * sqrt((double)N) / (std_d * t95));
    if (sig_bits < 0.0)
      sig_bits = 0.0;
  }

  fprintf(stderr, "\n--- Ensemble Uncertainty & CADNA Stochastic Analysis "
                  "---------------------------\n");
  fprintf(stderr, "  Vote Split         : %u TRUE vs %u FALSE (total %u)\n",
          true_votes, false_votes, N);
  fprintf(stderr, "  Operand a (mean±σ) : %+.17g ± %.6e\n", mean_a, std_a);
  fprintf(stderr, "  Operand b (mean±σ) : %+.17g ± %.6e\n", mean_b, std_b);
  fprintf(stderr, "  Diff a-b  (mean±σ) : %+.17g ± %.6e\n", mean_d, std_d);
  fprintf(
      stderr,
      "  Significant Bits   : %.2f bits (95%% Student-t confidence, t=%.3f)\n",
      sig_bits, t95);
  fprintf(
      stderr, "  CADNA Diagnosis    : %s\n",
      is_comp_zero
          ? "COMPUTATIONAL ZERO (@.0) — |a - b| is dominated by rounding noise"
          : "NON-ZERO DIFFERENCE — operands separated beyond local noise "
            "floor");

  /* Call Stack & Backtrace Provenance */
  fprintf(stderr, "\n--- Call-Stack & Backtrace Provenance "
                  "------------------------------------------\n");
  if (g_shm->rings[0].call_stack_str[0] != '\0') {
    fprintf(stderr, "  Instrumented Call Stack (--inst-func): %s\n",
            g_shm->rings[0].call_stack_str);
  }
  if (g_shm->rings[0].bt_count > 0) {
    fprintf(stderr, "  Replica 0 Native Backtrace:\n");
    for (uint32_t i = 0; i < g_shm->rings[0].bt_count; i++) {
      char frame_loc[512];
      resolve_site_location(g_shm->executable_path, g_shm->rings[0].bt_addrs[i],
                            frame_loc, sizeof(frame_loc));
      fprintf(stderr, "    #%u %s\n", i, frame_loc);
    }
  } else {
    fprintf(stderr, "    #0 %s\n", primary_loc);
  }

  /* Flight Recorder History of Preceding Validated Sync Points */
  if (history_count > 0) {
    fprintf(stderr,
            "\n--- Flight Recorder (Last %zu Validated Sync Points Before "
            "Divergence) ---------\n",
            history_count);
    size_t start_idx =
        (history_head + VFC_SENTINEL_HISTORY_SIZE - history_count) %
        VFC_SENTINEL_HISTORY_SIZE;
    for (size_t i = 0; i < history_count; i++) {
      const vfc_history_entry_t *he =
          &history[(start_idx + i) % VFC_SENTINEL_HISTORY_SIZE];
      char h_loc[512];
      resolve_site_location(g_shm->executable_path, he->rec.site_id, h_loc,
                            sizeof(h_loc));
      fprintf(stderr,
              "  [#%" PRIu64 "] %s | outcome=%d | a=%+.6g, b=%+.6g\n"
              "       at %s\n",
              he->seq + 1, fcmp_predicate_name(he->rec.predicate),
              he->rec.outcome, he->rec.op_a, he->rec.op_b, h_loc);
    }
  }
  fprintf(stderr,
          "===================================================================="
          "============\n\n");
  fflush(stderr);

  write_json_report(g_shm->json_report_path, reason, seq, site_occurrence, recs,
                    primary_loc, mean_d, std_d, sig_bits, is_comp_zero);
}

/* =========================================================================
 * Coordinator Validator Loop
 * ========================================================================= */

static uint64_t get_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
}

static int coordinator_validator_loop(void) {
  uint32_t N = g_shm->num_replicas;
  uint32_t B = g_shm->num_buffers_b;

  uint64_t cur_batch_idx[VFC_SENTINEL_MAX_REPLICAS] = {0};
  uint32_t cur_rec_idx[VFC_SENTINEL_MAX_REPLICAS] = {0};
  int child_exited[VFC_SENTINEL_MAX_REPLICAS] = {0};
  int child_crash_sig[VFC_SENTINEL_MAX_REPLICAS] = {0};

  vfc_history_entry_t history[VFC_SENTINEL_HISTORY_SIZE];
  memset(history, 0, sizeof(history));
  size_t history_count = 0;
  size_t history_head = 0;

  vfc_hashmap_t site_counts = vfc_hashmap_create();
  uint64_t global_seq = 0;
  uint64_t last_progress_ms = get_time_ms();
  int allow_partial_peek = 0;
  int exit_code = 0;

  while (1) {
    /* Reap any exited children non-blockingly */
    int status = 0;
    pid_t wpid = waitpid(-1, &status, WNOHANG);
    if (wpid > 0) {
      for (uint32_t r = 0; r < N; r++) {
        if (g_shm->rings[r].pid == wpid) {
          child_exited[r] = 1;
          atomic_store_explicit(&g_shm->rings[r].eof, 1, memory_order_release);
          if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            if (sig != SIGUSR1 && sig != SIGTERM && sig != SIGKILL) {
              child_crash_sig[r] = sig;
            }
          }
        }
      }
    }

    /* Check if all N replicas have a record ready at global_seq */
    uint32_t ready_replicas = 0;
    uint32_t eof_empty_replicas = 0;
    vfc_sync_record_t cur_records[VFC_SENTINEL_MAX_REPLICAS];
    memset(cur_records, 0, sizeof(cur_records));

    for (uint32_t r = 0; r < N; r++) {
      vfc_sentinel_replica_ring_t *ring = &g_shm->rings[r];
      uint64_t b_idx = cur_batch_idx[r];
      uint64_t w_idx =
          atomic_load_explicit(&ring->write_batch_idx, memory_order_acquire);
      vfc_sentinel_batch_t *batch = &ring->batches[b_idx % B];

      if (b_idx < w_idx) {
        uint32_t cnt =
            atomic_load_explicit(&batch->count, memory_order_acquire);
        if (cur_rec_idx[r] < cnt) {
          cur_records[r] = batch->records[cur_rec_idx[r]];
          ready_replicas++;
        } else {
          /* Advance to next batch slot */
          atomic_store_explicit(&batch->count, 0, memory_order_relaxed);
          atomic_store_explicit(&batch->published, 0, memory_order_relaxed);
          cur_rec_idx[r] = 0;
          cur_batch_idx[r] = b_idx + 1;
          atomic_store_explicit(&ring->read_batch_idx, b_idx + 1,
                                memory_order_release);
        }
      } else {
        /* Batch b_idx is not yet published; check if partial peek or EOF
         * applies */
        uint32_t partial_cnt =
            atomic_load_explicit(&batch->count, memory_order_acquire);
        int is_eof = atomic_load_explicit(&ring->eof, memory_order_acquire) ||
                     child_exited[r];
        if (allow_partial_peek && cur_rec_idx[r] < partial_cnt) {
          cur_records[r] = batch->records[cur_rec_idx[r]];
          ready_replicas++;
        } else if (is_eof && cur_rec_idx[r] < partial_cnt) {
          cur_records[r] = batch->records[cur_rec_idx[r]];
          ready_replicas++;
        } else if (is_eof && cur_rec_idx[r] >= partial_cnt) {
          eof_empty_replicas++;
        }
      }
    }

    if (ready_replicas == N) {
      /* Advance per-replica record cursors */
      for (uint32_t r = 0; r < N; r++) {
        vfc_sentinel_replica_ring_t *ring = &g_shm->rings[r];
        uint64_t b_idx = cur_batch_idx[r];
        uint64_t w_idx =
            atomic_load_explicit(&ring->write_batch_idx, memory_order_acquire);
        vfc_sentinel_batch_t *batch = &ring->batches[b_idx % B];
        uint32_t cnt =
            atomic_load_explicit(&batch->count, memory_order_acquire);
        cur_rec_idx[r]++;
        if (b_idx < w_idx && cur_rec_idx[r] >= cnt) {
          atomic_store_explicit(&batch->count, 0, memory_order_relaxed);
          atomic_store_explicit(&batch->published, 0, memory_order_relaxed);
          cur_rec_idx[r] = 0;
          cur_batch_idx[r] = b_idx + 1;
          atomic_store_explicit(&ring->read_batch_idx, b_idx + 1,
                                memory_order_release);
        }
      }

      last_progress_ms = get_time_ms();
      allow_partial_peek = 0;

      /* Update occurrence count for primary site_id */
      size_t prev_occ =
          (size_t)vfc_hashmap_get(site_counts, (size_t)cur_records[0].site_id);
      size_t cur_occ = prev_occ + 1;
      vfc_hashmap_insert(site_counts, (size_t)cur_records[0].site_id,
                         (void *)cur_occ);

      /* 1. Verify control-flow trace alignment (site_id, kind, predicate) */
      bool trace_mismatch = false;
      bool outcome_mismatch = false;
      for (uint32_t r = 1; r < N; r++) {
        if (cur_records[r].site_id != cur_records[0].site_id ||
            cur_records[r].kind != cur_records[0].kind ||
            cur_records[r].predicate != cur_records[0].predicate) {
          trace_mismatch = true;
          break;
        }
        if (cur_records[r].outcome != cur_records[0].outcome) {
          outcome_mismatch = true;
        }
      }

      if (trace_mismatch) {
        atomic_fetch_add(&g_shm->disagreement_count, 1);
        atomic_store_explicit(&g_shm->diverged_flag, 1, memory_order_release);
        report_provenance_diagnostics(VFC_DIV_TRACE_SITE_MISMATCH, global_seq,
                                      cur_occ, cur_records, history,
                                      history_count, history_head);
        exit_code = 1;
        break;
      }

      if (outcome_mismatch) {
        atomic_fetch_add(&g_shm->disagreement_count, 1);
        if (g_shm->policy == VFC_POLICY_STOP) {
          atomic_store_explicit(&g_shm->diverged_flag, 1, memory_order_release);
          vfc_divergence_reason_t reason = VFC_DIV_NUMERICAL_BRANCH;
          if (cur_records[0].kind == VFC_SYNC_FPTOI_FLOAT ||
              cur_records[0].kind == VFC_SYNC_FPTOI_DOUBLE) {
            reason = VFC_DIV_FPTOI_MISMATCH;
          } else if (cur_records[0].kind == VFC_SYNC_USER_PROBE) {
            reason = VFC_DIV_USER_PROBE_MISMATCH;
          }
          report_provenance_diagnostics(reason, global_seq, cur_occ,
                                        cur_records, history, history_count,
                                        history_head);
          exit_code = 1;
          break;
        } else {
          /* Lockstep consensus policy (reference or majority) */
          int32_t consensus = cur_records[0].outcome;
          if (g_shm->policy == VFC_POLICY_MAJORITY) {
            uint32_t ones = 0;
            for (uint32_t r = 0; r < N; r++) {
              if (cur_records[r].outcome)
                ones++;
            }
            consensus = (ones * 2 >= N) ? 1 : 0;
          }
          atomic_store_explicit(&g_shm->lockstep_outcome, consensus,
                                memory_order_release);
        }
      } else if (g_shm->batch_size_k == 1) {
        atomic_store_explicit(&g_shm->lockstep_outcome, cur_records[0].outcome,
                              memory_order_release);
      }

      /* Release lockstep barrier for k == 1 */
      if (g_shm->batch_size_k == 1) {
        atomic_store_explicit(&g_shm->lockstep_seq, global_seq + 1,
                              memory_order_release);
      }

      /* Record validated sync point in flight recorder */
      history[history_head].seq = global_seq;
      history[history_head].rec = cur_records[0];
      history_head = (history_head + 1) % VFC_SENTINEL_HISTORY_SIZE;
      if (history_count < VFC_SENTINEL_HISTORY_SIZE)
        history_count++;

      atomic_fetch_add(&g_shm->validated_events, 1);
      global_seq++;
      continue;
    }

    /* Check if all replicas have cleanly reached EOF with no pending events */
    if (eof_empty_replicas == N) {
      /* Check if any replica crashed with a fatal signal */
      int crashed_r = -1;
      for (uint32_t r = 0; r < N; r++) {
        if (child_crash_sig[r] != 0) {
          crashed_r = (int)r;
          break;
        }
      }
      if (crashed_r >= 0) {
        fprintf(stderr,
                "[VERIFICARLO SENTINEL] Replica %d (PID %d) terminated by "
                "signal %d after %" PRIu64 " validated sync points.\n",
                crashed_r, (int)g_shm->rings[crashed_r].pid,
                child_crash_sig[crashed_r], global_seq);
        exit_code = 1;
      } else if (g_shm->json_report_path[0]) {
        write_json_report(g_shm->json_report_path, VFC_DIV_NONE, global_seq, 0,
                          NULL, NULL, 0.0, 0.0, 0.0, 0);
      }
      break;
    }

    /* Check if some replica reached EOF with 0 remaining records while another
     * replica still has a ready record => premature termination mismatch! */
    if (eof_empty_replicas > 0 && ready_replicas > 0 &&
        (eof_empty_replicas + ready_replicas == N)) {
      atomic_fetch_add(&g_shm->disagreement_count, 1);
      atomic_store_explicit(&g_shm->diverged_flag, 1, memory_order_release);
      report_provenance_diagnostics(VFC_DIV_PREMATURE_EOF, global_seq, 1,
                                    cur_records, history, history_count,
                                    history_head);
      exit_code = 1;
      break;
    }

    /* If no full batch has arrived for >= 15 ms, enable partial-buffer peek
     * so replicas blocked in MPI/I/O after a divergent comparison are caught */
    uint64_t now_ms = get_time_ms();
    if (now_ms - last_progress_ms >= 15) {
      allow_partial_peek = 1;
    }

    struct timespec ts = {.tv_sec = 0, .tv_nsec = 50000}; /* 50 us */
    nanosleep(&ts, NULL);
  }

  /* Terminate and reap any remaining replica processes */
  atomic_store_explicit(&g_shm->stop_flag, 1, memory_order_release);
  for (uint32_t r = 0; r < N; r++) {
    if (g_shm->rings[r].pid > 0 && !child_exited[r]) {
      if (exit_code != 0) {
        kill(g_shm->rings[r].pid, SIGKILL);
      }
      int st = 0;
      waitpid(g_shm->rings[r].pid, &st, 0);
    }
  }

  vfc_hashmap_destroy(site_counts);
  return exit_code;
}

/* =========================================================================
 * Initialization, Option Parsing, and Process Spawning
 * ========================================================================= */

static void parse_sentinel_env(uint32_t *num_replicas, uint32_t *batch_k,
                               uint32_t *num_buffers, uint32_t *policy,
                               uint32_t *ref_ieee, uint32_t *sync_func,
                               uint32_t *redirect_out, uint64_t *base_seed,
                               int *has_seed, char *report_path,
                               size_t report_size) {
  *num_replicas = 3;
  *batch_k = 256;
  *num_buffers = 4;
  *policy = VFC_POLICY_STOP;
  *ref_ieee = 0;
  *sync_func = 0;
  *redirect_out = 1;
  *base_seed = 0;
  *has_seed = 0;
  report_path[0] = '\0';

  const char *env = getenv("VFC_SENTINEL");
  if (env && env[0] != '\0') {
    char *copy = strdup(env);
    char *saveptr = NULL;
    char *tok = strtok_r(copy, ",;", &saveptr);
    while (tok) {
      while (*tok == ' ')
        tok++;
      char *eq = strchr(tok, '=');
      if (eq) {
        *eq = '\0';
        const char *k = tok;
        const char *v = eq + 1;
        if (strcasecmp(k, "replicas") == 0 || strcasecmp(k, "n") == 0) {
          *num_replicas = (uint32_t)strtoul(v, NULL, 10);
        } else if (strcasecmp(k, "k") == 0 || strcasecmp(k, "batch") == 0) {
          *batch_k = (uint32_t)strtoul(v, NULL, 10);
        } else if (strcasecmp(k, "buffers") == 0 || strcasecmp(k, "b") == 0) {
          *num_buffers = (uint32_t)strtoul(v, NULL, 10);
        } else if (strcasecmp(k, "policy") == 0) {
          if (strcasecmp(v, "reference") == 0 || strcasecmp(v, "ref") == 0)
            *policy = VFC_POLICY_REFERENCE;
          else if (strcasecmp(v, "majority") == 0 ||
                   strcasecmp(v, "consensus") == 0)
            *policy = VFC_POLICY_MAJORITY;
          else
            *policy = VFC_POLICY_STOP;
        } else if (strcasecmp(k, "reference_ieee") == 0 ||
                   strcasecmp(k, "ref_ieee") == 0) {
          *ref_ieee = (strcasecmp(v, "1") == 0 || strcasecmp(v, "true") == 0);
        } else if (strcasecmp(k, "sync_func") == 0) {
          *sync_func = (strcasecmp(v, "1") == 0 || strcasecmp(v, "true") == 0);
        } else if (strcasecmp(k, "redirect_output") == 0) {
          *redirect_out =
              (strcasecmp(v, "1") == 0 || strcasecmp(v, "true") == 0);
        } else if (strcasecmp(k, "seed") == 0) {
          *base_seed = (uint64_t)strtoull(v, NULL, 10);
          *has_seed = 1;
        } else if (strcasecmp(k, "report") == 0 || strcasecmp(k, "json") == 0) {
          strncpy(report_path, v, report_size - 1);
          report_path[report_size - 1] = '\0';
        }
      } else if (strcasecmp(tok, "reference_ieee") == 0 ||
                 strcasecmp(tok, "ref_ieee") == 0) {
        *ref_ieee = 1;
      } else if (strcasecmp(tok, "sync_func") == 0) {
        *sync_func = 1;
      }
      tok = strtok_r(NULL, ",;", &saveptr);
    }
    free(copy);
  }

  /* Allow individual VFC_SENTINEL_* environment variables to override */
  if (getenv("VFC_SENTINEL_REPLICAS"))
    *num_replicas =
        (uint32_t)strtoul(getenv("VFC_SENTINEL_REPLICAS"), NULL, 10);
  if (getenv("VFC_SENTINEL_K"))
    *batch_k = (uint32_t)strtoul(getenv("VFC_SENTINEL_K"), NULL, 10);
  if (getenv("VFC_SENTINEL_BUFFERS"))
    *num_buffers = (uint32_t)strtoul(getenv("VFC_SENTINEL_BUFFERS"), NULL, 10);
  if (getenv("VFC_SENTINEL_REPORT")) {
    strncpy(report_path, getenv("VFC_SENTINEL_REPORT"), report_size - 1);
    report_path[report_size - 1] = '\0';
  }

  if (*num_replicas < 2)
    *num_replicas = 2;
  if (*num_replicas > VFC_SENTINEL_MAX_REPLICAS)
    *num_replicas = VFC_SENTINEL_MAX_REPLICAS;
  if (*batch_k < 1)
    *batch_k = 1;
  if (*batch_k > VFC_SENTINEL_MAX_K)
    *batch_k = VFC_SENTINEL_MAX_K;
  if (*num_buffers < 2)
    *num_buffers = 2;
  if (*num_buffers > VFC_SENTINEL_MAX_BUFFERS)
    *num_buffers = VFC_SENTINEL_MAX_BUFFERS;

  /* Reference and majority branch policies require lockstep synchronization */
  if (*policy == VFC_POLICY_REFERENCE || *policy == VFC_POLICY_MAJORITY) {
    *batch_k = 1;
  }
}

static void configure_replica_backends_seed(uint32_t replica_id,
                                            uint64_t seed) {
  const char *orig = getenv("VFC_BACKENDS");
  if (!orig || orig[0] == '\0')
    return;

  /* Only inject --seed for stochastic backends that accept --seed */
  if (strstr(orig, "libinterflop_mca") == NULL &&
      strstr(orig, "libinterflop_bitmask") == NULL &&
      strstr(orig, "libinterflop_prism") == NULL) {
    return;
  }
  if (strstr(orig, "--seed") != NULL) {
    return;
  }

  char new_backends[2048];
  snprintf(new_backends, sizeof(new_backends), "%s --seed=%" PRIu64, orig,
           seed);
  setenv("VFC_BACKENDS", new_backends, 1);
  (void)replica_id;
}

void vfc_sentinel_pre_init(const vfc_options_t *options) {
  (void)options;
  const char *sentinel_env = getenv("VFC_SENTINEL");
  const char *replicas_env = getenv("VFC_SENTINEL_REPLICAS");
  if ((!sentinel_env || sentinel_env[0] == '\0' ||
       strcmp(sentinel_env, "0") == 0 ||
       strcasecmp(sentinel_env, "false") == 0) &&
      (!replicas_env || replicas_env[0] == '\0')) {
    vfc_sentinel_enabled = 0;
    return;
  }

  /* Avoid recursive spawning if child processes load dynamic libraries */
  if (getenv("VFC_SENTINEL_ACTIVE_REPLICA") != NULL) {
    return;
  }

  uint32_t num_replicas, batch_k, num_buffers, policy, ref_ieee, sync_func,
      redirect_out;
  uint64_t base_seed;
  int has_seed;
  char report_path[512];
  parse_sentinel_env(&num_replicas, &batch_k, &num_buffers, &policy, &ref_ieee,
                     &sync_func, &redirect_out, &base_seed, &has_seed,
                     report_path, sizeof(report_path));

  if (!has_seed) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    base_seed = ((uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec) ^
                ((uint64_t)getpid() << 16);
  }

  g_shm = (vfc_sentinel_shm_t *)mmap(NULL, sizeof(vfc_sentinel_shm_t),
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (g_shm == MAP_FAILED) {
    logger_error("Sentinel failed to allocate shared memory: %s",
                 strerror(errno));
  }
  memset(g_shm, 0, sizeof(vfc_sentinel_shm_t));

  g_shm->magic = VFC_SENTINEL_MAGIC;
  g_shm->num_replicas = num_replicas;
  g_shm->batch_size_k = batch_k;
  g_shm->num_buffers_b = num_buffers;
  g_shm->policy = policy;
  g_shm->reference_ieee = ref_ieee;
  g_shm->sync_func = sync_func;
  g_shm->redirect_output = redirect_out;
  strncpy(g_shm->json_report_path, report_path,
          sizeof(g_shm->json_report_path) - 1);

  ssize_t exe_len = readlink("/proc/self/exe", g_shm->executable_path,
                             sizeof(g_shm->executable_path) - 1);
  if (exe_len > 0) {
    g_shm->executable_path[exe_len] = '\0';
  } else {
    strcpy(g_shm->executable_path, "/proc/self/exe");
  }

  fflush(stdout);
  fflush(stderr);

  /* Spawn N replica processes */
  for (uint32_t r = 0; r < num_replicas; r++) {
    uint64_t r_seed = base_seed + (uint64_t)r + 1ULL;
    g_shm->rings[r].seed = r_seed;

    pid_t pid = fork();
    if (pid < 0) {
      logger_error("Sentinel failed to fork replica %u: %s", r,
                   strerror(errno));
    }
    if (pid == 0) {
      /* Child Replica r */
      g_is_coordinator = 0;
      g_replica_id = (int)r;
      g_ring = &g_shm->rings[r];
      g_ring->pid = getpid();
      vfc_sentinel_enabled = 1;
      vfc_sentinel_sync_func = (unsigned char)sync_func;

      char id_buf[16];
      snprintf(id_buf, sizeof(id_buf), "%u", r);
      setenv("VFC_SENTINEL_ACTIVE_REPLICA", id_buf, 1);
      setenv("VFC_BACKENDS_SILENT_LOAD", "True", 1);

      struct sigaction sa;
      memset(&sa, 0, sizeof(sa));
      sa.sa_handler = sentinel_sigusr1_handler;
      sa.sa_flags = SA_RESTART;
      sigaction(SIGUSR1, &sa, NULL);

      if (r > 0 && redirect_out) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
          dup2(devnull, STDOUT_FILENO);
          close(devnull);
        }
      }

      if (r == 0 && ref_ieee) {
        setenv("VFC_BACKENDS", "libinterflop_ieee.so", 1);
        g_ring->seed = 0;
      } else {
        configure_replica_backends_seed(r, r_seed);
      }

      /* Return into vfc_init() so this replica loads its backends and runs
       * the instrumented application */
      return;
    } else {
      g_shm->rings[r].pid = pid;
    }
  }

  /* Parent Coordinator Process: run the validator loop and exit */
  g_is_coordinator = 1;
  int code = coordinator_validator_loop();
  munmap(g_shm, sizeof(vfc_sentinel_shm_t));
  _exit(code);
}

void vfc_sentinel_post_init(void) {}

void vfc_sentinel_atexit(void) {
  if (!vfc_sentinel_enabled || !g_ring || g_is_coordinator || g_atexit_done)
    return;
  g_atexit_done = 1;
  sentinel_publish_current_batch();
  atomic_store_explicit(&g_ring->eof, 1, memory_order_release);
}
