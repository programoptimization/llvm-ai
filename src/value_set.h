#pragma once

#include <unordered_map>
#include <vector>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"

#include "global.h"

namespace pcpo {

class AbstractDomainDummy {
public:
    // This has to initialise to either top or bottom, depending on the flagxs
    AbstractDomainDummy(bool isTop = false)
        { assert(false); };

    // Copy constructor. We need to be able to copy this type. (To be precise, we also need copy
    // assignment.) Probably you can just ignore this and leave the default, compiler-generated copy
    // constructor.
    AbstractDomainDummy(AbstractDomainDummy const&) = default;
    
    // Initialise from a constant
    AbstractDomainDummy(llvm::Constant& constant): AbstractDomainDummy(true) {}

    // @Cleanup Documentation
    static AbstractDomainDummy interpret (
        llvm::Instruction& inst, std::vector<AbstractDomainDummy> const& operands
    ) { return AbstractDomainDummy(true); }

    // @Cleanup Documentation
    bool operator== (AbstractDomainDummy o) const {
        assert(false); return false;
    }
    
    // Refine a by using the information that the value has to fulfill the predicate w.r.t. b. For
    // example, if the domain is an interval domain:
    //     refine_branch( ULE, [5, 10], [7, 8] ) => [5, 8]
    // or
    //     refine_branch( ULE, [5, 10], [7, 8] ) => [5, 10]
    // would be valid implementations, though the former is more precise. In this case the only
    // relevant information is that the number has to be less than or equal to 8.
    //  The following properties have to be fulfilled, if c = refine_branch(~, a, b):
    //     1. c <= a
    //     2. For all x in a, y in b with x~y we have x in c.
    static AbstractDomainDummy refine_branch (
        llvm::CmpInst::Predicate pred, llvm::Value& lhs, llvm::Value& rhs,
        AbstractDomainDummy a, AbstractDomainDummy b
    ) { return a; }

    // @Cleanup Documentation
    static AbstractDomainDummy merge(Merge_op::Type op, AbstractDomainDummy a, AbstractDomainDummy b)
        { return AbstractDomainDummy(true); }
};

char const* get_predicate_name(llvm::CmpInst::Predicate pred);

template <typename AbstractDomain>
class AbstractStateValueSet {
public:
    std::unordered_map<llvm::Value*, AbstractDomain> values;

    // We need an additional boolean, as there is a difference between an empty AbstractState and
    // one that is bottom.
    bool isBottom = true;

public:
    AbstractStateValueSet() = default;
    AbstractStateValueSet(AbstractStateValueSet const& state) = default;

    explicit AbstractStateValueSet(llvm::Function& f) {
        // We need to initialise the arguments to T
        for (llvm::Argument& arg: f.args()) {
            values[&arg] = AbstractDomain {true};
        }
        isBottom = false;
    }

    void apply(llvm::BasicBlock& bb, std::vector<AbstractStateValueSet> const& pred_values) {
        if (isBottom) {
            dbgs(3) << "    Basic block is unreachable, everything is bottom\n";

            return;
        }
        
        std::vector<AbstractDomain> operands;

        // Go through each instruction of the basic block and apply it to the state
        for (llvm::Instruction& inst: bb) {
            // If the result of the instruction is not used, there is no reason to compute
            // it. (There are no side-effects in LLVM IR. (I hope.))
            if (inst.use_empty()) continue;

            AbstractDomain inst_result;
            
            if (llvm::PHINode* phi = llvm::dyn_cast<llvm::PHINode>(&inst)) {
                // Phi nodes are handled here, to get the precise values of the predecessors
                
                for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i) {
                    // Find the predecessor corresponding to the block of the phi node
                    unsigned block = 0;
                    for (llvm::BasicBlock* pred_bb: llvm::predecessors(&bb)) {
                        if (pred_bb == phi->getIncomingBlock(i)) break;
                        ++block;
                    }

                    // Take the union of the values
                    AbstractDomain pred_value = pred_values[block].getAbstractValue(*phi->getIncomingValue(i));
                    inst_result = AbstractDomain::merge(Merge_op::UPPER_BOUND, inst_result, pred_value);

                    operands.push_back(pred_value); // Keep the debug output happy
                }
            } else {
                for (llvm::Value* value: inst.operand_values()) {
                    operands.push_back(getAbstractValue(*value));
                }

                // Compute the result of the operation
                inst_result = AbstractDomain::interpret(inst, operands);
            }
            
            values[&inst] = inst_result;

            dbgs(3).indent(2) << inst << " // " << values.at(&inst) << ", args ";
            {int i = 0;
            for (llvm::Value* value: inst.operand_values()) {
                if (i) dbgs(3) << ", ";
                if (value->getName().size()) dbgs(3) << '%' << value->getName() << " = ";
                dbgs(3) << operands[i];
                ++i;
            }}
            dbgs(3) << '\n';

            operands.clear();
        }
    }
    
