#ifndef VSA
#define VSA

#include "api/vsa_result.h"
#include "fixpoint/vsa_visitor.h"
#include "fixpoint/worklist.h"
#include "interprocedural/CallHierarchy.h"
#include "interprocedural/ReturnDomainJoin.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <queue>
#include <unordered_map>

#include "llvm/IR/Dominators.h"

using namespace llvm;
using namespace pcpo;

namespace {
struct VsaPass : public ModulePass {
  // Pass identification, replacement for typeid
  static char ID;

  bool do_print;

  WorkList worklist;

  std::unordered_map<CallHierarchy, std::map<BasicBlock *, State>> programPoints;

//  VsaResult result;

public:
  VsaPass(bool do_print = false)
      : ModulePass(ID), do_print(do_print), worklist(), programPoints() {}

  bool doInitialization(Module &m) override {
    return ModulePass::doInitialization(m);
  }

  bool doFinalization(Module &m) override {
    return ModulePass::doFinalization(m);
  }

  bool runOnModule(Module &module) override {
    /// ignore empty modules and go to the next module
    if (module.empty())
      /// our analysis does not change the IR
      return false;

    auto const current_function = module.getFunction("main");
    if (!current_function) {
      return false;
    }

//    std::map<BasicBlock *, State> programPoints;
//    auto vsaResult = VsaResult(programPoints);
//    CallHierarchy<LocalData> hierarchy(current_function, vsaResult);

    programPoints.clear();

    CallHierarchy initCallHierarchy{current_function};
    VsaVisitor vis(worklist, initCallHierarchy, programPoints);

    /// get the first basic block and push it into the worklist
    worklist.push(WorkList::Item(initCallHierarchy, &current_function->front()));

    int visits = 0;

    std::map<std::string, std::vector<int /*visits*/>> trance;
    for (auto &bb : *current_function)
      trance[std::string(bb.getName())].clear();

    // pop instructions from the worklist and visit them until no more
    // are available (the visitor pushes new instructions query-based)
    while (!worklist.empty()) {

      auto item = worklist.pop();
      trance[std::string(item.block->getName())].push_back(visits);

      vis.setCurrentCallHierarchy(item.hierarchy);
      vis.visit(item.block);
#ifdef DEBUG
      print_local(vis, visits);
#endif
      visits++;
    }

#ifndef DEBUG
    print_local(vis, visits - 1);
#endif

    /// print trace
    if (do_print) {
      // TODO Get the function name from the current call hierarchy
      errs() << "\nTRACE OF FUNCTION " << current_function->getName() << ":\n";
      for (auto t : trance) {
        errs() << t.first << "#" << t.second.size() << ": ";
        for (auto s : t.second)
          errs() << s << " ";
        errs() << "\n";
      }
    }

    auto mainReturnDomain = AD_TYPE::create_top(current_function->getReturnType()->getIntegerBitWidth());

    joinReturnDomain(programPoints[initCallHierarchy], mainReturnDomain);

    // todo: Use `mainReturnDomain` to create VsaResult object

    // Our analysis does not change the IR
    return false;
  }

  void print_local(VsaVisitor &vis, int visits) {
    if (!do_print)
      return;
    STD_OUTPUT("");
    STD_OUTPUT("Global state after " << visits << " visits");
    vis.print();
    STD_OUTPUT("");
    STD_OUTPUT("");
    STD_OUTPUT("");
    STD_OUTPUT("");
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
