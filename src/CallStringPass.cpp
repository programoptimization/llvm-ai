
#include "CallStringPass.h"
#include "fixpoint/vsa_visitor.h"
#include "vsa.h"
#include "fixpoint/vsa_visitor.h"
#include <llvm/IR/Module.h>

template <>
class CallStringVisitorTraits<VsaVisitor>
{
  void inject(VsaVisitor& visitor);
};

bool CallStringPass::runOnModule(llvm::Module &module) {
  for (auto &&functions : module) {
    // ..
  }

  // Our analysis does not change the IR
  return false;
}

void CallStringPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {}

char CallStringPass::ID = 0;

#ifndef VSA_STATIC
static RegisterPass<CallStringPass>
    Y("csapass", "CSA Pass (with getAnalysisUsage implemented)");
#endif // VSA_STATIC
