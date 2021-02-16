#!/bin/bash

export VFC_BACKENDS="libinterflop_mca.so"

make
veritracer launch --jobs=16 --binary="tchebychev EXPANDED 100 > /dev/null" --force
veritracer analyze --format=binary
HASH=$(grep ret locationInfo.map | grep expanded | cut -d':' -f1)
veritracer plot .vtrace/veritracer.000bt --no-show --output=tchebychev.png --std --mean --invocation-mode --transparency=0.5 -v $HASH