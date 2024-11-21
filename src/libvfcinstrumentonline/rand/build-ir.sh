#!/bin/bash

bazelisk build --repo_env=CC=$(llvm-config-14 --bindir)/clang \
    --repo_env=CXX=$(llvm-config-14 --bindir)/clang++ \
    --compile_one_dependency //src:sr_hw.cpp \
    --cxxopt="-march=native" --cxxopt="-emit-llvm" --cxxopt="-O3" --save_temps \
    --cxxopt="-USR_DEBUG" --cxxopt="-g0"

bazelisk build --repo_env=CC=$(llvm-config-14 --bindir)/clang \
    --repo_env=CXX=$(llvm-config-14 --bindir)/clang++ \
    --compile_one_dependency //src:ud_hw.cpp \
    --cxxopt="-march=native" --cxxopt="-emit-llvm" --cxxopt="-O3" --save_temps \
    --cxxopt="-USR_DEBUG" --cxxopt="-g0"

cp -f bazel-bin/src/_objs/sr/sr_hw.pic.s sr_hw.ll
cp -f bazel-bin/src/_objs/ud/ud_hw.pic.s ud_hw.ll
