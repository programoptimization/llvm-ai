
#ifndef CALL_STRING_PASS_H_
#define CALL_STRING_PASS_H_

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

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
