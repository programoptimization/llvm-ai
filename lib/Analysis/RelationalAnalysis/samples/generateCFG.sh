#!/usr/bin/env bash

while :; do
    case $1 in
        -m|--mem2reg)
        optFlag1="mem2reg"
        ;;
        *) break
    esac
    shift
done

# if one argument passed: only analyze the passed program
if [ $# == 1 ] ; then
    ARRAY=($1)
else # run all samples
    ARRAY=($(ls -d *.c))
fi

# if no folder is existent
mkdir -p build

for f in ${ARRAY[*]};
do
    # ... print file name
    echo "###############################################################################"
    echo $(pwd)/$f
    # ... clean up for old run
    rm -f build/$f.out
    rm -f build/$f-opt.bc
	rm -f build/$f.*.pdf
	rm -f build/$f.*.dot

    # ... compile
    printf "${BLUE} Compiling to llvm IR using clang... ${NC}\n"
    $LLVM_BUILD_PATH/bin/clang -O0 -emit-llvm $f -Xclang -disable-O0-optnone -c -o build/$f.bc

    if  [ "$optFlag1" = "mem2reg" ]; then
        # ... run mem2reg optimization
        printf "${BLUE} Optimizing using mem2reg... ${NC}\n"
        $LLVM_BUILD_PATH/bin/opt -mem2reg < build/$f.bc > build/$f-opt.bc
        # ... disassemble optimized file
        printf "${BLUE} Disassemble optimized file... ${NC}\n"
        $LLVM_BUILD_PATH/bin/llvm-dis build/$f-opt.bc
        # generate .dot from CFG
        $LLVM_BUILD_PATH/bin/opt -dot-cfg < build/$f-opt.bc > /dev/null
    else
        # generate .dot from CFG
        $LLVM_BUILD_PATH/bin/opt -dot-cfg < build/$f.bc > /dev/null
    fi

    # convert .dot to .pdf and store under correct path
	DOT_FILES=($(ls -d *.dot))
	for dotfile in ${DOT_FILES[*]};
	do
		dot -Tpdf $dotfile -o build/$f.$dotfile.pdf
	done
    # remove .dot files
    rm -f *.*.dot
	# pls dont
	open build/*dot.pdf
done
