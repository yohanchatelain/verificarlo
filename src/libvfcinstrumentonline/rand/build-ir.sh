#!/bin/bash

export CLANG=clang-14
export CLANGXX=clang++-14

$CLANGXX -S -emit-llvm src/sr_hw.cpp -O2 -USR_DEBUG -o sr_hw.ll -I. -Ibazel-rand/external/_main~_repo_rules~hwy/ -std=c++17 &
$CLANGXX -S -emit-llvm src/ud_hw.cpp -O2 -USR_DEBUG -o ud_hw.ll -I. -Ibazel-rand/external/_main~_repo_rules~hwy/ -std=c++17 &

$CLANGXX -S -emit-llvm src/sr_hw.cpp -march=native -O2 -USR_DEBUG -o sr_hw-native.ll -I. -Ibazel-rand/external/_main~_repo_rules~hwy/ -std=c++17 &
$CLANGXX -S -emit-llvm src/ud_hw.cpp -march=native -O2 -USR_DEBUG -o ud_hw-native.ll -I. -Ibazel-rand/external/_main~_repo_rules~hwy/ -std=c++17 &

wait
