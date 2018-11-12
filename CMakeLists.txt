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

add_llvm_loadable_module( llvm-vsa
  src/util/util.h
  src/vsa.cpp
  src/vsa.h
  src/abstract_domain/AbstractDomain.h
  src/abstract_domain/BoundedSet.cpp
  src/abstract_domain/BoundedSet.h
  src/abstract_domain/StridedInterval.cpp
  src/abstract_domain/StridedInterval.h
  src/abstract_domain/CompositeDomain.cpp
  src/abstract_domain/CompositeDomain.h
  src/abstract_domain/Util.h
  src/abstract_domain/SimpleInterval.cpp
  src/fixpoint/state.cpp
  src/fixpoint/state.h
  src/fixpoint/vsa_visitor.cpp
  src/fixpoint/vsa_visitor.h
  src/fixpoint/worklist.cpp
  src/fixpoint/worklist.h
  src/fixpoint/branch_conditions.cpp
  src/fixpoint/branch_conditions.h
  src/api/vsa_result.cpp
  src/api/vsa_result.h
  src/api/vsa_result_value.cpp
  src/api/vsa_result_value.h
  test/BoundedSetTest.cpp
  test/StridedIntervalTest.cpp
  test/CompositeDomainTest.cpp
  test/TestRunner.cpp
  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
