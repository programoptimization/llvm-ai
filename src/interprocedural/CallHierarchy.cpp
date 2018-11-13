
#include "CallHierarchy.h"
#include "Hash.h"

static const size_t CSA_DEPTH = 1;

namespace pcpo {
CallHierarchy CallHierarchy::append(llvm::CallInst *callInst) const {
  auto newCallInsts = callInsts;
  newCallInsts.push_back(callInst);
  if (newCallInsts.size() > CSA_DEPTH) {
    newCallInsts.pop_front();
  }
  return CallHierarchy(mainFunction, newCallInsts);
}

llvm::Function *CallHierarchy::getCurrentFunction() const {
  if (callInsts.empty()) {
    // If we are in the main function return it
    return mainFunction;
  } else {
    // Otherwise return the current function we are inside
    return callInsts.back()->getCalledFunction();
  }
}

bool CallHierarchy::operator==(CallHierarchy const &other) const {
  return (mainFunction == other.mainFunction) && (callInsts == other.callInsts);
}

llvm::CallInst *CallHierarchy::getLastCallInstruction() const {
  if (callInsts.empty()) {
    return nullptr;
  }

  return callInsts.back();
}
} // namespace pcpo

size_t std::hash<pcpo::CallHierarchy>::
operator()(pcpo::CallHierarchy const& hierarchy) const {
  return 0; //  pcpo::multiHash(hierarchy.mainFunction, hierarchy.callInsts);
}
