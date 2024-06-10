#!/bin/bash

function run_test() {
    local n=$1
    local args=${@:2}
    for i in $(seq 1 $n); do
        ./test $args >/dev/null
    done
}

export -f run_test

declare -A operation_name=(["+"]="add" ["-"]="sub" ["x"]="mul" ["/"]="div", ["f"]="fma")

for size in 10 100 1000 10000; do
    for op in + - x /; do
        file=perf.data.${size}.${operation_name[$op]}
        echo "Running test $size $op on ${operation_name[$op]}..."
        perf stat -r 30 -x, -o $file -- bash -c "run_test ${size} ${op} 0.1 0.01"
    done
done

for size in 10 100 1000 10000; do
    for op in f; do
        file=perf.data.${size}.${operation_name[$op]}
        echo "Running test $size $op on ${operation_name[$op]}..."
        perf stat -r 30 -x, -o $file -- bash -c "run_test ${size} ${op} 0.1 0.01 0.001"
    done
done
