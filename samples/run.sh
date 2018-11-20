#!/usr/bin/env bash

# Script to run all VSA on all test c-programs.
# Please specify the path to llvm and clang in the environment variables
# VSA_CLANG_PATH and VSA_LLVM_PATH.

if [[ -z ${VSA_CLANG_PATH+x} ]]; then
    echo Variable VSA_CLANG_PATH is not defined
    exit 1
fi

if [[ -z ${VSA_LLVM_PATH+x} ]]; then
    echo Variable VSA_CLANG_PATH is not defined
    exit 1
fi

while :; do
    case $1 in
        -t) flag1="TEST"
        ;;
        *) break
    esac
    shift
done

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    EXE=llvm-vsa.so
elif [[ "$OSTYPE" == "darwin"* ]]; then
    EXE=llvm-vsa.dylib
elif [[ "$OSTYPE" == "msys" ]]; then
    # We build the pass directly into opt on windows
    EXE=""
else
    echo "Unknown OS check run.sh:34"
fi

PASS=vsapass

# if one argument passed:
if [[ $# == 1 ]] ; then
    if [[ $1 == *.c ]] ; then
        # if it ends with .c find files ending with that name
        ARRAY=($(ls -d *$1))
    else
        # otherwise find all .c files whose names contain given substring
        ARRAY=($(ls -d *$1*.c))
    fi
else # run all
    ARRAY=($(ls -d *.c))
fi

# color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# if no folder is existent
mkdir -p build

# for all c-files...
for f in ${ARRAY[*]};
do
    # ... print file name
    echo "###############################################################################"
    echo $(pwd)/$f
    # ... clean up for old run
    rm -f build/$f.out
    rm -f build/$f.bc
    rm -f build/$f-opt.bc
    # ... compile
    ${VSA_CLANG_PATH}/bin/clang -O0 -emit-llvm $f -Xclang -disable-O0-optnone -c -o build/$f.bc
    # ... run mem2reg optimization
    ${VSA_LLVM_PATH}/bin/opt -mem2reg < build/$f.bc > build/$f-opt.bc
    # ... disassemble optimized file
    ${VSA_LLVM_PATH}/bin/llvm-dis build/$f-opt.bc

    # Extract the run parameters from the C file which are inclosed
    # in the first line by '// OPT: "????"'.
    PARAM=($(head -n 1 $f | grep "// OPT:" | sed -e 's/\/\/ OPT: "\(.*\)\"/\1/'))
    if [[ ! -z "$PARAM" ]];
      then
        echo "Passing additional parameters to opt: \"$PARAM\""
    fi

    # ... run VSA
    if [[ "$OSTYPE" == "msys" ]]; then
      # On windows we are expecting the opt tool to contain our pass
      ${VSA_LLVM_PATH}/bin/opt -${PASS} $PARAM < build/$f-opt.bc > /dev/null 2> >(tee build/$f.out >&2)
    else
      ${VSA_LLVM_PATH}/bin/opt -load ${VSA_LLVM_PATH}/lib/${EXE} -${PASS} $PARAM < build/$f-opt.bc > /dev/null 2> >(tee build/$f.out >&2)
    fi
    cp -n build/$f.out build/$f.ref
done

printf "\nTEST SUMMARY:\n"
for f in ${ARRAY[*]};
do
    # Find out to which file we are comparing the file
    if [[ -e vectors/$f.yml ]]
    then
        echo "Comparing against vector file vectors/$f.yml:"
        VECTOR_FILE="vectors/$f.yml"
    else
        echo "Comparing against the old output:"
        VECTOR_FILE="build/$f.ref"
    fi

    # Perform the actual comparison
    if cmp build/$f.out ${VECTOR_FILE}; then
        # ... success
        printf "Run: ${GREEN}$f${NC}\n"
    else
        # ... failure
        printf "Run: ${RED}$f${NC}\n"
    fi
done
printf "\n"
