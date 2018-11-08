# If we don't need RTTI or EH, there's no reason to export anything
# from the hello plugin.
#if( NOT LLVM_REQUIRES_RTTI )
#  if( NOT LLVM_REQUIRES_EH )
#    set(LLVM_EXPORTED_SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Hello.exports)
#  endif()
#endif()

if(WIN32 OR CYGWIN)
  set(LLVM_LINK_COMPONENTS Core Support)
endif()

set(VSA_SOURCES
src/ReturnDomainJoin.h
src/ReturnDomainJoin.cpp
src/CallStringPass.h
src/CallStringPass.cpp
src/vsa.h
src/vsa.cpp
src/abstract_domain/BoundedSet.h
src/abstract_domain/StridedInterval.h
src/abstract_domain/CompositeDomain.h
src/abstract_domain/BoundedSet.cpp
src/abstract_domain/StridedInterval.cpp
src/abstract_domain/CompositeDomain.cpp
src/fixpoint/state.h
src/fixpoint/state.cpp
src/fixpoint/vsa_visitor.h
src/fixpoint/vsa_visitor.cpp
src/fixpoint/worklist.h
src/fixpoint/branch_conditions.h
src/fixpoint/worklist.cpp
src/fixpoint/branch_conditions.cpp
src/api/vsa_result.h
src/api/vsa_result.cpp
src/api/vsa_result_value.h
src/api/vsa_result_value.cpp
test/BoundedSetTest.cpp
test/StridedIntervalTest.cpp
test/CompositeDomainTest.cpp
test/TestRunner.cpp
)

if (NOT VSA_STATIC)
add_llvm_loadable_module( llvm-vsa
  ${VSA_SOURCES}
  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
else()
add_llvm_library(LLVMValueSetAnalysis
  ${VSA_SOURCES}
  src/VSAPass.cpp
  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms/ValueSetAnalysis/src
  DEPENDS
  intrinsics_gen
  )

target_compile_definitions(LLVMValueSetAnalysis PRIVATE VSA_STATIC)
endif()
