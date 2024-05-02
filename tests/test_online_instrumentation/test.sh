#!/bin/bash

set -e

export SAMPLES=100

check_status() {
    if [[ $? != 0 ]]; then
        echo "Fail"
        exit 1
    fi
}

check_executable() {
    if [[ ! -f $1 ]]; then
        echo "Executable $1 not found"
        exit 1
    fi
}

optimizations=('-O0' '-O1' '-O2' '-O3' '-Ofast')

export VFC_BACKENDS_LOGGER=False

parallel --header : "make --silent type={type} optimization={optimization}" ::: type float double ::: optimization "${optimizations[@]}"

run_test() {
    declare -A operation_name=(["+"]="add" ["-"]="sub" ["x"]="mul" ["/"]="div")

    local type="$1"
    local optimization="$2"
    local op="$3"
    local op_name=${operation_name[$op]}

    local bin=test_${type}_${optimization}
    local file=tmp.$type.$op_name.$optimization.txt

    echo "Running test $type $op $optimization $op_name"

    rm -f $file

    for i in $(seq 1 $SAMPLES); do
        ./$bin $op 0.1 0.2 >>$file
    done

    if [[ $? != 0 ]]; then
        echo "Failed!"
        exit 1
    fi

    # Check if we have variabilities
    if [[ $(sort -u $file | wc -l) == 1 ]]; then
        echo "Failed!"
        echo "File $file failed"
        sort -u $file
        exit 1
    fi
}

export -f run_test

parallel --halt now,fail=1 --header : "run_test {type} {optimization} {op} " \
    ::: type float double \
    ::: op "+" "-" "x" "/" \
    ::: optimization "${optimizations[@]}"

if [[ $? != 0 ]]; then
    echo "Failed!"
    exit 1
else
    echo "Success!"
    exit 0
fi
