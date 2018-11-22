//
// Created by Dominik Horn on 19.11.18.
//

#include <llvm/Pass.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Analysis/CFGPrinter.h>
//#include <llvm/Support/GraphWriter.h>
#include "DotPrinter.h"
#include "CustomGraphWriter.h"
#include "src/util.h"
#include "src/BlockManager.h"

using namespace llvm;

namespace bra {
    void DotPrinter::writeCFGToDotFile(Function &F, BlockManager* bmgr) {
        std::string Filename = ("cfg." + F.getName() + ".dot").str();
        errs() << "Writing '" << Filename << "'...";

        std::error_code EC;
        raw_fd_ostream File(Filename, EC, sys::fs::F_Text);

        if (!EC) {
            printGraph(File, (const Function *) &F, bmgr, false, "");
        } else {
            errs() << "  error opening file for writing!";
        }
        errs() << "\n";
    }

    template<typename GraphType>
    raw_ostream &DotPrinter::printGraph(raw_ostream &O, const GraphType &G, BlockManager* bmgr, bool shortNames, const Twine &title) {
        // Start the graph emission process...
        CustomGraphWriter<GraphType> W(O, G, bmgr, shortNames);

        // Emit the graph.
        W.writeGraph(title.str());

        return O;
    }

    void DotPrinter::print(Function &F, BlockManager* bmgr) {
        writeCFGToDotFile(F, bmgr);
    }
}


