#!/bin/bash

set -e

echo "Test scalar"
./test_scalar.sh

if [[ $? != 0 ]]; then
    echo "Failed!"
    exit 1
fi

echo "Test vector"
./test_vector.sh

if [[ $? != 0 ]]; then
    echo "Failed!"
    exit 1
else
    echo "Success!"
    exit 0
fi
