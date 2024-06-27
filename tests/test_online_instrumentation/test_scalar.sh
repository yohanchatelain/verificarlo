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

parallel --header : "make --silent type={type} optimization={optimization} operator={operator} rng={rng}" ::: type float double ::: optimization "${optimizations[@]}" ::: operator add sub mul div ::: rng xoroshiro shishua

run_test() {
    declare -A operation_name=(["+"]="add" ["-"]="sub" ["x"]="mul" ["/"]="div")

    local type="$1"
    local optimization="$2"
    local op="$3"
    local rng="$4"
    local op_name=${operation_name[$op]}

    local bin=test_${type}_${optimization}_${op_name}_${rng}
    local file=tmp.$type.$op_name.$optimization.$rng.txt

    echo "Running test $type $op $optimization $op_name $rng"

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

for rng in xoroshiro shishua; do
    echo "Running tests with rng: $rng"
    parallel --halt now,fail=1 --header : "run_test {type} {optimization} {op} {rng}" \
        ::: type float double \
        ::: op "+" "-" "x" "/" \
        ::: optimization "${optimizations[@]}" \
        ::: rng $rng

    if [[ $? != 0 ]]; then
        echo "Failed!"
        exit 1
    fi
done

echo "Success!"
exit 0
