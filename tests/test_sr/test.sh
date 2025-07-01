#!/bin/bash
set -e

rm -rf logs/ binary32-* binary64-* sr
mkdir logs/
export VFC_BACKENDS_LOGFILE="logs/verificarlo.log"

ITERATIONS=100

parallel --header : "verificarlo-c -DITERATIONS=$ITERATIONS -O0  sr-{type}.c -o sr-{type} -lm" ::: type binary32 binary64

trap 'echo "Failed with backend $BACKEND"; exit 1' ERR

for BACKEND in libinterflop_mca.so libinterflop_mca_int.so; do

  for p in 23 24 25 26; do
    for PREC in "--mode=rr --precision-binary32=24"; do
      VFC_BACKENDS="$BACKEND $PREC" ./sr-binary32 $p >binary32-$p-$BACKEND
    done
  done

  for p in 52 53 54 55; do
    for PREC in "--mode=rr --precision-binary64=53"; do
      VFC_BACKENDS="$BACKEND $PREC" ./sr-binary64 $p >binary64-$p-$BACKEND
    done
  done

  ./check.py $ITERATIONS $BACKEND
  echo "Success with backend $BACKEND!"

done

exit 0
