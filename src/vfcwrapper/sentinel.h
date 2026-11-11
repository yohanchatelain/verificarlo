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

#ifndef __VFC_SENTINEL_H__
#define __VFC_SENTINEL_H__

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "interflop/interflop.h"
#include "vfcwrapper.h"

#define VFC_SENTINEL_MAGIC 0x56464353454E5431ULL /* "VFCSENT1" */
#define VFC_SENTINEL_MAX_REPLICAS 16
#define VFC_SENTINEL_MAX_BUFFERS 8
#define VFC_SENTINEL_MAX_K 4096
#define VFC_SENTINEL_HISTORY_SIZE 16
#define VFC_SENTINEL_MAX_BT_FRAMES 16

/* Types of synchronization points tracked by Sentinel */
typedef enum {
  VFC_SYNC_NONE = 0,
  VFC_SYNC_FCMP_FLOAT = 1,
  VFC_SYNC_FCMP_DOUBLE = 2,
  VFC_SYNC_FCMP_VEC_FLOAT = 3,
  VFC_SYNC_FCMP_VEC_DOUBLE = 4,
  VFC_SYNC_FPTOI_FLOAT = 5,
  VFC_SYNC_FPTOI_DOUBLE = 6,
  VFC_SYNC_FUNC_ENTER = 7,
  VFC_SYNC_FUNC_EXIT = 8,
  VFC_SYNC_USER_PROBE = 9
} vfc_sync_kind_t;

/* Policy when a comparison disagreement occurs */
typedef enum {
  VFC_POLICY_STOP = 0,      /* Stop at first disagreement (any k >= 1) */
  VFC_POLICY_REFERENCE = 1, /* Force Replica 0's decision (requires k == 1) */
  VFC_POLICY_MAJORITY = 2   /* Force majority vote decision (requires k == 1) */
} vfc_sentinel_policy_t;

/* Classification of detected divergence */
typedef enum {
  VFC_DIV_NONE = 0,
  VFC_DIV_NUMERICAL_BRANCH = 1,
  VFC_DIV_FPTOI_MISMATCH = 2,
  VFC_DIV_USER_PROBE_MISMATCH = 3,
  VFC_DIV_TRACE_SITE_MISMATCH = 4,
  VFC_DIV_PREMATURE_EOF = 5,
  VFC_DIV_REPLICA_CRASH = 6
} vfc_divergence_reason_t;

/* 32-byte aligned sync-point trace record (2 records per 64B cache line) */
typedef struct __attribute__((aligned(32))) {
  uint64_t site_id;   /* Instruction return address - CALL_OP_SIZE or hash */
  double op_a;        /* First operand (promoted to double) */
  double op_b;        /* Second operand (0.0 for unary ops) */
  int32_t outcome;    /* Boolean (0/1), lane bitmask for vec, or int32 cast */
  uint16_t predicate; /* FCMP_PREDICATE or signedness flag */
  uint8_t kind;       /* vfc_sync_kind_t */
  uint8_t vec_size;   /* 1 for scalar, 2/4/8/16 for vector */
} vfc_sync_record_t;

/* Single batch buffer holding up to k records */
typedef struct {
  uint64_t seq_start;
  _Atomic uint32_t count;
  _Atomic uint32_t published;
  vfc_sync_record_t records[VFC_SENTINEL_MAX_K];
} vfc_sentinel_batch_t;

/* Per-replica SPSC ring buffer and diagnostic state */
typedef struct __attribute__((aligned(64))) {
  _Atomic uint64_t write_batch_idx;
  _Atomic uint64_t read_batch_idx;
  _Atomic int eof;
  _Atomic int bt_ready;
  pid_t pid;
  uint64_t seed;
  uint32_t bt_count;
  uint64_t bt_addrs[VFC_SENTINEL_MAX_BT_FRAMES];
  char call_stack_str[1024];
  char last_probe_label[128];
  vfc_sentinel_batch_t batches[VFC_SENTINEL_MAX_BUFFERS];
} vfc_sentinel_replica_ring_t;

/* Shared-memory control block across coordinator and all N replicas */
typedef struct {
  uint64_t magic;
  uint32_t num_replicas;
  uint32_t batch_size_k;
  uint32_t num_buffers_b;
  uint32_t policy;
  uint32_t reference_ieee;
  uint32_t sync_func;
  uint32_t redirect_output;
  _Atomic int stop_flag;
  _Atomic int diverged_flag;
  /* Lockstep (k == 1) consensus synchronization channel */
  _Atomic uint64_t lockstep_seq;
  _Atomic int32_t lockstep_outcome;
  /* Summary counters */
  _Atomic uint64_t validated_events;
  _Atomic uint64_t disagreement_count;
  char executable_path[512];
  char json_report_path[512];
  vfc_sentinel_replica_ring_t rings[VFC_SENTINEL_MAX_REPLICAS];
} vfc_sentinel_shm_t;

extern unsigned char vfc_sentinel_enabled;
extern unsigned char vfc_sentinel_sync_func;

/* Lifecycle hooks called from main.c */
void vfc_sentinel_pre_init(const vfc_options_t *options);
void vfc_sentinel_post_init(void);
void vfc_sentinel_atexit(void);

/* Standard IEEE-754 floating-point predicate evaluation helpers */
int vfc_eval_fcmp_float(enum FCMP_PREDICATE p, float a, float b);
int vfc_eval_fcmp_double(enum FCMP_PREDICATE p, double a, double b);

/* Sync-point recording hooks */
int vfc_sentinel_record_cmp_float(uint64_t site_id, enum FCMP_PREDICATE p,
                                  float a, float b, int outcome);
int vfc_sentinel_record_cmp_double(uint64_t site_id, enum FCMP_PREDICATE p,
                                   double a, double b, int outcome);
void vfc_sentinel_record_vec_cmp_float(uint64_t site_id, enum FCMP_PREDICATE p,
                                       const float *a, const float *b,
                                       int *outcomes, int size);
void vfc_sentinel_record_vec_cmp_double(uint64_t site_id, enum FCMP_PREDICATE p,
                                        const double *a, const double *b,
                                        int *outcomes, int size);
int64_t vfc_sentinel_record_fptoi_float(uint64_t site_id, int is_signed,
                                        float a, int64_t result);
int64_t vfc_sentinel_record_fptoi_double(uint64_t site_id, int is_signed,
                                         double a, int64_t result);
void vfc_sentinel_record_func(uint64_t site_id, const char *func_name,
                              int is_enter);
void vfc_sentinel_record_user_probe(uint64_t site_id, const char *label,
                                    double value);
void vfc_sentinel_flush(void);

/* Public C / Fortran user probes */
void vfc_sync_point(const char *label, double val);
void vfc_sync_flush(void);

#endif /* __VFC_SENTINEL_H__ */
