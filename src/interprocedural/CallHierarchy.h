#ifndef CALL_HIERARCHY_H_
#define CALL_HIERARCHY_H_

#include <cassert>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <queue>
#include <utility>

namespace pcpo {
class CallHierarchy {
  friend std::hash<CallHierarchy>;

public:
  explicit CallHierarchy(llvm::Function *mainFunction,
                         std::deque<llvm::CallInst *> callInsts = {})
      : mainFunction(mainFunction), callInsts(std::move(callInsts)) {}

  /// Appends the CallInstruction to the current call hierarchy
  CallHierarchy append(llvm::CallInst *callInst) const;

  /// Gets the current top function of the hierarchy we are
  /// currently inside in.
  llvm::Function *getCurrentFunction() const;

  bool operator==(CallHierarchy const &other) const;

private:
  llvm::Function *mainFunction;
  std::vector<llvm::CallInst *> callInsts;
};
} // namespace pcpo

namespace std {
template <> struct hash<pcpo::CallHierarchy> {
  size_t operator()(pcpo::CallHierarchy const &hierarchy) const;
};
} // namespace std

#endif // CALL_HIERARCHY_H_
