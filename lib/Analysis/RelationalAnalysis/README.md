# Program Optimization Lab 2018

The aim of the Program Optimization Lab 2018 is to explore the LLVM opt tool and 
extend it with relational analysis.

# Authors
tbd

# Installation
Clone the project into the following folder:
```
cd llvm/lib/Analysis
git clone https://github.com/fogshot/relational-analysis.git RelationalAnalysis
```

and add the following line to `llvm/lib/Analysis/CMakeLists.txt`:
```
add_subdirectory(RelationalAnalysis)
```