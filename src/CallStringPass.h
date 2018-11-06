
#ifndef CALL_STRING_PASS_H_
#define CALL_STRING_PASS_H_

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <utility>
#include <vector>

using CallSites = std::vector<llvm::CallInst *>;

struct ArgumentSet {};

struct CallString {
  CallSites sites_;
  ArgumentSet set_;
};

struct ReturnSet {};

using WhatWeWant = std::map<CallString, ReturnSet>;

template <typename SubVisitor>
class CallStringVisitorTraits;

template <typename SubVisitor>
class CallStringVisitor
    : public llvm::InstVisitor<CallStringVisitor<SubVisitor>, void> {

  static_assert(
      std::is_base_of<llvm::InstVisitor<SubVisitor, void>, SubVisitor>::value,
      "The Visitor needs to inherit from llvm::InstVisitor!");

public:
  explicit CallStringVisitor(SubVisitor sub_visitor)
      : sub_visitor_(std::move(sub_visitor)) {}

  void visitCallInst(llvm::CallInst &I) {
    // Implement this later

    for (auto&& arg : llvm::make_range(I.arg_begin(), I.arg_end()))
    {
      // arg.get()->
    }
  }

  // TODO Implement this later when exceptions are supported
  // void visitInvokeInst(llvm::InvokeInst &I);

  /// default
  void visitInstruction(llvm::Instruction &I) { sub_visitor_.visit(I); }

private:
  SubVisitor sub_visitor_;
};

class CallStringPass : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

public:
  CallStringPass() : llvm::ModulePass(ID) {}

  bool doInitialization(llvm::Module &m) override {
    return llvm::ModulePass::doInitialization(m);
  }

  bool doFinalization(llvm::Module &m) override {
    return llvm::ModulePass::doFinalization(m);
  }

  bool runOnModule(llvm::Module &function) override;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  // VsaResult &getResult() { return result; }
};

#endif // CALL_STRING_PASS_H_
