
#ifndef RETURN_DOMAIN_JOIN_H_
#define RETURN_DOMAIN_JOIN_H_

#include "fixpoint/state.h"
#include <map>
#include <memory>

namespace pcpo {
class AbstractDomain;
}

namespace llvm {
class BasicBlock;
}

std::shared_ptr<pcpo::AbstractDomain>
joinReturnDomain(std::map<BasicBlock *, pcpo::State> const &program_points);

#endif // RETURN_DOMAIN_JOIN_H_
