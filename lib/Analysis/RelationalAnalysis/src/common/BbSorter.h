#ifndef LLVM_BBSORTER_H
#define LLVM_BBSORTER_H

#include <memory>
#include "llvm/IR/BasicBlock.h"

namespace bra {

    struct BbSorter {
        inline bool
        operator()(const llvm::BasicBlock* bb1, const llvm::BasicBlock* bb2) const {
            return bb1->getName().str() < bb2->getName().str();
        }

    };

}


#endif //LLVM_BBSORTER_H