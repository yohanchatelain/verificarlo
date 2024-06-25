#!/bin/bash

set -e

./test_scalar.sh

if [[ $? != 0 ]]; then
    echo "Failed!"
    exit 1
fi

./test_vector.sh

if [[ $? != 0 ]]; then
    echo "Failed!"
    exit 1
else
    echo "Success!"
    exit 0
fi
