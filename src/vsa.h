#ifndef VSA
#define VSA

#include "api/vsa_result.h"
#include "fixpoint/vsa_visitor.h"
#include "fixpoint/worklist.h"
#include "interprocedural/CallHierarchy.h"
#include "interprocedural/ReturnDomainJoin.h"

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"

#include <unordered_map>
#include <set>

using namespace llvm;
using namespace pcpo;

namespace {
struct VsaPass : public ModulePass {

  // Pass identification, replacement for typeid
  static char ID;

  WorkList worklist;

  std::unordered_map<CallHierarchy, std::map<BasicBlock *, State>> programPointsByHierarchy;

public:
  explicit VsaPass() : ModulePass(ID), worklist(), programPointsByHierarchy() {}

  bool runOnModule(Module &module) override {
    auto const mainFunction = module.getFunction("main");
    if (!mainFunction) {
      return false;
    }

    CallHierarchy initCallHierarchy{mainFunction};
    VsaVisitor vis(worklist, initCallHierarchy, programPointsByHierarchy);

    worklist.push(WorkList::Item(initCallHierarchy, &mainFunction->front()));

    while (!worklist.empty()) {
      auto item = worklist.pop();

      vis.setShouldSkipInstructions(false);
      vis.setCurrentCallHierarchy(item.hierarchy);
      vis.visit(*item.block);
    }

    printPassResultsToTestOutput();

    return false;
  }

  // We don't modify the program, so we preserve all analyses.
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }

private:

  void printPassResultsToTestOutput() {
    for (auto const &hierarchy : collectOrderedCallHierarchies()) {
      Function *currentFunction = hierarchy.getCurrentFunction();
      auto *firstBlock = &currentFunction->front();

      auto const &programPoints = programPointsByHierarchy.at(hierarchy);
      auto const &state = programPoints.at(firstBlock);

      TEST_OUTPUT("- \"" << hierarchy << "\":");

      auto args = currentFunction->args();
      if (args.begin() != args.end()) {
        TEST_OUTPUT("  - arguments:");
        for (auto &&arg : args) {
          auto const domain = state.findAbstractValueOrBottom(&arg);
          TEST_OUTPUT("    - " << arg.getArgNo() << ": \"" << *domain << "\"");
        }
      }

      if (currentFunction->getReturnType()->isIntegerTy()) {
        auto const returnDomain = joinReturnDomain(programPoints);
        TEST_OUTPUT("  - returns:");
        TEST_OUTPUT("    - \"" << *returnDomain << "\"");
      }
    }
  }

  std::set<CallHierarchy> collectOrderedCallHierarchies() {
    std::set<CallHierarchy> orderedHierarchies;
    for (auto &&entry : programPointsByHierarchy) {
      orderedHierarchies.insert(entry.first);
    }
    return orderedHierarchies;
  }

};

char VsaPass::ID = 0;
} // namespace

#endif
