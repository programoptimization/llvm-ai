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
  explicit CallHierarchy(std::deque<llvm::CallInst *> callInsts)
      : callInsts(std::move(callInsts)) {}

  CallHierarchy append(llvm::CallInst *callInst) const {
    auto newCallInsts = callInsts;
    newCallInsts.push_back(callInst);
    if (newCallInsts.size() > CSA_DEPTH) {
      newCallInsts.pop_front();
    }
    return CallHierarchy(newCallInsts);
  }

private:
  std::deque<llvm::CallInst *> callInsts;
};

} // namespace pcpo

#endif // CALL_HIERARCHY_H_
