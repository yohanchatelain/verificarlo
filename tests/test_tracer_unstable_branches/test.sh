#!/bin/bash

rm -f locationInfo.map
make dot

export VFC_BACKENDS="libinterflop_mca.so"

veritracer launch --jobs=16 --binary="dot_product 10" --force
veritracer analyze --filename veritracer.dat
HASH=$(grep res locationInfo.map | cut -d':' -f1)
echo $HASH
veritracer plot .vtrace/veritracer.000bt --no-show --output=dot_product.png  --invocation-mode --transparency=0.5 -v $HASH -bt=.vtrace/.backtrace/000bt.rdc
