#ifndef CALL_HIERARCHY_H_
#define CALL_HIERARCHY_H_

#include <cassert>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <utility>
#include <vector>

namespace pcpo {
class CallHierarchy {
  friend std::hash<CallHierarchy>;

  using CallInstructions = std::vector<llvm::CallInst *>;

public:
  explicit CallHierarchy(llvm::Function *mainFunction,
                         CallInstructions callInsts = {},
                         std::size_t offset = 0U)
      : mainFunction(mainFunction), callInsts(std::move(callInsts)),
        offset(offset) {
    /// Assert against bad offsets
    assert(offset <= callInsts.size());
  }

  /// Returns true when we are currently iside the main function
  bool isInMainFunction() const;

  /// Appends the CallInstruction to the current call hierarchy
  CallHierarchy append(llvm::CallInst *callInst) const;

  /// Gets the current top function of the hierarchy we are
  /// currently inside in.
  llvm::Function *getCurrentFunction() const;

  /// Returns nullptr when in main function.
  llvm::CallInst *getLastCallInstruction() const;

  bool operator==(CallHierarchy const &other) const;

private:
  CallInstructions::const_iterator callInstructionsBegin() const;
  CallInstructions::const_iterator callInstructionsEnd() const;

  /// In case the call hierarchy empty we still require to return the
  /// current function we are inside and this will be the main function.
  llvm::Function *mainFunction;
  /// A list of all call instructions inside the hierarchy
  CallInstructions callInsts;
  /// The offset the call hierarchy starts when we limited it
  /// to a certain depth. The depth to the end of the callInst vector will
  /// be taken into account when doing hashing and element comparison.
  std::size_t offset;
};
} // namespace pcpo

namespace std {
template <> struct hash<pcpo::CallHierarchy> {
  size_t operator()(pcpo::CallHierarchy const &hierarchy) const;
};
} // namespace std

#endif // CALL_HIERARCHY_H_
