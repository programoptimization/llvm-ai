#include <utility>
#include <string>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Value.h>
#include <sstream>
#include "InstructionVisitor.h"

using namespace llvm;
using namespace bra;

std::shared_ptr<State> InstructionVisitor::getState() {
    return state;
}

InstructionVisitor::InstructionVisitor(std::vector<std::shared_ptr<AbstractDomain>> startDomains,
                                       std::shared_ptr<State> state,
                                       shared_ptr<std::map<Value *, std::shared_ptr<Variable>>> pValueMap) : state(
        std::move(state)), startDomains(std::move(startDomains)), pValueMap(std::move(pValueMap)) {}

void InstructionVisitor::visit(BasicBlock &bb) {
    DEBUG_OUTPUT(std::string(PURPLE)
                         +"Visiting \"" + bb.getName().str() + "\":");
    DEBUG_OUTPUT("State: " + state->toString());
    DEBUG_OUTPUT("Start Domains:");
    for (const auto &dom : startDomains) {
        DEBUG_OUTPUT(std::string(PURPLE)
                             +"  " + dom->toString() + std::string(NO_COLOR));
    }

    globalDebugOutputTabLevel++;
    state->willVisit();
    InstVisitor::visit(bb);
    globalDebugOutputTabLevel--;

    /// Update all domains of state.
    updateStartDomains();

//    DEBUG_OUTPUT(std::string(GREEN)
//                         +"State after: " + state->toString() + std::string(NO_COLOR));
}

void InstructionVisitor::updateStartDomains() const {
    for (const auto &dom : startDomains) {
        state->updateDomain(dom);
    }
}

void InstructionVisitor::visit(Instruction &inst) {
    DEBUG_OUTPUT(instToString(inst));
    unsigned long tempVarCounter = pValueMap->size();

    // Discover any previously unknown temporary Variables
    if (inst.getValueID() == TEMPORARY_VAR_ID) {
        if (pValueMap->find(&inst) == pValueMap->end()) {
            // Does not yet exist
            pValueMap->insert({&inst, std::make_shared<Variable>(std::to_string(tempVarCounter), true)});
        }
    }

    // Actually visit instruction
    globalDebugOutputTabLevel++;
    InstVisitor::visit(inst);
//    DEBUG_OUTPUT(std::string(YELLOW) + state->toString() + std::string(NO_COLOR));
    for (const auto &dom : startDomains) {
        DEBUG_OUTPUT(std::string(GREEN)
                             +"  " + dom->toString() + std::string(NO_COLOR));
    }
    globalDebugOutputTabLevel--;
}

std::shared_ptr<Representative> InstructionVisitor::helperParseOperand(Value *val) {
    /// TODO: add capability to deal with floating point numbers
    if (ConstantInt::classof(val)) {
        int value = reinterpret_cast<ConstantInt *>(val)->getSExtValue();
        return std::make_shared<Constant>(value);
    }

    return helperParseVariable(val);
}

std::shared_ptr<Variable> InstructionVisitor::helperParseVariable(Value *val) {
    std::shared_ptr<Variable> result;

    if (val->getValueID() == TEMPORARY_VAR_ID) {
        auto itP = pValueMap->find(val);
        if (itP != pValueMap->end()) {
            result = itP->second;
        } else {
            DEBUG_ERR("VISIT ADD ENCOUNTERED TEMPORARY VARIABLE NOT IN VALUEMAP!!!");
        }
    } else {
        result = std::make_shared<Variable>(val->getName());
    }

    return result;
}

void InstructionVisitor::visitAdd(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_add(destination, arg1, arg2);
    }
}


void InstructionVisitor::visitFAdd(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_fadd(destination, arg1, arg2);
    }
}


void InstructionVisitor::visitSub(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_sub(destination, arg1, arg2);
    }
}

void InstructionVisitor::visitFSub(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_fsub(destination, arg1, arg2);
    }
}

void InstructionVisitor::visitMul(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_mul(destination, arg1, arg2);
    }
}

void InstructionVisitor::visitFMul(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_fmul(destination, arg1, arg2);
    }
}

void InstructionVisitor::visitUDiv(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_udiv(destination, arg1, arg2);
    }
}

void InstructionVisitor::visitSDiv(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_sdiv(destination, arg1, arg2);
    }
}

void InstructionVisitor::visitFDiv(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_fdiv(destination, arg1, arg2);
    }
}

void InstructionVisitor::visitURem(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_urem(destination, arg1, arg2);
    }
}

void InstructionVisitor::visitSRem(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_srem(destination, arg1, arg2);
    }
}

void InstructionVisitor::visitFRem(BinaryOperator &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));
    std::shared_ptr<Representative> arg2 = helperParseOperand(inst.getOperand(1));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_frem(destination, arg1, arg2);
    }
}

