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

using namespace llvm;
using namespace pcpo;

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

    CallHierarchy initCallHierarchy{mainFunction};

    worklist.push(WorkList::Item(initCallHierarchy, &mainFunction->front()));

    VsaVisitor vis(worklist, initCallHierarchy, programPoints);

    unsigned long long numberOfItemsVisited = 0;
    while (!worklist.empty()) {
      auto item = worklist.pop();

      STD_OUTPUT("Visiting function: `" << item.hierarchy.getCurrentFunction()->getName() << "` :: BB address: `" << item.block << "`");

      vis.setCurrentCallHierarchy(item.hierarchy);
      vis.visit(*item.block);
      vis.makeRunnable();

      numberOfItemsVisited++;
    }

    // todo: Use `mainReturnDomain` to create VsaResult object
    auto &mainReturnDomain = vis.getMainReturnDomain();

    for (auto &&entry : programPoints) {
      CallHierarchy const hierarchy = entry.first;
      Function *currentFunction = hierarchy.getCurrentFunction();
      auto *firstBlock = &currentFunction->front();

      // The state is required to exist
      auto state = entry.second.find(firstBlock);
      assert(state != entry.second.end());

      TEST_OUTPUT(hierarchy << " ");

      for (auto &&arg : currentFunction->args()) {
        auto domain = state->second.findAbstractValueOrBottom(&arg);
        TEST_OUTPUT("  - " << arg.getArgNo() << ": " << *domain);
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
