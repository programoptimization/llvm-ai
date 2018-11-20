#ifndef PROJECT_VISITOR_H
#define PROJECT_VISITOR_H

#include "../abstract_domain/AbstractDomain.h"
#include "../interprocedural/CallHierarchy.h"
#include "../util/util.h"
#include "branch_conditions.h"
#include "state.h"
#include "worklist.h"
#include <llvm/IR/CFG.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Operator.h>
#include <map>
#include <unordered_map>

using namespace llvm;

namespace pcpo {

using ProgramPoints = std::map<BasicBlock *, State>;
using ProgramPointsByCallHierarchy =
    std::unordered_map<CallHierarchy, ProgramPoints>;

class VsaVisitor : public InstVisitor<VsaVisitor, void> {

public:
  VsaVisitor(WorkList &q, CallHierarchy callHierarchy,
             ProgramPointsByCallHierarchy &programPointsByCallHierarchy)
      : worklist(q), currentCallHierarchy_(std::move(callHierarchy)),
        programPointsByHierarchy(programPointsByCallHierarchy),
        shouldSkipInstructions(false){};

  /// create lub of states of preceeding basic blocks and use it as newState;
  /// the visitor automatically visits all instructions of this basic block
  void visitBasicBlock(BasicBlock &BB);

  /// visit TerminatorInstruction:
  /// compare lub(oldState, newState) with oldState of basic block; if state
  /// has changed: update state and push direct successors basic block into
  /// the working list
  void visitTerminatorInst(TerminatorInst &I);

  /// visit BranchInstruction:
  /// before calling visitTerminatorInst, it evaluates branch conditions
  void visitBranchInst(BranchInst &I);

  /// visit SwitchInstruction:
  /// before calling visitTerminatorInst, it evaluates branch conditions
  void visitSwitchInst(SwitchInst &I);

  /// visit LoadInstruction:
  /// set variable explicitly top
  void visitLoadInst(LoadInst &I);

  /// visit PHINode:
  /// visitBasicBlock has already created lub of states of preceeding bbs
  /// here we only add the
  void visitPHINode(PHINode &I);

  /*void visitIndirectBrInst(IndirectBrInst &I);
  void visitResumeInst(ResumeInst &I);
  void visitICmpInst(ICmpInst &I);
  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &I);
  void visitTruncInst(TruncInst &I);
  void visitZExtInst(ZExtInst &I);
  void visitSExtInst(SExtInst &I);
  void visitBitCastInst(BitCastInst &I);
  void visitSelectInst(SelectInst &I);
  void visitVAArgInst(VAArgInst &I);
  void visitExtractElementInst(ExtractElementInst &I);
  void visitExtractValueInst(ExtractValueInst &I);*/

  /// Call and Invoke
  void visitCallInst(CallInst &I);
  // void visitInvokeInst(InvokeInst &I);
  void visitReturnInst(ReturnInst &I);

  /// BinaryOperators
  void visitAdd(BinaryOperator &I);
  void visitSub(BinaryOperator &I);
  void visitMul(BinaryOperator &I);
  void visitURem(BinaryOperator &I);
  void visitSRem(BinaryOperator &I);
  void visitUDiv(BinaryOperator &I);
  void visitSDiv(BinaryOperator &I);
  void visitAnd(BinaryOperator &I);
  void visitOr(BinaryOperator &I);
  void visitXor(BinaryOperator &I);

  void visitShl(Instruction &I);
  void visitLShr(Instruction &I);
  void visitAShr(Instruction &I);

  /// if not specific get the whole class
  // void visitCastInst(CastInst &I);
  void visitBinaryOperator(BinaryOperator &I);
  // void visitCmpInst(CmpInst &I);
  void visitUnaryInstruction(UnaryInstruction &I);

  /// default
  void visitInstruction(Instruction &I);

  void setCurrentCallHierarchy(CallHierarchy callHierarchy);

  /// if set to true any instruction visit will be skipped
  void setShouldSkipInstructions(bool shouldSkipInstructions);

private:
  /// return the program points
  ProgramPoints &getCurrentProgramPoints();
  ProgramPoints &getProgramPoints(CallHierarchy &callHierarchy);

  /// merges domains of parameters of called function with argument domains.
  /// returns true if any parameter domain has changed.
  bool mergeParamDomains(CallInst &callInst, State &calleeState);

  /// puts current newState into global program points or
  /// merges it with the old one.
  void upsertNewState(BasicBlock *currentBB);

  /// merges return domain with call-site domain
  void mergeReturnDomains(CallInst &lastCallInst,
                          CallHierarchy &lastCallHierarchy,
                          std::shared_ptr<AbstractDomain> returnDomain);

  /// push directly reachable basic blocks onto worklist
  void pushSuccessors(TerminatorInst &I);

  void putBothBranchConditions(
      BranchInst &I, Value *op,
      std::pair<shared_ptr<AbstractDomain>, shared_ptr<AbstractDomain>>
          &valuePair);

  /// Returns the current call hierarchy
  pcpo::CallHierarchy &getCurrentCallHierarchy();

  /// Returns the dominator tree for the current function we are inside
  DominatorTree const &getCurrentDominatorTree();

  /// Returns the function where are currently inside in
  llvm::Function *getCurrentFunction();

  State newState;
  WorkList &worklist;
  pcpo::CallHierarchy currentCallHierarchy_;
  ProgramPointsByCallHierarchy &programPointsByHierarchy;
  bool shouldSkipInstructions;
  std::unordered_map<llvm::Function *, DominatorTree> dominatorTreeCache;
  //  /*std::map<CallString,*/ BranchConditions /*>*/ bcs;
};
} // namespace pcpo

#endif // PROJECT_VISITOR_H
