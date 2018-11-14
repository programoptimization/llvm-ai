
#include "ReturnDomainJoin.h"
#include "../abstract_domain/AbstractDomain.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/InstVisitor.h>

using namespace llvm;
using namespace pcpo;

namespace {
class ReturnDomainVisitor : public InstVisitor<ReturnDomainVisitor, void> {

  std::map<BasicBlock *, State> const &program_points_;
  BasicBlock *current_block_;
  std::shared_ptr<AbstractDomain> &return_domain_;

public:
  explicit ReturnDomainVisitor(
      std::map<BasicBlock *, State> const &program_points,
      BasicBlock *current_block, std::shared_ptr<AbstractDomain> &return_domain)
      : program_points_(program_points), current_block_(current_block),
        return_domain_(return_domain) {}

  void visitReturnInst(llvm::ReturnInst &I) {
    auto const state = program_points_.find(current_block_);
    if (state == program_points_.end()) {
      llvm_unreachable("The ReturnInst is required to have a present domain!");
    }

    std::shared_ptr<AbstractDomain> current =
        state->second.getAbstractValue(&I);

    if (return_domain_) {
      return_domain_->leastUpperBound(*current);
    } else {
      return_domain_ = current;
    }
  }
};
} // namespace

void joinReturnDomain(std::map<BasicBlock *, State> const &program_points,
                      std::shared_ptr<AbstractDomain> return_domain) {
  for (auto &&entry : program_points) {
    BasicBlock *block = entry.first;

    ReturnDomainVisitor visitor(program_points, block, return_domain);
    visitor.visit(block);
  }
}
