
#include "VSAPass.h"
#include "CallStringPass.h"
#include "vsa.h"
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>

#ifndef VSA_STATIC
#error Only compile this source file when building VSA statically into LLVM.
#endif // VSA_STATIC

namespace llvm {
void initializeVsaPassPass(llvm::PassRegistry &Registry);
void initializeCallStringPassPass(llvm::PassRegistry &Registry);

void initializeValueSetAnalysis(llvm::PassRegistry &Registry) {
  initializeVsaPassPass(Registry);
  initializeCallStringPassPass(Registry);
}
} // namespace llvm

INITIALIZE_PASS(VsaPass, "vsapass",
                "Lower VSA Pass (with getAnalysisUsage implemented)", false,
                true)
INITIALIZE_PASS(CallStringPass, "csapass",
                "CSA Pass (with getAnalysisUsage implemented)", false, true)

llvm::Pass *createValueSetAnalysisPass() { return new VsaPass(); }
llvm::Pass *createCallStringPass() { return new CallStringPass(); }
