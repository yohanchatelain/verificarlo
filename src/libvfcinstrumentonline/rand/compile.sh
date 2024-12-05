#!/bin/bash

bazelisk build src:prism src:prism-native src:prism-dbg src:prism-native-dbg

rm -rf $VIRTUAL_ENV/lib/libprism*
cp bazel-bin/src/libprism* $VIRTUAL_ENV/lib/
