#!/bin/sh
autoreconf -is

rm -rf src/tools/sigdigits
git submodule init
git submodule update
