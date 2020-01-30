#!/bin/bash
set -e

if grep "undef FLANG_PATH" ../../config.h > /dev/null; then
	echo "this test is not run when not using --with-flang"
	exit 0
fi

cd NPB3.0-SER
mkdir -p bin
./run-bench.sh

echo 'Test successed'
exit 0
