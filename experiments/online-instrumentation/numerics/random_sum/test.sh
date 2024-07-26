#!/bin/bash

for type in float double; do

    echo "Compiling test.c with $type precision"

    verificarlo-c test.c -DREAL=$type --online-instrumentation=up-down -O0 -o test_O0_ud_$type
    verificarlo-c test.c -DREAL=$type --online-instrumentation=up-down -O3 -o test_O3_ud_$type

    # rm -rf ud_results_${type}_O0.*.txt
    # rm -rf ud_results_${type}_O3.*.txt

    # echo "Running up-down instrumentation"
    # for i in {0..10}; do
    #     ./test_O0 $N $RANDOM >ud_results_${type}_O0.$i.txt
    #     ./test_O3 $N $RANDOM >ud_results_${type}_O3.$i.txt
    # done

    verificarlo-c test.c -DREAL=$type --online-instrumentation=sr -O0 -o test_O0_sr_$type -lm
    verificarlo-c test.c -DREAL=$type --online-instrumentation=sr -O3 -o test_O3_sr_$type -lm

    # rm -rf sr_results_${type}_O0.*.txt
    # rm -rf sr_results_${type}_O3.*.txt

    # echo "Running sr instrumentation"
    # for i in {0..10}; do
    #     ./test_O0 $N $RANDOM >sr_results_${type}_O0.$i.txt
    #     ./test_O3 $N $RANDOM >sr_results_${type}_O3.$i.txt
    # done

    verificarlo-c test.c -DREAL=$type -o test_O0_ieee_$type
    verificarlo-c test.c -DREAL=$type -o test_O3_ieee_$type

    cp test_O0_ieee_$type test_O0_mca_$type
    cp test_O3_ieee_$type test_O3_mca_$type

    # rm -rf ieee_results_${type}_O0.*.txt
    # rm -rf ieee_results_${type}_O3.*.txt

    # export VFC_BACKENDS_LOGGER=False
    # export VFC_BACKENDS_SILENT_LOAD=True
    # export VFC_BACKENDS="libinterflop_ieee.so"

    # echo "Running ieee instrumentation"
    # for i in {0..10}; do
    #     ./test_O0 $N $RANDOM >ieee_results_${type}_O0.$i.txt
    #     ./test_O3 $N $RANDOM >ieee_results_${type}_O3.$i.txt
    # done

    # rm -rf mca_results_${type}_O0.*.txt
    # rm -rf mca_results_${type}_O3.*.txt

    # export VFC_BACKENDS="libinterflop_mca.so -m rr"

    # echo "Running mca instrumentation"
    # for i in {0..10}; do
    #     ./test_O0 $N $RANDOM >mca_results_${type}_O0.$i.txt
    #     ./test_O3 $N $RANDOM >mca_results_${type}_O3.$i.txt
    # done

done

function run() {
    N=1000000

    local mode=$1
    local type=$2
    local backend=$3

    rm -rf ${mode}_results_${type}_O0.*.txt
    rm -rf ${mode}_results_${type}_O3.*.txt

    export VFC_BACKENDS_LOGGER=False
    export VFC_BACKENDS_SILENT_LOAD=True
    export VFC_BACKENDS=$backend

    echo "Running ${mode} instrumentation"
    perf stat -o 00_${mode}_${type}.perf -r 10 -- ./test_O0_${mode}_${type} $N 23 2>/dev/null
    perf stat -o 03_${mode}_${type}.perf -r 10 -- ./test_O3_${mode}_${type} $N 23 2>/dev/null

}

export -f run

parallel --progress -j 1 "run {1} {2} {3}" ::: ud sr ieee mca ::: float double ::: "libinterflop_ieee.so" "libinterflop_mca.so -m rr"
