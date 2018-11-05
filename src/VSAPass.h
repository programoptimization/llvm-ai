
#ifndef VSA_PASS_H_
#define VSA_PASS_H_

namespace llvm {
class Pass;
}

llvm::Pass *createValueSetAnalysisPass();

#endif // VSA_PASS_H_
