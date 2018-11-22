#!/usr/bin/python3
# coding: utf-8

import argparse
import os
import sys

if sys.version_info[0] < 3:
    print("Error: This script only supports Python 3")
    sys.exit(5)

parser = argparse.ArgumentParser(description='Setup the project. This creates the necessary symbolic links in the LLVM source code and adds the entries into the right CMakeList.txt. Also initialises the configuration for the run.py script.')
parser.add_argument('--llvm-path', help='path to the LLVM build directory, containing a file bin/opt.')
parser.add_argument('--llvm-src', help='path to the LLVM source directory, containing lib/Analysis ')
parser.add_argument('--clang-path', help='path to the clang build direcotry, containing a file bin/clang')
args = parser.parse_args()

llvm_path = args.llvm_path
llvm_src = args.llvm_src
clang_path = args.clang_path

project_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
project_name = 'AbstractInterpretation'

if llvm_path is None:
    # Try to guess the correct path
    d = os.path.abspath(project_dir)
    while os.path.dirname(d) != d:
        d = os.path.dirname(d)
        for i in os.listdir(d):
            if os.path.isfile(d + '/' + i + '/bin/opt'):
                llvm_path = d + '/' + i
                print('Auto-detecting llvm-path as ' + llvm_path)
                break
        if llvm_path is not None: break
    else:
        print('Error: No llvm-path specified (use --llvm-path)')
        parser.print_help()
        sys.exit(1)
        
if llvm_src is None:
    # Try to guess the correct path
    d = os.path.abspath(project_dir)
    while os.path.dirname(d) != d:
        d = os.path.dirname(d)
        for i in os.listdir(d):
            if os.path.isfile(d + '/' + i + '/cmake_install.cmake'):
                continue
            if os.path.isdir(d + '/' + i + '/lib/Analysis'):
                llvm_src = d + '/' + i
                print('Auto-detecting llvm-src as ' + llvm_src)
                break
        if llvm_src is not None: break
    else:
        print('Error: No llvm-src specified (use --llvm-src)')
        parser.print_help()
        sys.exit(1)

if clang_path is None:
    clang_path = llvm_path
    print('clang-path not specified, defaulting to ' + clang_path)

opt = llvm_path + '/bin/opt'
llvm_dest = llvm_src + '/lib/Analysis'
clang = clang_path + '/bin/clang'

if not os.path.isfile(opt):
    print('Error: no opt exists at ' + opt + ' (maybe you forgot to build LLVM?)')
    sys.exit(2)
if not os.path.isdir(llvm_dest):
    print('Error: directory does not exist, at ' + llvm_dest)
    sys.exit(2)
if not os.path.isfile(clang):
    print('Error: no clang exists at ' + clang)
    sys.exit(2)

# Create the symbolic link in the LLVM sources
try:
    link_name = llvm_dest + '/' + project_name
    os.symlink(project_dir, link_name, target_is_directory=True)
    print('Created symbolic link from %s to %s' % (link_name, project_dir))
except FileExistsError:
    print('Symlink already exists')

# Write the configuration for the run.py script
config_file_name = project_dir + '/.config';
config_file = open(config_file_name, 'w')
config_file.write(llvm_src+'\n'+llvm_path+'\n'+clang_path+'\n')
config_file.close()
print('Wrote configuration to %s' % (config_file_name,))

# Adjust CMakeLists.txt
cmake_file_name = llvm_dest + '/CMakeLists.txt'
line_to_insert = 'add_subdirectory(%s)\n' % (project_name,)
cmake_file = open(cmake_file_name, 'r')
lines = cmake_file.readlines()       
needs_changes = line_to_insert not in lines
cmake_file.close()
if needs_changes:
    cmake_file = open(cmake_file_name, 'a')
    cmake_file.write('\n' + line_to_insert)
    cmake_file.close()
    print('CMakeLists.txt modified, at %s' % (cmake_file_name,))
else:
    print('CMakeLists.txt is fine')

