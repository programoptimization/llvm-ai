#!/usr/bin/python3
# coding: utf-8

import argparse
import os
import platform
import shlex
import subprocess
import sys

if sys.version_info[0] < 3:
    print("Error: This script only supports Python 3")
    sys.exit(5)

project_dir = os.path.abspath(os.path.dirname(sys.argv[0]))
os.chdir(project_dir)

if not os.path.isfile(".config"):
    print("No config file was found. Please run init.py first!")
    sys.exit(-1)

config_file = open(".config", "r")
lines = config_file.readlines()
llvm_path = lines[1].strip()
clang_path = lines[2].strip()
config_file.close()

opt = llvm_path + '/bin/opt'
llvm_dis = llvm_path + '/bin/llvm-dis'
llvm_config = llvm_path + '/bin/llvm-config'
clang = clang_path + '/bin/clang'
cmake = 'cmake'
gdb = 'gdb'

CXX = os.environ.get('CXX', 'c++')

if not os.path.isfile(opt):
    print('Error: no opt exists at ' + opt + ' (maybe you forgot to build LLVM?)')
    sys.exit(2)
if not os.path.isfile(llvm_dis):
    print('Error: no llvm-dis exists at ' + llvm_dis + ' (maybe you forgot to build LLVM?)')
    sys.exit(2)
if not os.path.isfile(clang):
    print('Error: no clang exists at ' + clang)
    sys.exit(2)

if platform.system() == 'Linux':
    libeext = '.so'
elif platform.system() == 'Windows':
    libeext = '.dll'
    print('Error: Windows is not supported. (You can try to delete this error and proceed at your own risk.')
    sys.exit(4)
elif platform.system() == 'Darwin':
    libeext = '.dylib'
else:
    print('Error: Unknown platform ' + platform.system())
    sys.exit(4)
    
pass_lib = llvm_path + "/lib/llvm-pain" + libeext
pass_name = "painpass"
make_target = "llvm-pain"

samples = project_dir + '/samples'

