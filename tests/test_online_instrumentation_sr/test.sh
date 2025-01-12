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

# SCALAR TESTS

run "Test scalar dynamic dispatch" test_scalar.sh
run "Test scalar static dispatch" test_scalar.sh 1
run "Test scalar dynamic dispatch -march=native" test_scalar.sh 0 1
run "Test scalar static dispatch -march=native" test_scalar.sh 1 1

# VECTOR TESTS

run "Test vector dynamic dispatch" test_vector.sh
run "Test vector static dispatch" test_vector.sh 1 0 1
run "Test vector dynamic dispatch -march=native" test_vector.sh 0 1
run "Test vector static dispatch -march=native" test_vector.sh 1 1

exit 0
