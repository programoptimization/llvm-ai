//===----------------------------------------------------------------------===//
//
// This file implements the entry point for our Basic Relational Analysis Pass,
// which is capable of finding equality relations between multiple variables
// and/or constants.
//
//===----------------------------------------------------------------------===//

#include <string>
#include <sstream>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include "util.h"
#include "src/BlockManager.h"
#include "domains/EqualityDomain.h"
#include "common/Representative.h"
#include "common/Variable.h"
#include "common/Constant.h"
#include "dotprinter/DotPrinter.h"

using namespace llvm;
using namespace std;

namespace bra {
    struct BasicRelationalAnalysisPass : public FunctionPass {
        static char ID; // Pass identification, replacement for typeid
        BasicRelationalAnalysisPass() : FunctionPass(ID) {}

    private:
        typedef FunctionPass super;
    public:
        bool runOnFunction(Function &F) override {
            // Obtain a block manager
            BlockManager blockManager;

            // Analyse current function
            DEBUG_OUTPUT(std::string(PURPLE)
                         +"Function \"" + F.getName().str() + "\"" + std::string(NO_COLOR));
            globalDebugOutputTabLevel++;
            blockManager.analyse(F);
            globalDebugOutputTabLevel--;

            // Print to .dot file
            DotPrinter p;
            p.print(F, &blockManager);

            // This is an analysis pass and never modifies any code
            return false;
        }

        void getAnalysisUsage(AnalysisUsage &AU) const override {
            // This Pass is pure analysis => No modification => Preserves all
            AU.setPreservesAll();
        }

        void releaseMemory() override {
            // TODO: release Memory once applicable (analysis result)
        }
    };

    char BasicRelationalAnalysisPass::ID = 0;
    /// cmd-option-name, description (--help), onlyCFG, analysisPass
    static RegisterPass<BasicRelationalAnalysisPass> X("basicra", "Basic Relational Analysis Pass", false, true);
}
