#!/bin/bash
set -e

OPTIONS_LIST=(
    "-O0"
    "-O3 -ffast-math"
)

export VFC_BACKENDS="libinterflop_mca.so --precision-binary32 24"

for OPTION in "${OPTIONS_LIST[@]}"; do
    verificarlo --verbose --report --function sum_kahan ${OPTION} kahan.c  -o test
done