def main():
    def run(arg, cwd=None, redirect=None):
        if not args.only_print:
            try:
                if redirect:
                    f = open(redirect, 'w')
                    subprocess.run(arg, cwd=cwd, stdout=f, stderr=f, check=True)
                    f.close()
                else:
                    subprocess.run(arg, cwd=cwd, check=True)
            except subprocess.CalledProcessError as e:
                print('Error: while executing ' + str(e.cmd))
                if redirect:
                    f.close()
                    print(open(redirect, 'r').read())
                sys.exit(3)
        else:
            cmd = ' '.join(shlex.quote(i) for i in arg)
            if redirect:
                cmd += ' > %s 2>&1' % (shlex.quote(redirect),)
            if cwd is None:
                print('    ' + cmd)
            else:
                print('    ( cd %s && %s )' % (shlex.quote(cwd), cmd))
    
    parser = argparse.ArgumentParser()
    parser.add_argument("file", help="run only the specfied files", nargs='*')
    parser.add_argument("-n", dest='only_print', help="only print the commands, do not execute anything", action="store_true")
    parser.add_argument("-v", dest='show_output', help="show output on stdout", action="store_true")
    parser.add_argument("--cfg", dest='view_cfg', help="show llvm control flow graph", action="store_true")
    parser.add_argument("--make", dest='do_make', help="call make before executing the script", action="store_true")
    parser.add_argument("--only-make", dest='do_make_only', help="only call make, do not execute any samples", action="store_true")
    parser.add_argument("--gdb", dest='do_gdb', help="open the debugger for the specified file", action="store_true")
    parser.add_argument("--run-test", dest='run_test', help="run the test for SimpleInterval", action="store_true")
    parser.add_argument("--use-cxx", metavar='path', dest='use_cxx', help="use as c++ compiler when building the test")
    args = parser.parse_args()

    # If no files are specified, set it to all .c files in current directory
    files = args.file
    if args.run_test:
        if files:
            print('Error: you are trying to both run the test and a file. This does not really make sense.')
            sys.exit(4)
    elif args.do_gdb:
        if len(files) != 1:
            print('Error: you are trying to run the debugger on multiple files. This does not really make sense, just specify a single one.')
            sys.exit(4)
    elif args.do_make_only:
        args.do_make = True
        files = []
        args.run_test = False
    elif not files:
        files = [i for i in os.listdir(samples) if i.endswith('.c')]

    if args.do_make:
        print("Building %s..." % (make_target,))
        run([cmake, '--build', llvm_path, '--target', make_target])

    if not os.path.isfile(pass_lib):
        print('Error: Could not find shared library ' + pass_lib)
        print('Please build the project (for example by running this script with the option --make')
        sys.exit(7)
        
    os.makedirs('output', exist_ok=True)

    for fname in files:
        f_orig  = 'samples/%s' % (fname,)
        f_bc    = 'output/%s-tmp.bc' % (fname,)
        f_optbc = 'output/%s.bc' % (fname,)
        f_optll = 'output/%s.ll' % (fname,)
        f_out   = 'output/%s.out' % (fname,)

        if not os.path.isfile(f_orig):
            print("Error: " + f_orig +" not found!")
            continue
        print("Processing file " + fname + " ...")
    
        run([clang, '-O0', '-emit-llvm', f_orig, '-Xclang', '-disable-O0-optnone', '-c', '-o', f_bc])
        run([opt, '-mem2reg', f_bc, '-o', f_optbc])
        run([llvm_dis, f_optbc, '-o', f_optll])
        run(['rm', f_bc, f_optbc])

        redir = None if args.show_output else f_out
        args_add = []
        if args.view_cfg:
            args_add.append('--view-cfg')

        base_args = ['-load', pass_lib, '-'+pass_name, '-S'] + args_add + ['-o', '/dev/null', f_optll]
        if not args.do_gdb:
            run([opt] + base_args, redirect=redir)
        else:
            break_at = 'pcpo::AbstractInterpretationPass::runOnModule(llvm::Module&)'
            if not args.only_print:
                print('In a moment, gdb is going to read in the symbols of opt. As you might notice, that takes a long time. So, here is a tip: Just restart the program using r (no need to specify arguments). Even if you rebuild the project, that is in a shared library and will thus be reloaded the next time you start the program.')
            run([gdb, '-q',  opt, '-ex', 'r ' + ' '.join(map(shlex.quote, base_args))])

    if args.run_test:
        if not os.path.isfile(llvm_config):
            print('Error: no llvm-config exists at ' + llvm_config + ' (maybe you forgot to build LLVM?)')
            sys.exit(2)

        os.makedirs('build', exist_ok=True)

        cxx = CXX
        if args.use_cxx is not None:
            cxx = args.use_cxx
        
        print('Info: Building the test using %s' % (cxx,))

        cxxflags = subprocess.run([llvm_config, '--cxxflags'], stdout=subprocess.PIPE).stdout.decode('ascii').split()
        ldflags  = subprocess.run([llvm_config, '--ldflags' ], stdout=subprocess.PIPE).stdout.decode('ascii').split()
        libs     = subprocess.run([llvm_config, '--libs', 'analysis'], stdout=subprocess.PIPE).stdout.decode('ascii').split()
        if platform.system() == 'Darwin':
            libs += '-lz -ldl -lpthread -lm -lcurses'.split()
        else:
            libs += '-lz -lrt -ldl -ltinfo -lpthread -lm'.split()
        
        run([cxx, 'test/simple_interval_test.cpp', 'src/simple_interval.cpp', '-Isrc', '-fmax-errors=2'] + cxxflags
             + ['-o', 'build/SimpleIntervalTest'] + ldflags + libs)

        try:
            run(['build/SimpleIntervalTest'])
        except KeyboardInterrupt:
            pass
        
if __name__ == "__main__":
    main()
