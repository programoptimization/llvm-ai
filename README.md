# Program Optimization Lab 2018

Implements an LLVM analysis pass using abstract interpretation.

## Build

Get the LLVM source code from [here](http://releases.llvm.org/download.html). Then get clang as well, into `llvm/tools`. Create a build directory somewhere, initialise CMake, and build. For example

    # From your llvm-7.0.0-src, or whatever the version is now
    cd ..
    mkdir llvm_build
    cd llvm_build
    cmake ../llvm-?.?.?-src -DLLVM_TARGETS_TO_BUILD=X86
    make -j2
    
The parallel make may run out of memory at the end. You can restart it sequentially by issuing another `make`.

Now we can initalise the repository.

    cd ..
    git clone ssh://git@github.com/PUT/THE/CORRECT/REPOSITORY/IN/HERE.git
    cd PUT/THE/CORRECT/REPOSITORY/IN/HERE
    python3 init_repo.py
    
The script should be able to find your LLVM and clang. If it is not, you need to specify them by hand.

At last, let us compile and run the samples.

    python3 run.py --make
    
If there are errors regarding missing header files, you probably need to rebuild llvm.

## Useful things

The `run.py` script contains everything, up to and including the kitchen sink. It can run the samples, build, run the debugger, as well as build and run the tests. Just read its help message to get all the good stuff. I want to highlight the `-n` option, which causes it to just print out the commands it would run. This is great to just copy-paste the relevant ones into your terminal (or IDE).

## Authors

* Ramona Brückl
* Philipp Czerner ([github](https://github.com/suyjuris/), [mail](mailto:philipp.czerner@nicze.de))
* Tim Gymnich
* Thomas Frank

### Authors of previous semesters
* Julian Erhard
* Jakob Gottfriedsen
* Peter Munch
* Alexander Roschlaub
* Michael B. Schwarz

