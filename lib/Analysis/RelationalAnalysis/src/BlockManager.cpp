#include "BlockManager.h"
#include "common/InstructionVisitor.h"
#include "llvm/IR/CFG.h"
#include "domains/EqualityDomain.h"
#include <llvm/Support/raw_ostream.h>
#include <iostream>
#include "util.h"
#include "common/ClassType.h"

using namespace llvm;

namespace bra {
    std::shared_ptr<State> BlockManager::getStateForBBName(std::string bbName) const {
        for (auto bbIt : basicBlock2StateMap) {
            if (bbIt.first->getName().str() == bbName) {
                return bbIt.second;
            }
        }

        return nullptr;
    }

    void BlockManager::analyse(Function &function) {
        pValue2TempVariableMap = make_shared<std::map<Value *, std::shared_ptr<Variable>>>();
        // Visit each BB at least once
        for (BasicBlock &bb : function) {
            /// Visit each BB at least once
            workList.push(&bb);

            /// Initialize a State for each BB
            std::shared_ptr<State> st = std::make_shared<State>(
                    std::vector<std::shared_ptr<AbstractDomain>>({std::make_shared<EqualityDomain>()})
            );
            basicBlock2StateMap.insert({&bb, st});
        }

        while (!workList.empty()) {
            auto block = workList.peek();

            /// Least Upper bounds (starting domains) for each visit
            std::vector<std::shared_ptr<AbstractDomain>> lubs;

            auto preds = predecessors(block);
            if (preds.begin() == preds.end()) {
                // TODO: Manualy add other domains in this case aswell.
                // NOTE: it is important that each domain represents its respective bottom
                lubs.push_back(std::make_shared<EqualityDomain>());
            } else {
                /// Group all domains from all predecessors based on classType
                std::map<DomainType, std::shared_ptr<std::vector<std::shared_ptr<AbstractDomain>>>> domMap;
                for (BasicBlock *pred : preds) {
                    for (auto dom : basicBlock2StateMap[pred]->getDomains()) {
                        auto domIt = domMap.find(dom->getClassType());
                        std::shared_ptr<std::vector<std::shared_ptr<AbstractDomain>>> domList;
                        if (domIt == domMap.end()) {
                            domList = std::make_shared<std::vector<std::shared_ptr<AbstractDomain>>>();
                            domList->push_back(dom);
                            domMap.insert({dom->getClassType(), domList});
                        } else {
                            domList = domIt->second;
                            domList->push_back(dom);
                        }
                    }
                }

                /// Calculate LUB for each domain group (== aggregate using LUB)
                for (auto domMapIt = domMap.begin(); domMapIt != domMap.end(); domMapIt++) {
                    std::vector<std::shared_ptr<AbstractDomain>> domainList = *domMapIt->second.get();
                    lubs.push_back((domainList[0])->leastUpperBound(domainList));
                }
            }

            std::shared_ptr<State> state = basicBlock2StateMap.find(block)->second;
            InstructionVisitor instructionVisitor(lubs, state, pValue2TempVariableMap);
            instructionVisitor.visit(*workList.pop());

            if (state->wasUpdatedOnLastVisit()) {
                // Reappend all children of bb
                for (BasicBlock *succ : successors(block)) {
                    if (!workList.find(succ)) {
                        workList.push(succ);
                    }
                }
            }

            for (auto stIt = basicBlock2StateMap.begin(); stIt != basicBlock2StateMap.end(); stIt++) {
                auto st = stIt->second;
                for (const auto &d : st->getDomains()) {
                    DEBUG_OUTPUT(string(BLUE)
                                         +stIt->first->getName().str() + " -> " +
                                         d->toString() + string(NO_COLOR));
                }
            }
        }

        /// Print out analysis result
        DEBUG_OUTPUT(string(BLUE)
                             +"-------------------------------Analysis Result----------------------------------" +
                             string(NO_COLOR));
        for (auto it = basicBlock2StateMap.begin(); it != basicBlock2StateMap.end(); it++) {
            DEBUG_OUTPUT(it->first->getName().str() + ":");
            for (const auto &d : it->second->getDomains()) {
                DEBUG_OUTPUT("  "
                             + d->toString());
            }
        }
        DEBUG_OUTPUT(string(BLUE)
                             +"--------------------------------------------------------------------------------" +
                             string(NO_COLOR));
    }
}
