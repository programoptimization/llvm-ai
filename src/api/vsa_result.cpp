#include "vsa_result.h"

using namespace llvm;

namespace pcpo {

void VsaResult::print() const {
  for (auto &pp : programPoints) {
    STD_OUTPUT("VsaVisitor::print():" << pp.first->getName());
    pp.second.print();
  }
}

bool VsaResult::isReachable(BasicBlock *BB) const {
  return programPoints.find(BB) != programPoints.end();
}

bool VsaResult::isResultAvailable(BasicBlock *BB, Value *val) const {
  if (!isReachable(BB))
    return false;

  return programPoints[BB].isAvailable(val);
}

std::unique_ptr<VsaResultValue> VsaResult::getAbstractValue(BasicBlock *BB,
                                                            Value *val) const {
  assert(isResultAvailable(BB, val) &&
         "VsaResult::getAbstractValue where no abstract value is available");

  return std::unique_ptr<VsaResultValue>(
      new VsaResultValue(programPoints[BB].getAbstractValue(val)));
}
}
