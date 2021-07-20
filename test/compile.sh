#!/bin/bash

set -e

cd $2$1/test
mkdir -p build
cd build
cmake ..
make -j32

