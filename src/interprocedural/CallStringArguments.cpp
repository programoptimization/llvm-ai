#include "CallStringArguments.h"
#include <cassert>
void pcpo::CallStringArguments::joinIn(const pcpo::CallStringArguments &other) {
  assert(argDomains.size() == other.argDomains.size());
  //TODO implement element-wise merge
}
