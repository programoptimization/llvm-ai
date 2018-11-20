#ifndef VSA
#define VSA

#include "api/vsa_result.h"
#include "fixpoint/vsa_visitor.h"
#include "fixpoint/worklist.h"
#include "interprocedural/CallHierarchy.h"
#include "interprocedural/ReturnDomainJoin.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Pass.h"

#include <unordered_map>
#include <set>

using namespace llvm;
using namespace pcpo;

extern cl::opt<unsigned> CallStringDepth;

namespace {
struct VsaPass : public ModulePass {
  // Pass identification, replacement for typeid
  static char ID;

  WorkList worklist;

  std::unordered_map<CallHierarchy, std::map<BasicBlock *, State>> programPoints;

//  VsaResult result;

public:
  explicit VsaPass() : ModulePass(ID), worklist(), programPoints() {}

  bool runOnModule(Module &module) override {
    if (module.empty()) {
      return false;
    }

    auto const mainFunction = module.getFunction("main");

    if (!mainFunction) {
      return false;
    }

    // todo: use CallStringDepth here
    // unsigned x = CallStringDepth;
    CallHierarchy initCallHierarchy{mainFunction};

    worklist.push(WorkList::Item(initCallHierarchy, &mainFunction->front()));

    VsaVisitor vis(worklist, initCallHierarchy, programPoints);

    unsigned long long numberOfItemsVisited = 0;
    while (!worklist.empty()) {
      auto item = worklist.pop();

      vis.setCurrentCallHierarchy(item.hierarchy);
      vis.visit(*item.block);
      vis.makeRunnable();

      numberOfItemsVisited++;
    }

    std::set<CallHierarchy> orderedHierarchies;
    for (auto &&entry : programPoints) {
      orderedHierarchies.insert(entry.first);
    }

    for (auto const &hierarchy : orderedHierarchies) {
      Function *currentFunction = hierarchy.getCurrentFunction();
      auto *firstBlock = &currentFunction->front();

      auto entry = programPoints.find(hierarchy);
      assert(entry != programPoints.end());

      // The state is required to exist
      auto const stateItr = entry->second.find(firstBlock);
      assert(stateItr != entry->second.end());

      TEST_OUTPUT("- \"" << hierarchy << "\":");

      auto args = currentFunction->args();
      if (args.begin() != args.end()) {
        TEST_OUTPUT("  - arguments:");
        for (auto &&arg : args) {
          auto const domain = stateItr->second.findAbstractValueOrBottom(&arg);
          TEST_OUTPUT("    - " << arg.getArgNo() << ": \"" << *domain << "\"");
        }
      }

      if (currentFunction->getReturnType()->isIntegerTy()) {
        auto const returnDomain = joinReturnDomain(entry->second);
        TEST_OUTPUT("  - returns:");
        TEST_OUTPUT("    - \"" << *returnDomain << "\"");
      }
    }

    return false;
  }

  // We don't modify the program, so we preserve all analyses.
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }

//  VsaResult &getResult() { return result; }
};

char VsaPass::ID = 0;
} // namespace

#endif
