#ifndef BBWORKLIST
#define BBWORKLIST

#include "llvm/IR/BasicBlock.h"
#include <queue>
#include <set>

using namespace llvm;
using namespace std;

namespace bra {

    class BbWorkList {

    public:
        void push(BasicBlock *bb);

        BasicBlock *peek();

        BasicBlock *pop();

        bool empty();

        std::string toString();

		bool find(BasicBlock *bb);

    private:
        std::queue<BasicBlock *> worklist;
        std::set<BasicBlock *> inWorklist;
    };
}

#endif