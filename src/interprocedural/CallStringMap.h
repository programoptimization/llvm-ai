#ifndef LLVM_CALLSTRINGMAP_H
#define LLVM_CALLSTRINGMAP_H

#include "../abstract_domain/AbstractDomain.h"
#include "CallHierarchy.h"
#include "CallStringArguments.h"
#include <map>

namespace pcpo {

class CallStringMap {

  std::map<CallHierarchy, CallStringArguments> argsByHierarchy;

public:
  CallStringMap() = default;

  std::shared_ptr<AbstractDomain> getReturnDomain(CallStringArguments args) {
    // Handle deadloops here somehow
    return nullptr;
  }
};

} // namespace pcpo

#endif // LLVM_CALLSTRINGMAP_H
