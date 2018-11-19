#ifndef PROJECT_VISITOR_H
#define PROJECT_VISITOR_H

#include <llvm/IR/CFG.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Operator.h>
#include <unordered_map>
#include "../abstract_domain/AbstractDomain.h"
#include "../util/util.h"
#include "../interprocedural/CallHierarchy.h"
#include "branch_conditions.h"
#include "state.h"
#include "worklist.h"
#include <map>
#include <unordered_map>

using namespace llvm;

namespace pcpo {

class VsaVisitor : public InstVisitor<VsaVisitor, void> {

public:
  VsaVisitor(WorkList &q, CallHierarchy callHierarchy, std::unordered_map<CallHierarchy, std::map<BasicBlock *, State>>& programPoints)
      : worklist(q), currentCallHierarchy_(std::move(callHierarchy)), programPoints(programPoints), shouldRun(true) /*, bcs(programPoints)*/{
  };

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

  void upsertNewState(BasicBlock *currentBB);
  void mergeReturnDomains(CallInst &lastCallInst, CallHierarchy &lastCallHierarchy, std::shared_ptr<AbstractDomain> returnDomain);

  /// print state of all basic blocks
  void print() const;

  /// return the program points
  std::map<BasicBlock *, State> &getProgramPoints();
  std::map<BasicBlock *, State> const& getProgramPoints() const;
  std::map<BasicBlock *, State> &getProgramPoints(CallHierarchy& callHierarchy);

  void setCurrentCallHierarchy(CallHierarchy callHierarchy);

  void makeRunnable();

private:
  /// push directly reachable basic blocks onto worklist
  void pushSuccessors(TerminatorInst &I);

  void putBothBranchConditions(BranchInst& I, Value* op,
    std::pair<shared_ptr<AbstractDomain>, shared_ptr<AbstractDomain>> &valuePair);

  /// Returns the current call hierarchy
  pcpo::CallHierarchy &getCurrentCallHierarchy();

  /// Returns the dominator tree for the current function we are inside
  DominatorTree const &getCurrentDominatorTree();

  /// Returns the function where are currently inside in
  llvm::Function *getCurrentFunction();

  State newState;
  WorkList &worklist;
  pcpo::CallHierarchy currentCallHierarchy_;
  std::unordered_map<CallHierarchy, std::map<BasicBlock *, State>> &programPoints;
  bool shouldRun;
  std::unordered_map<llvm::Function *, DominatorTree> dominatorTreeCache;
//  /*std::map<CallString,*/ BranchConditions /*>*/ bcs;
};
} // namespace pcpo

#endif // PROJECT_VISITOR_H
