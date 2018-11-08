
#ifndef CALL_HIERARCHY_H_
#define CALL_HIERARCHY_H_

#include <cassert>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <queue>

template <typename LocalData> class CallHierarchy {
  struct StackFrame {
    llvm::Function &function;
    llvm::CallSite call_site;
    LocalData local_data;
  };

  std::deque<StackFrame> frames_;

public:
  explicit CallHierarchy(llvm::Function &main_function, LocalData local_data) {
    // Push our main function on top of the hierarchy
    frames_.push_back({main_function, {}, std::move(local_data)});
  }

  void push(llvm::Function &function_, llvm::CallSite call_site,
            LocalData local_data) {
    frames_.push_back({function_, call_site, std::move(local_data)});
  }

  void pop() { frames_.pop_back(); }

  llvm::Function &currentFunction() const { return current().function; }
  llvm::CallSite currentCallSite() const { return current().call_site; }
  LocalData &currentData() { return current().data; }
  LocalData &currentData() const { return current().data; }

private:
  StackFrame &current() {
    assert(!frames_.empty());
    return frames_.back();
  }
  StackFrame const &current() const {
    assert(!frames_.empty());
    return frames_.back();
  }
};

#endif // CALL_HIERARCHY_H_
