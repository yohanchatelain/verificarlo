#!/bin/bash

declare -A backends
backends[ieee]='libinterflop_ieee.so'
backends[mca]='libinterflop_mca.so'

reset() {
    unset VFC_BACKENDS
    unset VFC_BACKENDS_FROM_FILE
}

set_vfcbackends() {
    export VFC_BACKENDS=${backends[$1]}
}

set_vfcbackends_from_file() {
    FILE=vfcbackends.txt
    export VFC_BACKENDS_FROM_FILE=$FILE
    echo ${backends[$1]} > $FILE
}

check() {
    ./test &> log.txt
    cat log.txt
    grep "${1}" log.txt
    if [[ $? == 0 ]]; then
        echo "Pass"
    else
        echo "Fail"
        exit 1
    fi
}

verificarlo-c test.c -o test

# Test 1
echo "SUBTEST 1: Check that an error is raised when no VFC_BACKENDS is set"
reset
check "VFC_BACKENDS is empty, at least one backend should be provided"

# Test 2
echo "SUBTEST 2: Check that an error is raised when an invalid backend is given"
reset
export VFC_BACKENDS="libinterflop_idk.so"
check "cannot open shared object file: No such file or directory"

# Test 3
echo "SUBTEST 3: Check that an error is raise when the file pointed by VFC_BACKENDS_FROM_FILE is unknown"
reset
export VFC_BACKENDS_FROM_FILE=a
check "Error while opening file pointed by VFC_BACKENDS_FROM_FILE"

# Test 4
echo "SUBTEST 4: Check that VFC_BACKENDS is set"
reset
set_vfcbackends ieee
check ieee

# Test 5
echo "SUBTEST 5: Check that an VFC_BACKENDS_FROM_FILE is set"
reset
set_vfcbackends_from_file mca
check mca

# Test 6
echo "SUBTEST 6: Check that VFC_BACKENDS takes precedence over VFC_BACKENDS_FROM_FILE"
reset
set_vfcbackends ieee
set_vfcbackends_from_file mca
check ieee

