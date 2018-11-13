
#include "CallHierarchy.h"
#include "Hash.h"
#include <algorithm>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

static std::size_t const callStringDepth = 1;

namespace pcpo {
bool CallHierarchy::isInMainFunction() const { return callInsts.empty(); }

std::size_t CallHierarchy::size() const {
  return std::distance(callInstructionsBegin(), callInstructionsEnd());
}

CallHierarchy CallHierarchy::push(llvm::CallInst *callInst) const {
  auto newCallInsts = callInsts;
  newCallInsts.push_back(callInst);

  auto newOffset = std::max(std::int64_t(0), std::int64_t(newCallInsts.size()) -
                                                 std::int64_t(callStringDepth));

  return CallHierarchy(mainFunction, std::move(newCallInsts), newOffset);
}

CallHierarchy CallHierarchy::pop(std::size_t frame_count) const {
  assert(frame_count <= size());

  auto end = callInsts.end();
  std::advance(end, -frame_count);
  CallInstructions newCallInsts(callInsts.begin(), end);

  std::size_t newOffset = std::max(
      std::int64_t(0), std::int64_t(offset) - std::int64_t(frame_count));

  return CallHierarchy(mainFunction, std::move(newCallInsts), newOffset);
}

llvm::Function *CallHierarchy::getCurrentFunction() const {
  if (isInMainFunction()) {
    // If we are in the main function return it
    return mainFunction;
  } else {
    // Otherwise return the current function we are inside
    return callInsts.back()->getCalledFunction();
  }
}

bool CallHierarchy::operator==(CallHierarchy const &other) const {
  if (isInMainFunction() && other.isInMainFunction()) {
    // If the call hierarchies are empty just compare the main functions
    return mainFunction == other.mainFunction;
  }

  /// The hierarchy depth of both hierarchies don't match
  if (size() != other.size()) {
    return false;
  }

  /// Check whether both hierarchies match
  return std::equal(callInstructionsBegin(), callInstructionsEnd(),
                    other.callInstructionsBegin());
}

CallHierarchy::CallInstructions::const_iterator
CallHierarchy::callInstructionsBegin() const {
  if (offset == 0) {
    return callInsts.begin();
  } else {
    // Move the iterator to the current offset
    auto itr = callInsts.begin();
    std::advance(itr, offset);
    return itr;
  }
}

CallHierarchy::CallInstructions::const_iterator
CallHierarchy::callInstructionsEnd() const {
  return callInsts.end();
}

llvm::CallInst *CallHierarchy::getLastCallInstruction() const {
  if (callInsts.empty()) {
    return nullptr;
  }

  return callInsts.back();
}
} // namespace pcpo

size_t std::hash<pcpo::CallHierarchy>::
operator()(pcpo::CallHierarchy const &hierarchy) const {
  if (hierarchy.isInMainFunction()) {
    // if we are in the main function just return its hash
    return std::hash<void *>{}(hierarchy.mainFunction);
  } else {
    return pcpo::HashRange(hierarchy.callInstructionsBegin(),
                           hierarchy.callInstructionsEnd());
  }
}
