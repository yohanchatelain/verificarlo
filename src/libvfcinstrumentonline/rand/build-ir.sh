#!/bin/bash

export CLANG=$(llvm-config --bindir)/clang
export CLANGXX=$(llvm-config --bindir)/clang++



bazelisk build --repo_env=CC=$CLANG \
    --repo_env=CXX=$CLANGXX \
    --compile_one_dependency //src:sr_hw.cpp \
    --cxxopt="-march=native" --cxxopt="-emit-llvm" --cxxopt="-O3" --save_temps \
    --cxxopt="-USR_DEBUG" --cxxopt="-g0" --suppress_output

bazelisk build --repo_env=CC=$CLANG \
    --repo_env=CXX=$CLANGXX \
    --compile_one_dependency //src:ud_hw.cpp \
    --cxxopt="-march=native" --cxxopt="-emit-llvm" --cxxopt="-O3" --save_temps \
    --cxxopt="-USR_DEBUG" --cxxopt="-g0"



if [ $! -ne 0 ]; then
    $CLANGXX -S -emit-llvm src/sr_hw.cpp -march=native -O3 -USR_DEBUG -o sr_hw.ll -I. -Ibazel-rand/external/_main~_repo_rules~hwy/ -std=c++17
    $CLANGXX -S -emit-llvm src/ud_hw.cpp -march=native -O3 -USR_DEBUG -o ud_hw.ll -I. -Ibazel-rand/external/_main~_repo_rules~hwy/ -std=c++17
fi

cp -f bazel-bin/src/_objs/sr/sr_hw.pic.s sr_hw.ll
cp -f bazel-bin/src/_objs/ud/ud_hw.pic.s ud_hw.ll
