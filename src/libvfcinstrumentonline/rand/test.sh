#!/bin/bash

if [ $# -ne 3 ]; then
    echo "Usage: $0 <op> <a> <b>"
    exit 1
fi

verificarlo-c++ -DREAL=$REAL --function=apply_op test.cpp -std=c++17 -Wall -o test

export VFC_BACKENDS="libinterflop_mca.so --precision-binary64=53 --precision-binary32=24"
export VFC_BACKENDS_LOGGER=False
./test $1 $2 $3
