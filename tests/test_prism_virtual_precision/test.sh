#!/bin/bash
# Tests that PRISM SR virtual precision produces results at the correct bit
# position for +, *, fma, and / on scalar float, scalar double, float4,
# and double2.
#
# For each (type, op, precision) triple, verifies:
#   BRACKETS:   exactly 2 distinct SR values, differing by 1 ULP at precision p.
#   STOCHASTIC: both brackets appear in N=300 samples.
#   UNBIASED:   sample mean converges to the exact result within 5σ.
#   INDEPENDENCE (vector only): SIMD lanes round independently.
#
# Operands for each operation:
#   div  : 1.0  /  3.0  → 1/3        (never exactly representable)
#   add  : 0.1  +  0.2  → 0.3        (not exactly representable in float/double)
#   mul  : 1.1  ×  1.3  → 1.43       (not exactly representable)
#   fma  : 1.1  ×  1.3  + 0.1 = 1.53 (not exactly representable)

set -e

source "$(dirname "$0")/../paths.sh"

if [ "${BUILD_PRISM}" = "no" ]; then
    echo "this test is not run when using --without-prism"
    exit 77
fi

export VFC_BACKENDS_LOGGER=False

make --silent PRISM_BACKEND=sr

FAIL=0

run() {
    local backends="$1"; shift
    local out
    out=$(VFC_BACKENDS="$backends" ./test_accuracy "$@" 2>/dev/null)
    echo "$out"
    echo "$out" | grep -q "^FAIL" && FAIL=1 || true
}

echo "--- scalar float (p=10, four ops) ---"
run "libinterflop_prism.so --precision-binary32 10" float div 10 1.0 3.0
run "libinterflop_prism.so --precision-binary32 10" float add 10 0.1 0.2
run "libinterflop_prism.so --precision-binary32 10" float mul 10 1.1 1.3
run "libinterflop_prism.so --precision-binary32 10" float fma 10 1.1 1.3 0.1

echo
echo "--- scalar float (p=24, full precision) ---"
run "libinterflop_prism.so --precision-binary32 24" float div 24 1.0 3.0
run "libinterflop_prism.so --precision-binary32 24" float add 24 0.1 0.2
run "libinterflop_prism.so --precision-binary32 24" float mul 24 1.1 1.3
run "libinterflop_prism.so --precision-binary32 24" float fma 24 1.1 1.3 0.1

echo
echo "--- scalar double (p=20, four ops) ---"
run "libinterflop_prism.so --precision-binary64 20" double div 20 1.0 3.0
run "libinterflop_prism.so --precision-binary64 20" double add 20 0.1 0.2
run "libinterflop_prism.so --precision-binary64 20" double mul 20 1.1 1.3
run "libinterflop_prism.so --precision-binary64 20" double fma 20 1.1 1.3 0.1

echo
echo "--- scalar double (p=53, full precision) ---"
run "libinterflop_prism.so --precision-binary64 53" double div 53 1.0 3.0
run "libinterflop_prism.so --precision-binary64 53" double add 53 0.1 0.2
run "libinterflop_prism.so --precision-binary64 53" double mul 53 1.1 1.3
run "libinterflop_prism.so --precision-binary64 53" double fma 53 1.1 1.3 0.1

echo
echo "--- vector float4 (p=10, four ops) ---"
run "libinterflop_prism.so --precision-binary32 10" vector div 10 1.0 3.0
run "libinterflop_prism.so --precision-binary32 10" vector add 10 0.1 0.2
run "libinterflop_prism.so --precision-binary32 10" vector mul 10 1.1 1.3
run "libinterflop_prism.so --precision-binary32 10" vector fma 10 1.1 1.3 0.1

echo
echo "--- vector float4 (p=24, full precision) ---"
run "libinterflop_prism.so --precision-binary32 24" vector div 24 1.0 3.0
run "libinterflop_prism.so --precision-binary32 24" vector add 24 0.1 0.2
run "libinterflop_prism.so --precision-binary32 24" vector mul 24 1.1 1.3
run "libinterflop_prism.so --precision-binary32 24" vector fma 24 1.1 1.3 0.1

echo
echo "--- vector double2 (p=20, four ops) ---"
run "libinterflop_prism.so --precision-binary64 20" vector-double div 20 1.0 3.0
run "libinterflop_prism.so --precision-binary64 20" vector-double add 20 0.1 0.2
run "libinterflop_prism.so --precision-binary64 20" vector-double mul 20 1.1 1.3
run "libinterflop_prism.so --precision-binary64 20" vector-double fma 20 1.1 1.3 0.1

echo
echo "--- vector double2 (p=53, full precision) ---"
run "libinterflop_prism.so --precision-binary64 53" vector-double div 53 1.0 3.0
run "libinterflop_prism.so --precision-binary64 53" vector-double add 53 0.1 0.2
run "libinterflop_prism.so --precision-binary64 53" vector-double mul 53 1.1 1.3
run "libinterflop_prism.so --precision-binary64 53" vector-double fma 53 1.1 1.3 0.1


echo

if [ $FAIL -eq 0 ]; then
    echo "PASS: all PRISM virtual precision accuracy checks passed"
    exit 0
else
    echo "FAIL: one or more checks failed (see output above)"
    exit 1
fi