void InstructionVisitor::visitStoreInst(StoreInst &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(inst.getOperand(1));
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_store(destination, arg1);
    }
}

void InstructionVisitor::visitLoadInst(LoadInst &inst) {
    std::shared_ptr<Variable> destination = helperParseVariable(&inst);
    std::shared_ptr<Representative> arg1 = helperParseOperand(inst.getOperand(0));

    // TODO generify this code since its the same for all visit* impls
    for (auto domIt = startDomains.begin(); domIt < startDomains.end(); domIt++) {
        domIt->get()->transform_load(destination, arg1);
    }
}

std::string InstructionVisitor::instToString(Instruction &inst) {
    // inst.getName() return variable name (if any)
    std::string instName = inst.getName().str();
    std::string result;

    if (!instName.empty()) {
        result = "%" + instName + " = ";
    } else {
        auto itP = pValueMap->find(&inst);
        if (itP != pValueMap->end()) {
            result = "%" + itP->second->getName() + " = ";
        }
    }
    result += std::string(inst.getOpcodeName());

    for (auto it = inst.op_begin(); it != inst.op_end(); it++) {
        std::string operatorRep;
        if (ConstantInt::classof(it->get())) {
            operatorRep = "'" + std::to_string(reinterpret_cast<ConstantInt *>(it->get())->getSExtValue()) + "'";
        } else {
            std::string operatorName = it->get()->getName().str();
            if (!operatorName.empty()) {
                operatorRep = "%" + operatorName;
            } else {
                auto itP = pValueMap->find(it->get());
                if (itP != pValueMap->end()) {
                    operatorRep = "%" + itP->second->getName();
                } else {
                    std::stringstream ss;
                    ss << it->get();
                    std::string name = ss.str();
                    operatorRep = "{" + std::to_string(it->get()->getValueID()) + ", " + name + "}";
                }
            }
        }


        result += " " + operatorRep;
    }

    return result;
}

/**
 * Visit method for PHI Nodes (only called form SSA formed IR)
 * @param I the PHINode to be visited
 */
void InstructionVisitor::visitPHINode(PHINode &I) {
    if (visitedPHINodes) {
        // we don't need to loop over the PHI nodes more than once, so guard against it
        return;
    }

    BasicBlock *pParentBlock = I.getParent();
    Instruction *pFirstNonPHI = pParentBlock->getFirstNonPHI();
    std::map<std::string, std::vector<shared_ptr<AbstractDomain>>> domainMap;

    // loop over all PHI nodes
    Instruction *pNode = &I;
    while (pNode != pFirstNonPHI) {
        // we need pNode to be an instruction pointer, but we also need a PhiNode here 
        auto *pPhiNode = static_cast<PHINode *>(pNode);
        // get the current variable to be assigned
        const shared_ptr<Variable> &pVariable = helperParseVariable(pPhiNode);
        // loop over incoming values in the phi node
        for (unsigned int i = 0; i < pPhiNode->getNumIncomingValues(); i++) {
            // get incoming value and block
            const shared_ptr<Representative> &operand = helperParseOperand(pPhiNode->getIncomingValue(i));
            const StringRef &incomingBlockName = pPhiNode->getIncomingBlock(i)->getName();
            const auto &iterator = domainMap.find(incomingBlockName);
            if (iterator != domainMap.end()) {
                // map contains key, add to existing domains
                for (const auto &dom : iterator->second) {
                    dom->transform_store(pVariable, operand);
                }
            } else {
                // map does not contain key, insert new domains into map
                // TODO make domain agnostic
                const auto newDomains = std::vector<std::shared_ptr<AbstractDomain>>(
                        {std::make_shared<EqualityDomain>()});
                for (const auto &dom : newDomains) {
                    dom->transform_store(pVariable, operand);
                }
                domainMap.insert(std::make_pair(incomingBlockName, newDomains));
            }
        }

        pNode = pNode->getNextNode();
    }

    std::map<int, std::vector<shared_ptr<AbstractDomain>>> domainType2DomainsMap;

    for (auto pair : domainMap) {
        for (unsigned int i = 0; i < pair.second.size(); i++) {
            domainType2DomainsMap[i].push_back(pair.second[i]);
        }
    }

    // iterate over the types of domain (EQDomain, ...)
    for (auto pair : domainType2DomainsMap) {
        DEBUG_OUTPUT(string(GREEN)
                             +"[" + std::to_string(pair.first) + ", ");
        for (const auto &dom : pair.second) {
            DEBUG_OUTPUT(dom->toString());
        }
        DEBUG_OUTPUT("]\n" + string(NO_COLOR));
        const shared_ptr<AbstractDomain> &lub = startDomains[pair.first]->leastUpperBound(pair.second);
        DEBUG_OUTPUT(string(PURPLE)
                             +lub->toString() + string(NO_COLOR));
        startDomains[pair.first] = lub;
    }

    visitedPHINodes = true;
}
