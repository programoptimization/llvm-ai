set(LLVM_LINK_COMPONENTS
        Analysis
        AsmParser
        Core
        Support
        )

add_llvm_unittest(RelationalAnalysisTests
        EqualityDomainTest.cpp
        ../src/domains/EqualityDomain.cpp
        ../src/common/Variable.cpp
        ../src/common/Constant.cpp
        ../src/common/Representative.cpp
        ../src/common/RepresentativeHasher.cpp
        )