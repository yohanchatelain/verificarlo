#!/usr/bin/env python3
# /*****************************************************************************\
#  *                                                                           *\
#  *  This file is part of the Verificarlo project,                            *\
#  *  under the Apache License v2.0 with LLVM Exceptions.                      *\
#  *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
#  *  See https://llvm.org/LICENSE.txt for license information.                *\
#  *                                                                           *\
#  *  Copyright (c) 2026                                                       *\
#  *     Verificarlo Contributors                                              *\
#  *                                                                           *\
#  ****************************************************************************/

import argparse
import ctypes
import json
import os
import subprocess
import sys
import tempfile
import time

ADDR_NO_RANDOMIZE = 0x0040000


def disable_aslr():
    """Disable ASLR for child processes on Linux so PIE/shared library addresses match."""
    if sys.platform.startswith("linux"):
        try:
            libc = ctypes.CDLL("libc.so.6", use_errno=True)
            current = libc.personality(0xFFFFFFFF)
            if current != -1:
                libc.personality(current | ADDR_NO_RANDOMIZE)
        except Exception:
            pass


def build_sentinel_env_str(
    replicas,
    batch_size,
    buffers,
    policy,
    reference_ieee,
    sync_func,
    redirect_output,
    seed,
    json_report,
):
    parts = [
        f"replicas={replicas}",
        f"k={batch_size}",
        f"buffers={buffers}",
        f"policy={policy}",
        f"reference_ieee={1 if reference_ieee else 0}",
        f"sync_func={1 if sync_func else 0}",
        f"redirect_output={1 if redirect_output else 0}",
    ]
    if seed is not None:
        parts.append(f"seed={seed}")
    if json_report:
        parts.append(f"report={json_report}")
    return ",".join(parts)


def run_benchmark_k(args, cmd):
    k_values = [int(x.strip()) for x in args.benchmark_k.split(",") if x.strip()]
    print(
        f"[vfc_sentinel] Benchmarking k={k_values} with N={args.replicas} replicas, B={args.buffers} buffers"
    )
    print(
        f"{'k':>8} | {'Time (s)':>10} | {'Validated Syncs':>16} | {'Status':>12}"
    )
    print("-" * 56)

    for k in k_values:
        with tempfile.NamedTemporaryFile(suffix=".json", delete=False) as tmp:
            report_path = tmp.name
        env = os.environ.copy()
        if args.backends:
            env["VFC_BACKENDS"] = args.backends
        elif "VFC_BACKENDS" not in env:
            env["VFC_BACKENDS"] = "libinterflop_mca.so"

        env["VFC_SENTINEL"] = build_sentinel_env_str(
            replicas=args.replicas,
            batch_size=k,
            buffers=args.buffers,
            policy=args.policy,
            reference_ieee=args.reference_ieee,
            sync_func=args.sync_func,
            redirect_output=not args.keep_replica_output,
            seed=args.seed,
            json_report=report_path,
        )

        t0 = time.perf_counter()
        proc = subprocess.run(
            cmd,
            env=env,
            stdout=subprocess.DEVNULL if not args.keep_replica_output else None,
            stderr=subprocess.PIPE,
            text=True,
        )
        elapsed = time.perf_counter() - t0

        validated = "-"
        status = "OK" if proc.returncode == 0 else f"DIV({proc.returncode})"
        try:
            if os.path.exists(report_path) and os.path.getsize(report_path) > 0:
                with open(report_path, "r", encoding="utf-8") as f:
                    data = json.load(f)
                    validated = str(data.get("validated_sync_points", "-"))
                    status = data.get("status", status)
        except Exception:
            pass
        finally:
            if os.path.exists(report_path):
                os.unlink(report_path)

        print(f"{k:>8} | {elapsed:>10.4f} | {validated:>16} | {status:>12}")
    return 0


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        prog="vfc_sentinel",
        description=(
            "Verificarlo Sentinel: Semi-synchronous replica execution and "
            "floating-point branch divergence detector."
        ),
    )
    parser.add_argument(
        "-n",
        "--replicas",
        type=int,
        default=3,
        help="Number of concurrent replica processes N (default: 3).",
    )
    parser.add_argument(
        "-k",
        "--batch-size",
        type=int,
        default=256,
        help="Comparisons per published ring buffer batch k (default: 256; use 1 for lockstep).",
    )
    parser.add_argument(
        "-b",
        "--buffers",
        type=int,
        default=4,
        help="Number of ring buffers B per replica (default: 4).",
    )
    parser.add_argument(
        "--policy",
        choices=["stop", "reference", "majority"],
        default="stop",
        help=(
            "Branch policy on disagreement: 'stop' (report and stop at first divergence), "
            "'reference' (force replica 0's branch decision, implies k=1), or "
            "'majority' (force majority consensus branch decision, implies k=1)."
        ),
    )
    parser.add_argument(
        "--reference-ieee",
        action="store_true",
        help="Configure Replica 0 to use unperturbed IEEE-754 arithmetic (libinterflop_ieee.so).",
    )
    parser.add_argument(
        "--sync-func",
        action="store_true",
        help="Also synchronize function entry/exit events across replicas.",
    )
    parser.add_argument(
        "--backends",
        type=str,
        default=None,
        help="VFC_BACKENDS configuration string (default: $VFC_BACKENDS or 'libinterflop_mca.so').",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Base RNG seed for deterministic per-replica seeds.",
    )
    parser.add_argument(
        "--json",
        type=str,
        default=None,
        help="Write structured JSON divergence/validation report to the given file path.",
    )
    parser.add_argument(
        "--keep-replica-output",
        action="store_true",
        help="Do not suppress stdout on non-zero replicas (1..N-1).",
    )
    parser.add_argument(
        "--benchmark-k",
        type=str,
        default=None,
        help="Comma-separated list of batch sizes k to benchmark (e.g. '1,16,64,256,1024,4096').",
    )
    parser.add_argument(
        "command",
        nargs=argparse.REMAINDER,
        help="Target Verificarlo-instrumented executable and its arguments.",
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    cmd = list(args.command)
    if cmd and cmd[0] == "--":
        cmd = cmd[1:]
    if not cmd:
        print("vfc_sentinel: error: no target executable specified.", file=sys.stderr)
        return 2

    disable_aslr()

    if args.benchmark_k:
        return run_benchmark_k(args, cmd)

    env = os.environ.copy()
    if args.backends:
        env["VFC_BACKENDS"] = args.backends
    elif "VFC_BACKENDS" not in env:
        env["VFC_BACKENDS"] = "libinterflop_mca.so"

    env["VFC_SENTINEL"] = build_sentinel_env_str(
        replicas=args.replicas,
        batch_size=args.batch_size,
        buffers=args.buffers,
        policy=args.policy,
        reference_ieee=args.reference_ieee,
        sync_func=args.sync_func,
        redirect_output=not args.keep_replica_output,
        seed=args.seed,
        json_report=args.json,
    )

    proc = subprocess.run(cmd, env=env)
    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
