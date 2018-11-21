#!/usr/bin/python3

import argparse
import os

parser = argparse.ArgumentParser(description='Setup the project.')
parser.add_argument('--llvm-path', help='path to the llvm source directory')
parser.add_argument('--clang-path', help='path to the clang build direcotry')
args = parser.parse_args()
llvm_path = args.llvm_path
clang_path = args.clang_path

destination_path = llvm_path+ 'lib/Analysis/'
project_directory = os.getcwd()
project_name = os.path.basename(project_directory)

# symlink
try:
    os.symlink(project_directory, destination_path+project_name, target_is_directory=True)
except FileExistsError:
    pass

# write config
config_file = open('.config', 'w')
config_file.write(""+llvm_path+"\n"+clang_path+"\n")
config_file.close()

# cmake anpassen
cmake_file = open(destination_path+'CMakeLists.txt', 'r')
lines = cmake_file.readlines()
needs_changes = 'add_subdirectory(%s)\n'%(project_name,) in lines
cmake_file.close()
if needs_changes:
    cmake_file = open(destination_path+'CMakeLists.txt', 'a')
    cmake_file.write('add_subdirectory(%s)\n'%(project_name,))
cmake_file.close()

