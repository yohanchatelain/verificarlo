#!/bin/bash

set -e

export SAMPLES=100

# check if variable static is set
: ${STATIC_DISPATCH:=0}

if [[ $STATIC_DISPATCH -eq 1 ]]; then
    DISPATCH="static=1"
else
    DISPATCH=
fi

# check if variable march_native is equal to 1
# Ensure MARCH_NATIVE is set to either 1 or 0
: ${MARCH_NATIVE:=0}

if [[ $MARCH_NATIVE -eq 1 ]]; then
    MARCH="native=1"
else
    MARCH=""
fi

if [[ -z ${VERBOSE+x} ]]; then
    VERBOSE=0
else
    VERBOSE=1
fi

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

mkdir -p .bin .objects .results

optimizations=('-O0' '-O1' '-O2' '-O3' '-Ofast')

export VFC_BACKENDS_LOGGER=False

parallel --halt now,fail=1 --header : "make --silent type={type} optimization={optimization} operator={operator} size={size} ${DISPATCH} ${MARCH}" \
    ::: type float double \
    ::: optimization "${optimizations[@]}" \
    ::: operator add sub mul div \
    ::: size 2 4 8 16

run_test() {
    declare -A operation_name=(["+"]="add" ["-"]="sub" ["x"]="mul" ["/"]="div")

    declare -A args
    args["float+"]="0.1 0.01"
    args["float-"]="0.1 0.001"
    args["floatx"]="0.1 0.001"
    args["float/"]="1 313"
    args["floats"]="0.1"
    args["double+"]="0.1 0.01"
    args["double-"]="0.1 0.001"
    args["doublex"]="0.1 0.01"
    args["double/"]="1 3"
    args["doubles"]="0.1"

    local type="$1"
    local optimization="$2"
    local op="$3"
    local size="$4"
    local op_name=${operation_name[$op]}

    local bin=.bin/test_${type}_${optimization}_${op_name}_${size}
    local file=.results/tmp.$type.x$size.$op_name.$optimization.txt

    if [[ $VERBOSE == 1 ]]; then
        echo "Running test $type x$size $op $optimization $op_name on ${args["$type$op"]}..."
    fi

    rm -f $file

    for i in $(seq 1 $SAMPLES); do
        ./$bin $op ${args["$type$op"]} &>>$file
    done

    if [[ $? != 0 ]]; then
        echo "Failed!"
        exit 1
    fi

    # Check if we have variabilities
    if [[ $(sort -u $file | wc -l) == 1 || $(sort -u $file | wc -l) == 0 ]]; then
        echo "Failed!"
        echo "File $file failed"
        sort -u $file
        echo "To reproduce the error run:"
        echo "make --silent type=$type optimization=$optimization operator=$op size=$size ${DISPATCH} ${MARCH}"
        echo "    ./$bin $op ${args["$type$op"]}"
        exit 1
    fi

    # Check that variabililty is within 2 significant digits
    if [[ $(./check_variability.py $file) ]]; then
        echo "Failed!"
        echo "File $file failed"
        exit 1
    fi
}

export -f run_test
export CHAR_POSITION=0

if [[ ${TEST_DEBUG} -eq 1 ]]; then
    VERBOSE=1
    echo "Running in debug mode"
    parallel --header : "run_test {type} {optimization} {op} {size}" \
        ::: type double float \
        ::: op "+" "-" "x" "/" \
        ::: optimization "${optimizations[@]}" \
        ::: size 2 4 8 16
else
    parallel --bar --halt now,fail=1 --header : "run_test {type} {optimization} {op} {size}" \
        ::: type double float \
        ::: op "+" "-" "x" "/" \
        ::: optimization "${optimizations[@]}" \
        ::: size 2 4 8 16
fi

if [[ $VERBOSE == 1 ]]; then
    echo "Success!"
fi

exit 0
