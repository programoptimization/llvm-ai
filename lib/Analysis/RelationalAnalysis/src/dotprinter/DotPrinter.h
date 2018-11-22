//
// Created by Dominik Horn on 19.11.18.
//

#ifndef LLVM_DOTPRINTER_H
#define LLVM_DOTPRINTER_H

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/Twine.h>
#include "src/BlockManager.h"

using namespace llvm;

namespace bra {
    class DotPrinter {
    public:
        // TODO: tmp
        void print(llvm::Function &, BlockManager*);

        void writeCFGToDotFile(llvm::Function &, BlockManager*);

        template<typename GraphType>
        raw_ostream &printGraph(raw_ostream&, const GraphType&, BlockManager*, bool, const Twine&);
    };
}


#endif //LLVM_DOTPRINTER_H
