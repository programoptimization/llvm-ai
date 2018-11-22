#ifndef LLVM_BRAINSTRUCTIONVISITOR_H
#define LLVM_BRAINSTRUCTIONVISITOR_H

#include <llvm/IR/Instruction.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/BasicBlock.h>
#include "State.h"
#include "src/util.h"
#include "src/domains/EqualityDomain.h"

using namespace llvm;

namespace bra {
    struct InstructionVisitor : public InstVisitor<InstructionVisitor> {
        void visit(BasicBlock &bb);

        void visit(Instruction &inst);

        InstructionVisitor(std::vector<std::shared_ptr<AbstractDomain>> startDomains, std::shared_ptr<State> state,
                           shared_ptr<std::map<Value *, std::shared_ptr<Variable>>> pValueMap);

        std::shared_ptr<State> getState();

        // TODO: implement all Operators

        /// Memory operations
        void visitStoreInst(StoreInst &);

        void visitLoadInst(LoadInst &);

        /// Math operations
        void visitAdd(BinaryOperator &);

        void visitFAdd(BinaryOperator &);

        void visitSub(BinaryOperator &);

        void visitFSub(BinaryOperator &);

        void visitMul(BinaryOperator &);

        void visitFMul(BinaryOperator &);

        void visitUDiv(BinaryOperator &);

        void visitSDiv(BinaryOperator &);

        void visitFDiv(BinaryOperator &);

        void visitURem(BinaryOperator &);

        void visitSRem(BinaryOperator &);

        void visitFRem(BinaryOperator &);

        /// Special operators
        void visitPHINode(PHINode &);

    private:
        // This map helps identify temporary variables without name
        std::shared_ptr<State> state;
        std::vector<std::shared_ptr<AbstractDomain>> startDomains;

        std::string instToString(Instruction &);

        shared_ptr<std::map<Value *, std::shared_ptr<Variable>>> pValueMap;

        /// Helper functions for visitor interface impl
        std::shared_ptr<Variable> helperParseVariable(Value *);

        std::shared_ptr<Representative> helperParseOperand(Value *val);

        void updateStartDomains() const;

        bool visitedPHINodes = false;
    };
}

#endif //LLVM_BRAINSTRUCTIONVISITOR_H
