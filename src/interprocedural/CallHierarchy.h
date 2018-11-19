#ifndef CALL_HIERARCHY_H_
#define CALL_HIERARCHY_H_

#include <cassert>
#include <utility>
#include <vector>

namespace llvm {
class Function;
class CallInst;
class raw_ostream;
} // namespace llvm

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
    assert(offset <= this->callInsts.size());
  }

  /// Returns true when we are currently iside the main function
  bool isInMainFunction() const;

  /// Returns the height of the current hierarchy
  std::size_t size() const;

  /// Pushes the CallInstruction to the current call hierarchy
  CallHierarchy push(llvm::CallInst *callInst) const;

  /// Pops frame_count frames from the hierarchy
  CallHierarchy pop(std::size_t frame_count = 1U) const;

  /// Gets the current top function of the hierarchy we are
  /// currently inside in.
  llvm::Function *getCurrentFunction() const;

  /// Returns nullptr when in main function.
  llvm::CallInst *getLastCallInstruction() const;

  bool operator==(CallHierarchy const &other) const;

  static size_t callStringDepth();

  /// Compares this call hierarchy to another lexicographically
  bool operator<(CallHierarchy const &other) const;
  /// Prints this call hierarchy to the given stream
  void print(llvm::raw_ostream &os) const;

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

namespace llvm {
raw_ostream &operator<<(raw_ostream &os, pcpo::CallHierarchy const &hierarchy);
} // namespace llvm

namespace std {
template <> struct hash<pcpo::CallHierarchy> {
  size_t operator()(pcpo::CallHierarchy const &hierarchy) const;
};
} // namespace std

#endif // CALL_HIERARCHY_H_
