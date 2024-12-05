#!/bin/bash

set -e

echo "Test scalar"
./test_scalar.sh

if [[ $? != 0 ]]; then
    echo "Failed!"
    exit 1
fi

echo "Test vector dynamic dispatch"
./test_vector.sh

if [[ $? != 0 ]]; then
    echo "Failed!"
    exit 1
fi

./clean.sh

# echo "Test vector static dispatch"
# STATIC_DISPATCH=1 ./test_vector.sh

# if [[ $? != 0 ]]; then
#     echo "Failed!"
#     exit 1
# fi

# ./clean.sh

echo "Test vector dynamic dispatch -march=native"
MARCH_NATIVE=1 ./test_vector.sh

if [[ $? != 0 ]]; then
    echo "Failed!"
    exit 1
fi

./clean.sh

echo "Test vector static dispatch -march=native"
STATIC_DISPATCH=1 MARCH_NATIVE=1 ./test_vector.sh

if [[ $? != 0 ]]; then
    echo "Failed!"
    exit 1
fi

echo "Success!"
exit 0
