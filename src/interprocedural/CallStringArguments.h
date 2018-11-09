#ifndef LLVM_CALLSTRINGARGUMENTS_H
#define LLVM_CALLSTRINGARGUMENTS_H

#include "../abstract_domain/AbstractDomain.h"

namespace pcpo {

class CallStringArguments {

  std::vector<AbstractDomain> argDomains;

public:
  void joinIn(CallStringArguments const &other);
};

} // namespace pcpo

#endif // LLVM_CALLSTRINGARGUMENTS_H
