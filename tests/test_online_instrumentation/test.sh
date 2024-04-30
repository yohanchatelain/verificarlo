#!/bin/bash

set -e

SAMPLES=100

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

# Test operates at different precisions, and different operands.
# It compares that results are equivalents up to the bit.
# parallel --header : "verificarlo-c --function=operator --verbose -D REAL={type} -D SAMPLES=$SAMPLES {optimizations} test.c -o test_{type} -lm" ::: type float double ::: optimizations '-O0' '-O1' '-O2' '-O3' '-Ofast'

parallel --header : "make type={type} optimization={optimization} samples=$SAMPLES" ::: type float double ::: optimization "${optimizations[@]}"

for optim in "${optimizations[@]}"; do
    check_executable test_float_${optim}
    check_executable test_double_${optim}
done

for type in float double; do
    for op in "+" "-" "x" "/"; do
        for optim in "${optimizations[@]}"; do
            ./test_float_${optim} $op 0.1 0.1 >tmp.float.${op}.${optim}.txt
            ./test_double_${optim} $op 0.1 0.1 >tmp.double.${op}.${optim}.txt
        done
    done
done

# parallel -j $(nproc) <run_parallel

cat >check_status.py <<HERE
import numpy as np
import sys
np.loadtxt(sys.argv[1])
HERE

# cat >check_status.py <<HERE
# import sys
# import glob
# paths=glob.glob('tmp.*/output.txt')
# ret=sum([int(open(f).readline().strip()) for f in paths]) if paths != []  else 1
# print(ret)
# HERE

status=$(python3 check_status.py)

if [ $status -eq 0 ]; then
    echo "Success!"
else
    echo "Failed!"
fi

rm -rf tmp.*

exit $status
