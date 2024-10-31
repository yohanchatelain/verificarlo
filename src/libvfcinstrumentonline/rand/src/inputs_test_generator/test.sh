#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: $0 <num1> <num2>"
    exit 1
fi

export VFC_BACKENDS="libinterflop_mca_int.so -m rr"
export VFC_BACKENDS_SILENT_LOAD=True
export VFC_BACKENDS_LOGGER=False

if [ ! -f test_float ]; then
    echo "test_float executable not found"
    echo "Please compile with 'make'"
    exit 1
fi

if [ ! -f test_double ]; then
    echo "test_double executable not found"
    echo "Please compile with 'make'"
    exit 1
fi

# Test 1
echo "float"
for op in + - x /; do
    echo "Operation: $op"
    ./test_float $op $1 $2
    echo ""
done

echo "double"
for op in + - x /; do
    echo "Operation: $op"
    ./test_double $op $1 $2
    echo ""
done
