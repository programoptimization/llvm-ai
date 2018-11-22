#ifndef BASICRA_UTIL_H
#define BASICRA_UTIL_H

#include <llvm/Support/raw_ostream.h>
#include <string>
#include "globals.h"

// Enable debug output
#define DEBUG

#define RED "\033[0;31m"
// TEMPORARY DEBUG OUTPUT
#define GREEN "\033[0;32m"
// IMPORTANT FOR USER
#define BLUE "\033[0;34m"
// VERBOSE OUTPUT
#define YELLOW "\033[0;33m"
// STRUCTURE OUTPUT
#define PURPLE "\033[0;35m"
// Other OUTPUT
#define NO_COLOR "\033[0m"

#define TEMPORARY_VAR_ID 54

#ifdef DEBUG
#define DEBUG_ERR(text) llvm::errs() << RED << "ERROR: " << text << NO_COLOR << "\n"
#define DEBUG_OUTPUT(text) llvm::errs() << std::string(2*globalDebugOutputTabLevel, ' ') << text << "\n"
#else
#define DEBUG_ERR(text)
#define DEBUG_OUTPUT(text)
#endif
#define STD_OUTPUT(text) llvm::errs() << text << "\n"


#endif //LLVM_UTIL_H