    bool merge(Merge_op::Type op, AbstractStateValueSet const& other) {
        bool changed = false;

        if (isBottom > other.isBottom) {
            changed = true;
            isBottom = false;
        }
        
        for (std::pair<llvm::Value*, AbstractDomain> i: other.values) {
            // If our value did not exist before, it will be implicitly initialised as bottom,
            // which works just fine.
            AbstractDomain v = AbstractDomain::merge(op, values[i.first], i.second);

            // No change, nothing to do here
            if (v == values[i.first]) continue;

            if (i.first->getName().size())
                dbgs(3) << "    %" << i.first->getName() << " set to " << v << ", " << Merge_op::name[op] << " "
                        << values[i.first] << " and " << i.second << '\n';
                
            values[i.first] = v;
            changed = true;

        }

        if (changed) checkForBottom(4);
        
        return changed;
    }
    
    void branch(llvm::BasicBlock& from, llvm::BasicBlock& towards) {
        llvm::Instruction* terminator = from.getTerminator();
        assert(terminator /* from is not a well-formed basic block! */);
        assert(terminator->isTerminator());

        llvm::BranchInst* branch = llvm::dyn_cast<llvm::BranchInst>(terminator);

        // If the terminator is not a simple branch, we are not interested
        if (not branch) return;

        // In the case of an unconditional branch, there is nothing to do
        if (branch->isUnconditional()) return;

        llvm::Value& condition = *branch->getCondition();

        llvm::ICmpInst* cmp = llvm::dyn_cast<llvm::ICmpInst>(&condition);

        // We only deal with integer compares here. If you want to do floating points operations as
        // well, you need to adjust the following lines of code a bit.
        if (not cmp) return;

        // We need to find out whether the towards block is on the true or the false branch
        llvm::CmpInst::Predicate pred;
        if (branch->getOperand(2) == &towards) {
            pred = cmp->getPredicate();
        } else if (branch->getOperand(1) == &towards) {
            // We are the false block, so do the negation of the predicate
            pred = cmp->getInversePredicate();
        } else {
            assert(false /* we were not passed the right 'from' block? */);
        }
        llvm::CmpInst::Predicate pred_s = llvm::CmpInst::getSwappedPredicate(pred);

        dbgs(3) << "      Detected branch from " << from.getName() << " towards " << towards.getName()
                << " using compare in %" << cmp->getName() << '\n';
        
        llvm::Value& lhs = *cmp->getOperand(0);
        llvm::Value& rhs = *cmp->getOperand(1);

        AbstractDomain lhs_new;
        AbstractDomain rhs_new;
        
        // Constrain the values if they exist.
        if (values.count(&lhs)) {
            dbgs(3) << "      Deriving constraint %" << lhs.getName()  << ' ' << get_predicate_name(pred) << ' ';
            (rhs.getName().size() ? dbgs(3) << "%" << rhs.getName() : dbgs(3) << rhs)
                    << ", with %" << lhs.getName() << " = " << values[&lhs];
            if (values.count(&rhs)) dbgs(3) << " and %" << rhs.getName() << " = " << values[&rhs];
            dbgs(3) << '\n';

            // For the lhs we say that 'lhs pred rhs' has to hold
            lhs_new = AbstractDomain::refine_branch(pred, lhs, rhs, values[&lhs], getAbstractValue(rhs));
        }
        if (values.count(&rhs)) {
            dbgs(3) << "      Deriving constraint %" << rhs.getName() << ' ' << get_predicate_name(pred_s) << ' ';
            (lhs.getName().size() ? dbgs(3) << "%" << lhs.getName() : dbgs(3) << lhs)
                    << ", with %" << rhs.getName() << " = " << values[&rhs];
            if (values.count(&lhs)) dbgs(3) << " and %" << lhs.getName() << " = " << values[&lhs];
            dbgs(3) << '\n';

            // Here, we take the swapped predicate and assert 'rhs pred_s lhs'
            rhs_new = AbstractDomain::refine_branch(pred_s, rhs, lhs, values[&rhs], getAbstractValue(lhs));
        }

        // The control flow is like this so that the previous ifs do not conflict with one another.
        if (values.count(&lhs)) values[&lhs] = lhs_new;
        if (values.count(&rhs)) values[&rhs] = rhs_new;
        
        if (values.count(&lhs) && values.count(&rhs)) {
            dbgs(3) << "      Values restricted to %" << lhs.getName() << " = " << values[&lhs] << " and %"
                    << rhs.getName() << " = " << values[&rhs] << '\n';
        } else if (values.count(&lhs)) {
            dbgs(3) << "      Value restricted to %" << lhs.getName() << " = " << values[&lhs]  << '\n';
        } else if (values.count(&rhs)) {
            dbgs(3) << "      Value restricted to %" << rhs.getName() << " = " << values[&rhs]  << '\n';
        } else {
            dbgs(3) << "      No restrictions were derived.\n";
        }

        // This cannot happen when doing UPPER_BOUND or WIDEN, but for NARROW it is possible, so
        // check just in case.
        checkForBottom(6);
    }

