#!/bin/bash

VSA_LLVM_PATH=/home/philipp/uni/pollvm/build_llvm

llvm_config=$VSA_LLVM_PATH/bin/llvm-config

cd $(dirname "$0")

mkdir -p ../build

echo 'Building...'
g++ SimpleIntervalTest.cpp -I../src -fmax-errors=2 `$llvm_config --cxxflags` -o ../build/test/SimpleIntervalTest `$llvm_config --ldflags` $VSA_LLVM_PATH/lib/llvm-vsa.so `$llvm_config --libs analysis` -lz -lrt -ldl -ltinfo -lpthread -lm 

echo 'Running...'
../build/test/SimpleIntervalTest
