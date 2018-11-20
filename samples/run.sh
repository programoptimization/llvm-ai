#!/bin/bash

# Script to run all VSA on all test c-programs.
# Please specify the path to llvm and clang in the environment variables
# VSA_CLANG_PATH and VSA_LLVM_PATH.

cd $(dirname "$0")

if [ -z "$VSA_LLVM_PATH" ]; then
    echo 'No VSA_LLVM_PATH environment variable specified. Please do something like'
    echo '    export VSA_LLVM_PATH=/home/philipp/uni/pollvm/build_llvm'
    echo 'or whatever your IDE wants you to do if you use one.'
    exit 1
fi

if [ -z "$VSA_CLANG_PATH" ]; then
    VSA_CLANG_PATH="$VSA_LLVM_PATH"
    echo 'Info: Initialising clang path to the same as VSA_LLVM_PATH'
fi

if [ ! -x "$VSA_LLVM_PATH/bin/opt" ]; then
    echo "Error: "'"'"$VSA_LLVM_PATH/bin/opt"'"'"does not exist."
    exit 2
fi
if [ ! -x "$VSA_CLANG_PATH/bin/clang" ]; then
    echo "Error: "'"'"$VSA_CLANG_PATH/bin/clang"'"'"does not exist."
    exit 2
fi

PASS_LIB="$VSA_LLVM_PATH/lib/llvm-pain.so"
PASS=painpass

if [ ! -x "$PASS_LIB" ]; then
    echo "Error: Could not find "'"'"$PASS_LIB"'"'", have you tried building the program?"
    exit 3
fi

# If an argument was passed we analyse only that file
if [ $# == 1 ] ; then
    ARRAY=($1) 
else
    ARRAY=($(ls -d *.c))
fi

mkdir -p build
# for all c-files...
for f in ${ARRAY[*]};
do
    echo "Processing $(pwd)/$f ..."
    rm -f build/$f.out build/$f.bc build/$f-opt.bc
    # Compile to bitcode
    $VSA_CLANG_PATH/bin/clang -O0 -emit-llvm $f -Xclang -disable-O0-optnone -c -o build/$f.bc
    # Run a preliminary pass to transform memory accesses into registers
    $VSA_LLVM_PATH/bin/opt -mem2reg build/$f.bc -o build/$f-opt.bc
    # Write the human readable version
    $VSA_LLVM_PATH/bin/llvm-dis build/$f-opt.bc
    # Run our own pass on that
    $VSA_LLVM_PATH/bin/opt -load "$PASS_LIB" -$PASS -S -o /dev/null build/$f-opt.bc > build/$f.out 2>&1
    if [ $? -ne 0 ]; then
        echo "Error while executing opt:"
        cat build/$f.out
        exit 4
    fi
    # Note: Some IDEs cannot handle redirects well, so just remove everything starting at '>' when
    # you copy-paste the above command into it.
done