    void printIncoming(llvm::BasicBlock& bb, llvm::raw_ostream& out, int indentation = 0) const {
        // @Speed: This is quadratic, could be linear
        bool nothing = true;
        for (std::pair<llvm::Value*, AbstractDomain> const& i: values) {
            bool read    = false;
            bool written = false;
            for (llvm::Instruction& inst: bb) {
                if (&inst == i.first) written = true;
                for (llvm::Value* v: inst.operand_values()) {
                    if (v == i.first) read = true;
                }
            }

            if (read and not written) {
                out.indent(indentation) << '%' << i.first->getName() << " = " << i.second << '\n';
                nothing = false;
            }
        }
        if (nothing) {
            out.indent(indentation) << "<nothing>\n";
        }
    }
    void printOutgoing(llvm::BasicBlock& bb, llvm::raw_ostream& out, int indentation = 0) const {
        for (auto const& i: values) {
            out.indent(indentation) << '%' << i.first->getName() << " = " << i.second << '\n';
        }
        if (values.size() == 0) {
            out.indent(indentation) << "<nothing>\n";
        }
    };
    
public:
    AbstractDomain getAbstractValue(llvm::Value& value) const {
        if (llvm::Constant* c = llvm::dyn_cast<llvm::Constant>(&value)) {
            return AbstractDomain {*c};
        } else if (values.count(&value)) {
            return values.at(&value);
        } else {
            return AbstractDomain {true};
        }
    }

    // If any of our values is bottom, then we are bottom as well. So this function checks that and
    // normalises our value. Returns whether this changed our value (i.e. we are now bottom).
    bool checkForBottom(int indent = 0) {
        if (isBottom) return false;
        
        for (auto const& i: values) {
            if (i.second == AbstractDomain {}) {
                values.clear();
                isBottom = true;

                dbgs(3).indent(indent) << "Variable %" << i.first->getName() << " is bottom, so the state is as well.\n";
                
                return true;
            }
        }
        return false;
    }
};

} /* end of namespace pcpo */
