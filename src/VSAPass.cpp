
#include "VSAPass.h"
#include "vsa.h"
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>

#ifndef VSA_STATIC
#error Only compile this source file when building VSA statically into LLVM.
#endif // VSA_STATIC

namespace llvm {
void initializeVsaPassPass(llvm::PassRegistry &Registry);

void initializeValueSetAnalysis(llvm::PassRegistry &Registry) {
  initializeVsaPassPass(Registry);
}
} // namespace llvm

INITIALIZE_PASS(VsaPass, "vsapass", "VSA Pass with Call-String analysis", false,
                true)

llvm::Pass *createValueSetAnalysisPass() { return new VsaPass(); }
