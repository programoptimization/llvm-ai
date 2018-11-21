#!/usr/bin/python3

import argparse
import os

parser = argparse.ArgumentParser(description='Setup the project.')
parser.add_argument('--llvm-path', help='path to the llvm build directory')
parser.add_argument('--llvm-src', help='path to the llvm source directory')
parser.add_argument('--clang-path', help='path to the clang build direcotry')
args = parser.parse_args()
llvm_path = args.llvm_path
llvm_src = args.llvm_src
clang_path = args.clang_path

destination_path = llvm_src+ 'lib/Analysis/'
project_directory = os.getcwd()
project_name = os.path.basename(project_directory)

# symlink
try:
    os.symlink(project_directory, destination_path+project_name, target_is_directory=True)
    print('Created symlink')
except FileExistsError:
    print("Symlink already exists")
    print('Symlink already exists')
    pass

# write config
config_file = open('.config', 'w')
config_file.write(""+llvm_src+"\n"+llvm_path+"\n"+clang_path+"\n")
config_file.close()
print("Created config")

# cmake anpassen
cmake_file = open(destination_path+'CMakeLists.txt', 'r')
lines = cmake_file.readlines()
needs_changes = 'add_subdirectory(%s)\n'%(project_name,) in lines
cmake_file.close()
if needs_changes:
    cmake_file = open(destination_path+'CMakeLists.txt', 'a')
    cmake_file.write('add_subdirectory(%s)\n'%(project_name,))
    print("CMakeLists.txt modified")
else:
    print("CMakeLists.txt already modified")
cmake_file.close()

