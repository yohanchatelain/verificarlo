#!/bin/bash

set -e

clang -I ../../venv/include/ -I ../../src/libvfcinstrumentonline/ -DREAL_TYPE=DOUBLE ../../src/libvfcinstrumentonline/rand.c test_local.c -lm -o test_double -O3

clang -I ../../venv/include/ -I ../../src/libvfcinstrumentonline/ -DREAL_TYPE=FLOAT ../../src/libvfcinstrumentonline/rand.c test_local.c -lm -lquadmath -o test_float -O3

function check_variability() {
    local type=$1
    local output_file=output.txt
    rm -f $output_file
    for i in {0..100}; do
        ./test_$type ${@:2} >>$output_file
    done
    # check if the file contains two different values
    local res=$(sort -u $output_file | wc -l)
    if [[ $res -ne 2 ]]; then
        echo " [FAIL]"
        exit 1
    else
        echo " [PASS]"
    fi
}

echo "Testing single precision"

echo -n "* testing addition"
check_variability float + .1 .01

echo -n "* testing substraction"
check_variability float - .1 .001

echo -n "* testing multiplication"
check_variability float x .1 .001

echo -n "* testing division"
check_variability float / 1 313

echo -n "* testing sqrt"
check_variability float s .1

echo "Testing double precision"

echo -n "* testing addition"
check_variability double + .1 .01

echo -n "* testing substraction"
check_variability double - .1 .001

echo -n "* testing multiplication"
check_variability double x .1 .01

echo -n "* testing division"
check_variability double / 1 3

echo -n "* testing sqrt"
check_variability double s .1
