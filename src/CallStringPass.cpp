
#include "CallStringPass.h"

bool CallStringPass::runOnModule(llvm::Module &function)
{
    // Our analysis does not change the IR
    return false;
}

void CallStringPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {}

char CallStringPass::ID = 0;

#ifndef VSA_STATIC
static RegisterPass<CallStringPass> Y("csapass",
                               "CSA Pass (with getAnalysisUsage implemented)");
#endif // VSA_STATIC
