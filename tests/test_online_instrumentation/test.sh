#!/bin/bash

function run() {
    local msg=$1
    local cmd=$2
    local static_dispatch=${3:-0}
    local march_native=${4:-0}
    local xfail=${5:-0}

    echo $msg
    STATIC_DISPATCH=${static_dispatch} MARCH_NATIVE=${march_native} ./$cmd

    if [[ $? != 0 ]]; then
        if [[ $xfail == 1 ]]; then
            echo "XFAIL"
        else
            echo "FAIL"
            exit 1
        fi
    else
        echo "PASS"
    fi
    ./clean.sh
}

DYNAMIC=0
STATIC=1

BASELINE=0
NATIVE=1

PASS=0
XFAIL=1

# SCALAR TESTS
run "Test scalar dynamic dispatch" test_scalar.sh $DYNAMIC $BASELINE
run "Test scalar static dispatch" test_scalar.sh $STATIC $BASELINE
run "Test scalar dynamic dispatch -march=native" test_scalar.sh $DYNAMIC $NATIVE
run "Test scalar static dispatch -march=native" test_scalar.sh $STATIC $NATIVE

# VECTOR TESTS
run "Test vector dynamic dispatch" test_vector.sh $DYNAMIC $BASELINE
run "Test vector static dispatch" test_vector.sh $STATIC $BASELINE $XFAIL
run "Test vector dynamic dispatch -march=native" test_vector.sh $DYNAMIC $NATIVE
run "Test vector static dispatch -march=native" test_vector.sh $STATIC $NATIVE

exit 0
