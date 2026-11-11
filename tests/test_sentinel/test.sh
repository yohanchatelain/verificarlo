#!/bin/bash

set -e

echo "=== [TEST 1] Unstable branch detection across k=1, k=16, k=256 ==="
verificarlo-c --sentinel -O0 unst_branch.c -o unst_branch

for k in 1 16 256; do
    rm -f report_k${k}.json
    set +e
    vfc_sentinel -n 4 -k ${k} --seed=0 \
        --backends="libinterflop_mca.so --precision-binary64=53 --mode=mca" \
        --json=report_k${k}.json ./unst_branch > out_k${k}.log 2> err_k${k}.log
    rc=$?
    set -e

    if [ $rc -eq 0 ]; then
        echo "[FAIL] Expected non-zero exit code on unstable branch for k=${k}"
        cat err_k${k}.log
        exit 1
    fi

    if ! grep -q "NUMERICAL_BRANCH_DIVERGENCE DETECTED" err_k${k}.log; then
        echo "[FAIL] Missing NUMERICAL_BRANCH_DIVERGENCE DETECTED diagnostic header for k=${k}"
        cat err_k${k}.log
        exit 1
    fi

    if ! grep -q "unst_branch.c" err_k${k}.log; then
        echo "[FAIL] Missing source file location unst_branch.c in diagnostics for k=${k}"
        cat err_k${k}.log
        exit 1
    fi

    if ! grep -q "FCMP_OGT" err_k${k}.log; then
        echo "[FAIL] Missing predicate FCMP_OGT in diagnostics for k=${k}"
        cat err_k${k}.log
        exit 1
    fi

    if ! grep -q "CADNA Diagnosis" err_k${k}.log; then
        echo "[FAIL] Missing CADNA computational zero check in diagnostics for k=${k}"
        cat err_k${k}.log
        exit 1
    fi

    if ! grep -q '"divergence_sequence": 2' report_k${k}.json; then
        echo "[FAIL] Expected divergence_sequence == 2 in report_k${k}.json"
        cat report_k${k}.json
        exit 1
    fi
    echo "[PASS] Unstable branch accurately detected at sequence #2 for k=${k}"
done

echo "=== [TEST 2] Stable loop across >10,000 comparisons and ring buffer wrap-arounds ==="
verificarlo-c --sentinel -O2 stable_loop.c -o stable_loop
rm -f report_stable.json
vfc_sentinel -n 3 -k 64 -b 4 --seed=123 \
    --backends="libinterflop_mca.so --precision-binary64=53 --mode=mca" \
    --json=report_stable.json ./stable_loop > out_stable.log 2> err_stable.log

if ! grep -q '"status": "OK"' report_stable.json; then
    echo "[FAIL] Expected status OK in report_stable.json"
    cat report_stable.json
    exit 1
fi
if ! grep -q '"validated_sync_points": 10000' report_stable.json; then
    echo "[FAIL] Expected 10000 validated sync points in report_stable.json"
    cat report_stable.json
    exit 1
fi
echo "[PASS] 10,000 stable comparisons validated across ring wrap-arounds"

echo "=== [TEST 3] Reference & Majority branch policies (k=1 lockstep) ==="
rm -f report_ref.json report_maj.json
vfc_sentinel -n 4 -k 1 --policy=reference --reference-ieee --seed=0 \
    --backends="libinterflop_mca.so --precision-binary64=53 --mode=mca" \
    --json=report_ref.json ./unst_branch > out_ref.log 2> err_ref.log

if ! grep -q '"status": "OK"' report_ref.json; then
    echo "[FAIL] Expected reference policy run to complete with status OK"
    cat report_ref.json
    exit 1
fi
if grep -q '"disagreement_count": 0' report_ref.json; then
    echo "[FAIL] Expected disagreement_count > 0 in reference policy run"
    cat report_ref.json
    exit 1
fi
echo "[PASS] Reference IEEE policy forced consensus and recorded disagreements"

vfc_sentinel -n 3 -k 1 --policy=majority --seed=0 \
    --backends="libinterflop_mca.so --precision-binary64=53 --mode=mca" \
    --json=report_maj.json ./unst_branch > out_maj.log 2> err_maj.log

if ! grep -q '"status": "OK"' report_maj.json; then
    echo "[FAIL] Expected majority policy run to complete with status OK"
    cat report_maj.json
    exit 1
fi
echo "[PASS] Majority consensus policy completed cleanly"

echo "=== [TEST 4] Float-to-int (fptosi) conversion divergence detection ==="
verificarlo-c --sentinel -O0 fptoi_div.c -o fptoi_div
rm -f report_fptoi.json
set +e
vfc_sentinel -n 4 -k 16 --seed=0 \
    --backends="libinterflop_mca.so --precision-binary64=53 --mode=mca" \
    --json=report_fptoi.json ./fptoi_div > out_fptoi.log 2> err_fptoi.log
rc=$?
set -e

if [ $rc -eq 0 ]; then
    echo "[FAIL] Expected non-zero exit code on fptosi divergence"
    cat err_fptoi.log
    exit 1
fi
if ! grep -q "FPTOI_DOUBLE" err_fptoi.log; then
    echo "[FAIL] Expected FPTOI_DOUBLE operation kind in err_fptoi.log"
    cat err_fptoi.log
    exit 1
fi
echo "[PASS] fptosi float-to-integer divergence detected"

echo "=== [TEST 5] Control-flow trace site mismatch & user sync probes ==="
verificarlo-c --sentinel -O0 site_mismatch.c -o site_mismatch
rm -f report_site.json
set +e
vfc_sentinel -n 3 -k 16 --seed=42 \
    --backends="libinterflop_ieee.so" \
    --json=report_site.json ./site_mismatch > out_site.log 2> err_site.log
rc=$?
set -e

if [ $rc -eq 0 ]; then
    echo "[FAIL] Expected non-zero exit code on site mismatch"
    cat err_site.log
    exit 1
fi
if ! grep -q '"status": "CONTROL_FLOW_TRACE_MISMATCH"' report_site.json; then
    echo "[FAIL] Expected status CONTROL_FLOW_TRACE_MISMATCH in report_site.json"
    cat report_site.json
    exit 1
fi
echo "[PASS] Control-flow trace site mismatch accurately detected"

verificarlo-c --sentinel -O0 user_probe.c -o user_probe
rm -f report_probe.json
set +e
vfc_sentinel -n 4 -k 16 --seed=0 \
    --backends="libinterflop_mca.so --precision-binary64=53 --mode=mca" \
    --json=report_probe.json ./user_probe > out_probe.log 2> err_probe.log
rc=$?
set -e

if [ $rc -eq 0 ]; then
    echo "[FAIL] Expected non-zero exit code on divergent user sync probe"
    cat err_probe.log
    exit 1
fi
if ! grep -q "mpi_pre_allreduce_residual" err_probe.log; then
    echo "[FAIL] Expected user probe label 'mpi_pre_allreduce_residual' in err_probe.log"
    cat err_probe.log
    exit 1
fi
echo "[PASS] Explicit vfc_sync_point / vfc_sync_flush probe verified"
