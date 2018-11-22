#pragma once

#include <llvm/ADT/APInt.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Instructions.h>

#include "global.h"

namespace pcpo {

// An AbstractDomain using intervals to represent integer values. The intervals are either bottom,
// top, or have to values representing the beginning and end, both inclusive. This means that there
// would be many different representations for top, besides the canonical one where state == T.
// However, all of these are usually disallowed. (Internally, we temporarily switch to the [0, -1]
// representation of top as that makes calculations easier. See _makeTopInterval and _makeTopSpecial
// about that.)
//  See AbstractDomainDummy in value_set.h for documentation of the AbstractDomain interface this
// class implements.
//  There are tests for this class! Look at the run.py script on how to run these. They do simple
// fuzzing of inputs, which is quite effective to detect correctness errors. If you implement an
// operation or modify an existing one, you can add it in there. Also, if you happen to discover a
// bug in the existing code (completely implausible, mind you, but just hypothetically), it would be
// nice to add a test for that behaviour into the testing code.
class SimpleInterval {
    using APInt = llvm::APInt;
public:
    enum State: char {
        // Do not change these values. They are used by operator<=.
        INVALID, BOTTOM = 1, NORMAL = 2, TOP = 4
    };
    char state;
    APInt begin, end;
    
public:
    // The AbstractDomain interface
    SimpleInterval(bool isTop = false): state{isTop ? TOP : BOTTOM} {}
    SimpleInterval(llvm::Constant const& constant);
    static SimpleInterval interpret(
        llvm::Instruction const& inst, std::vector<SimpleInterval> const& operands
    );
    static SimpleInterval refineBranch(
        llvm::CmpInst::Predicate pred, llvm::Value const& lhs, llvm::Value const& rhs,
        SimpleInterval a, SimpleInterval b
    );
    static SimpleInterval merge(Merge_op::Type op, SimpleInterval a, SimpleInterval b);

    // Other functions

    // Warning: This function does not normalise top, i.e. it always has state==NORMAL, even if it
    // contains all values. You might want to call _makeTopSpecial() afterwards.
    SimpleInterval(APInt _begin, APInt _end);

    bool operator==(SimpleInterval other) const;
    bool operator!=(SimpleInterval other) const {return !(*this == other);}
    bool operator<= (SimpleInterval o) const;
    
    bool isTop() const { return state == TOP; };
    bool isBottom() const { return state == BOTTOM; };
    
    bool contains(APInt value) const;

    // You can call this from your debugger
    void printOut() const;

    // These are internal functions that, generally speaking, do not deal with bottom and top. Also,
    // they use the interval representation of top (i.e. [a+1, a] for some a). If you call one of
    // these, you have to take care to deal with those values beforehand, and convert the result
    // into normal form by calling _makeTopSpecial.
    
    SimpleInterval _makeTopInterval(unsigned bitWidth) const;
    SimpleInterval _makeTopSpecial() const;
    SimpleInterval _Add (SimpleInterval o, bool nuw, bool nsw) const;
    SimpleInterval _Sub (SimpleInterval o, bool nuw, bool nsw) const;
    SimpleInterval _Mul (SimpleInterval o, bool nuw, bool nsw) const;
    SimpleInterval _UDiv(SimpleInterval o) const;
    SimpleInterval _URem(SimpleInterval o) const;
    SimpleInterval _SRem(SimpleInterval o) const;
    SimpleInterval _upperBound(SimpleInterval o) const;
    SimpleInterval _widen(SimpleInterval o) const;
    SimpleInterval _narrow(SimpleInterval o) const;

    static SimpleInterval _refineBranch(
        llvm::CmpInst::Predicate pred, SimpleInterval a, SimpleInterval b
    );
    
    APInt _umax() const;
    APInt _umin() const;
    APInt _smax() const;
    APInt _smin() const;
    APInt _smaxabsneg() const;
    bool _innerLe(APInt a, APInt b) const;    
};

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, SimpleInterval a);

} /* end of namespace pcpo */
