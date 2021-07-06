#!/bin/bash

set -e

cd $2$1/test/build
./supla_$1_test --gtest_repeat=20 --gtest_shuffle

