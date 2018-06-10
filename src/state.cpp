#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>

#include "state.h"
#include "util.h"

using namespace llvm;
namespace pcpo {

State::State() : conditionCacheUsed(false), bottom(true) {}

bool State::put(Value &v, std::shared_ptr<AbstractDomain> ad) {

   assert(!bottom && "Visited although bottom!");

  if (ad->lessOrEqual(*BoundedSet::create_bottom())) {
    DEBUG_OUTPUT("State::put: set to bottom because of " << v.getName());
    ad->printOut();
    bottom = true;
    return true;
  }

  DEBUG_OUTPUT("State::put for " << v.getName());
  // ad->printOut();
  if (vars.find(&v) != vars.end()) {
    if (ad->lessOrEqual(*vars[&v]))
      return false;
    vars[&v] = vars[&v]->leastUpperBound(*ad);

  } else {
    vars[&v] = ad;
  }
  return true;
}

bool State::leastUpperBound(State &other) {
  bool change = false;
  for (auto &var : other.vars) {
    change |= put(*var.first, var.second);
  }
  return change;
}

/// remove all <key,value> from this state where other does not have the key
/// \param other other state
void State::prune(State &other) {
  auto temp = vars;
  for (auto &var : vars) {
    DEBUG_OUTPUT("State::prune has: " << var.first->getName());
    if (other.vars.find(var.first) == other.vars.end()) {
      temp.erase(var.first);
      DEBUG_OUTPUT("   and removes");
    }
  }
  vars = temp;
}

shared_ptr<AbstractDomain> State::getAbstractValue(Value *v) {

  if (ConstantInt::classof(v)) {
    auto temp = reinterpret_cast<ConstantInt *>(v);
    return shared_ptr<AbstractDomain>(new BoundedSet(temp->getValue()));
  }

  auto find = vars.find(v);
  if (find != vars.end()) {
    return find->second;
  }

  DEBUG_OUTPUT("State::getAbstractValue " << v->getName() << " : failed");

  return AD_TYPE::create_top();
}

std::pair<Value *, std::shared_ptr<AbstractDomain>>
State::get_branch_condition(BasicBlock *bb) {
  if (branchConditions.find(bb) != branchConditions.end()) {
    return branchConditions[bb];
  } else {
    return std::pair<Value *, std::shared_ptr<AbstractDomain>>(nullptr,
                                                               nullptr);
  }
}

void State::applyCondition(BasicBlock *bb) {

  assert(!conditionCacheUsed &&
         "ConditionCache has not been correctly unapplied last time!");

  if (branchConditions.find(bb) != branchConditions.end()) {
    auto &branchCondition = branchConditions[bb];
    conditionCacheUsed = true;

    auto value = branchCondition.first;
    conditionCache = std::pair<Value *, std::shared_ptr<AbstractDomain>>(
        value, getAbstractValue(value));

    vars[value] = branchCondition.second;
  }
}

void State::unApplyCondition() {

  if (!conditionCacheUsed)
    return;

  vars[conditionCache.first] = conditionCache.second;
  conditionCacheUsed = false;
}

bool State::isBottom() { return bottom; }

void State::setNotBottom() { bottom = false; }

void State::print() {
  if (bottom) {
    STD_OUTPUT("bottom");
    return;
  }

  for (auto &var : vars) {
    // std::cout << *var.second << std::endl;
    STD_OUTPUT(var.first->getName() << " -> " << *var.second);
    // var.second->printOut();
  }
}

void State::putBranchConditions(BasicBlock *bb, Value *val,
                                std::shared_ptr<AbstractDomain> ad) {

  branchConditions[bb] =
      std::pair<Value *, std::shared_ptr<AbstractDomain>>(val, ad);
}

void State::transferBranchConditions(State &other) {
  this->branchConditions = other.branchConditions;
}

void State::transferBottomness(State &other) { this->bottom = other.bottom; }
}
