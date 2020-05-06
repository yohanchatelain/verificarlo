#!/bin/bash

backends=("libinterflop_ieee.so" \
	      "libinterflop_mca.so" \
	      "libinterflop_mca_mpfr.so" \
	      "libinterflop_bitmask.so" \
	      "libinterflop_cancellation.so" \
	      "libinterflop_vprec.so")

check_status() {
    if [[ $? != 0 ]]; then
	echo "Fail"
	exit 1
    fi
}

verificarlo test.c -o test
check_status

for backend in "${backends[@]}"; do
    export VFC_BACKENDS=$backend
    ./test
    check_status
done
