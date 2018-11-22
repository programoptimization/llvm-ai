add_llvm_loadable_module(llvm-ra
        src/BasicRelationalAnalysisPass.cpp
        src/BlockManager.cpp
        src/globals.cpp
        src/dotprinter/DotPrinter.cpp
        src/domains/EqualityDomain.cpp
        src/common/InstructionVisitor.cpp
        src/common/Representative.cpp
        src/common/RepresentativeHasher.cpp
        src/common/Constant.cpp
        src/common/Variable.cpp
        src/common/State.cpp
        src/common/BbWorklist.cpp
        src/common/Invariant.cpp

        PLUGIN_TOOL
        opt
        )

add_custom_target(RaTests)
set_target_properties(RaTests PROPERTIES FOLDER "Tests")

function(add_llvm_unittest test_dirname)
    add_unittest(RaTests ${test_dirname} ${ARGN})
endfunction()

add_subdirectory(test)