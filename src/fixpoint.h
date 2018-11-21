#pragma once

#include "llvm/Pass.h"

namespace pcpo {

class AbstractInterpretationPass: public llvm::ModulePass {
public:
    AbstractInterpretationPass(): llvm::ModulePass{ID} {}
    
    static char ID; // Pass identification, replacement for typeid
    
    virtual bool runOnModule(llvm::Module& M);
    
    virtual void getAnalysisUsage(llvm::AnalysisUsage &Info) const;
};

} /* end of namespace pcpo */
