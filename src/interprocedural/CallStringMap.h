#ifndef LLVM_CALLSTRINGMAP_H
#define LLVM_CALLSTRINGMAP_H

#include "CallHierarchy.h"
#include "CallStringArguments.h"
#include <map>

namespace pcpo {

class CallStringMap {

  std::map<CallHierarchy, CallStringArguments> argsByHierarchy;

public:
  CallStringMap() = default;
};

} // namespace pcpo

#endif // LLVM_CALLSTRINGMAP_H
