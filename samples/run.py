#!/bin/python3
# coding= utf-8
import argparse
import os.path
import os

# SET YOUR PATHS HERE
LLVM_PATH = "/home/thomas/Dokumente/Nextcloud/Uni/05/Praktikum_LLVM/llvm-7.0.0.src/cmake-build-debug"
CLANG_PATH = LLVM_PATH

#TODO:
#test if every necessary programs are compiled (opt, clang, ...)
#add flag for command line output
#work with config file

#do not modify
PASS_LIB = LLVM_PATH + "/lib/llvm-pain.so"
PASS = "painpass"
MAKE_TARGET = "llvm-pain"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("program", help="run on specific c file", nargs='*')
    parser.add_argument("-cfg", help="show llvm control flow graph", action="store_true")
    parser.add_argument("-print_commands", help="prints the commands executed", action="store_true")
    parser.add_argument("-m", help="compiles pass before execution", action="store_true")
    args = parser.parse_args()

    # if no files are specified set it to all .c files in current directory
    if len(args.program) == 0:
        args.program = filter(lambda file : str(file).endswith('.c'), os.listdir(os.getcwd()))

    if args.m:
        print( "Started build" )
        cur_dir = os.getcwd()
        os.chdir(LLVM_PATH)
        i = os.system("make "+MAKE_TARGET)
        os.chdir(cur_dir)
        if i:
            print ("Build failed")
            exit(i)
        else:
            print( "build finished" )

    for file in args.program:
        if not os.path.isfile(file):
            print ("ERROR: " + file +" wasn't found!")
            continue

        print( "Running on file " + file + " ..." )

        if args.print_commands:
            print (getCompileToByteCodeCMD(file))
            print (getMem2RegCMD(file))
            print (getHumanReadableCMD(file))
            if args.cfg:
                print (getRunPassCMD(file) + " -view-cfg")
            else:
                print (getRunPassCMD(file))
            return

        #create build directory
        os.system("mkdir -p build")

        #execute tests
        os.system(getCompileToByteCodeCMD(file))
        os.system(getMem2RegCMD(file))
        os.system(getHumanReadableCMD(file))
        if args.cfg:
            os.system(getRunPassCMD(file) + " -view-cfg")
        else:
            os.system(getRunPassCMD(file))

        #remove bytecode
        os.system ("rm -r build/" + file + ".bc build/" + file + "-opt.bc")

    print( "Done" )


def getCompileToByteCodeCMD(filename):
    return CLANG_PATH + "/bin/clang -O0 -emit-llvm " + filename + " -Xclang -disable-O0-optnone -c -o build/" + filename + ".bc"

def getMem2RegCMD(filename):
    return LLVM_PATH + "/bin/opt -mem2reg build/" + filename + ".bc -o build/" + filename + "-opt.bc"

def getHumanReadableCMD(filename):
    return LLVM_PATH + "/bin/llvm-dis build/" + filename + "-opt.bc"

def getRunPassCMD(filename):
    return LLVM_PATH + '/bin/opt -load "' + PASS_LIB + '" -' + PASS + ' -S -o /dev/null build/' + filename + '-opt.bc > build/' + filename + '.out 2>&1'

if __name__ == "__main__":
    main()