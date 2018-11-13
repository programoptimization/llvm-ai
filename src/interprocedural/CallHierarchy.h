#ifndef CALL_HIERARCHY_H_
#define CALL_HIERARCHY_H_

#include <cassert>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <queue>

static const size_t CSA_DEPTH = 1;

namespace pcpo {
class CallHierarchy {
public:
  explicit CallHierarchy(llvm::Function *mainFunction,
                         std::deque<llvm::CallInst *> callInsts = {})
      : mainFunction(mainFunction), callInsts(std::move(callInsts)) {}

  CallHierarchy append(llvm::CallInst *callInst) const {
    auto newCallInsts = callInsts;
    newCallInsts.push_back(callInst);
    if (newCallInsts.size() > CSA_DEPTH) {
      newCallInsts.pop_front();
    }
    return CallHierarchy(mainFunction, newCallInsts);
  }

  llvm::Function *getCurrentFunction() const {
    if (callInsts.empty()) {
      // If we are in the main function return it
      return mainFunction;
    } else {
      // Otherwise return the current function we are inside
      return callInsts.back()->getCalledFunction();
    }
  }

  bool operator==(CallHierarchy const &other) const {
    return (mainFunction == other.mainFunction) &&
           (callInsts == other.callInsts);
  }

private:
  llvm::Function *mainFunction;
  std::deque<llvm::CallInst *> callInsts;
};

} // namespace pcpo

#endif // CALL_HIERARCHY_H_
