#!/bin/bash
set -e

source ../paths.sh

verificarlo-rust main.rs -o main_test

export VFC_BACKENDS="libinterflop_bitmask.so --precision-binary64=10 --operator=zero"
./main_test > output1

export VFC_BACKENDS="libinterflop_ieee.so"
./main_test > output2

if diff output1 output2; then
    echo "output should differ with bitmask backend precision truncation"
    exit 1
else
    echo "Rust instrumentation test passed"
    exit 0
fi
