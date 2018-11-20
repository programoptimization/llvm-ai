
#include "CallHierarchy.h"
#include "Hash.h"
#include "llvm/Support/CommandLine.h"
#include <algorithm>
#include <cassert>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

extern llvm::cl::opt<unsigned> CallStringDepth;

namespace pcpo {

CallHierarchy::CallHierarchy(llvm::Function *currentFunction,
                             CallInstructions callInsts, std::size_t offset)
    : currentFunction(currentFunction), callInsts(std::move(callInsts)),
      offset(offset) {
  /// Assert against bad offsets
  assert(offset <= this->callInsts.size());
}

std::size_t CallHierarchy::size() const {
  return std::distance(callInstructionsBegin(), callInstructionsEnd());
}

CallHierarchy CallHierarchy::push(llvm::CallInst *callInst) const {
  auto newCallInsts = callInsts;
  newCallInsts.push_back(callInst);

  auto newOffset =
      std::max(std::int64_t(0), std::int64_t(newCallInsts.size()) -
                                    std::int64_t(callStringDepth()));

  auto nextCurrentFunction = callInst->getCalledFunction();
  return CallHierarchy(nextCurrentFunction, std::move(newCallInsts), newOffset);
}

CallHierarchy CallHierarchy::pop(std::size_t frame_count) const {
  assert((callStringDepth() > 0U) &&
         "CallHierarchy::pop is only allowed to be called in a non "
         "zero call string depth scenario!");
  assert(frame_count <= size());

  auto end = callInsts.end();
  std::advance(end, -frame_count);
  CallInstructions newCallInsts(callInsts.begin(), end);

  std::size_t newOffset = std::max(
      std::int64_t(0), std::int64_t(offset) - std::int64_t(frame_count));

  /// callInsts must not be empty
  auto prevCurrentFunction = callInsts.back()->getFunction();
  return CallHierarchy(prevCurrentFunction, std::move(newCallInsts), newOffset);
}

llvm::Function *CallHierarchy::getCurrentFunction() const {
  return currentFunction;
}

bool CallHierarchy::operator==(CallHierarchy const &other) const {
  /// The last functions in the hierarchies do not match
  if (currentFunction != other.currentFunction) {
    return false;
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

size_t CallHierarchy::callStringDepth() { return CallStringDepth; }

bool CallHierarchy::operator<(CallHierarchy const &other) const {
  llvm::SmallString<128> left;
  {
    llvm::raw_svector_ostream out(left);
    print(out);
  }
  llvm::SmallString<128> right;
  {
    llvm::raw_svector_ostream out(right);
    other.print(out);
  }

  assert((!(left < right) && !(right < left)) == (*this == other));
  return left < right;
}

template <typename T> struct CompareByAddress {
  T child;
  template <typename Other> bool operator()(Other &&other) const {
    // Compare by addresses
    return &other == child;
  }
};

/// Returns the ordinal index of a llvm IR node which represents
/// the index in its parent node such that node->getParent()->begin() + index
/// represents the child node passed to this function.
template <typename T> std::size_t indexOfChildInParent(T const *child) {
  auto const parent = child->getParent();
  assert(parent);
  auto const pos = std::find_if(parent->begin(), parent->end(),
                                CompareByAddress<T const *>{child});
  assert(pos != parent->end());

  return std::distance(parent->begin(), pos);
}

void CallHierarchy::print(llvm::raw_ostream &os) const {
  auto instructions = llvm::make_range(callInstructionsBegin(), //
                                       callInstructionsEnd());
  for (auto &&callSite : instructions) {
    std::size_t indexBBInFunction = indexOfChildInParent(callSite->getParent());
    std::size_t indexInstInBB = indexOfChildInParent(callSite);

    llvm::StringRef const callerFunctionName =
        callSite->getFunction()->getName();

    os << callerFunctionName << "/" << indexBBInFunction << ":"
       << indexInstInBB << "/";
  }

  os << getCurrentFunction()->getName();
}
} // namespace pcpo

namespace llvm {
raw_ostream &operator<<(raw_ostream &os, pcpo::CallHierarchy const &hierarchy) {
  hierarchy.print(os);
  return os;
}
} // namespace llvm

size_t std::hash<pcpo::CallHierarchy>::
operator()(pcpo::CallHierarchy const &hierarchy) const {
  std::size_t hash = 0;
  pcpo::HashRangeCombine(hash, hierarchy.callInstructionsBegin(),
                         hierarchy.callInstructionsEnd());
  pcpo::HashCombine(hash, hierarchy.getCurrentFunction());
  return hash;
}
