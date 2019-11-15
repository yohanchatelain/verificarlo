#!/bin/bash
set -e

if grep "FLANG_PATH \"\"" ../../config.h > /dev/null; then
	echo "this test is not run when using --without-dragonegg"
	exit 0
fi

cd NPB3.0-SER
./run-bench.sh

echo 'Test successed'
exit 0
