#pragma once

#include "llvm/Support/raw_ostream.h"

namespace pcpo {

// Higher values mean more debug output.
//   0: just the result
//   1: general information about the fixpoint iteration
//   2: intermediate results in the fixpoint iteration
//   3: details about the individual operations
// This could be a compile time constant, but it is not, so that you can set it in your debugger.
extern int debug_level;

// This is the initial setting
#define DEBUG_LEVEL 4

// This returns either a stream to stderr or to nowhere, depending on whether we are currently
// outputting that level.
inline llvm::raw_ostream& dbgs(int level) {
    if (level <= DEBUG_LEVEL) {
        return llvm::errs();
    } else {
        return llvm::nulls();
    }
}

namespace Merge_op {

// see the documentation of AbstractStateDummy::merge for an explanation of what these mean
// precisely.
enum Type: int {
    UPPER_BOUND, WIDEN, NARROW
};

constexpr char const* name[] = {
    "joining", "widening", "narrowing"
};

}

} /* end of namespace pcpo */
