#include "simple_interval.h"

namespace pcpo {

SimpleInterval::SimpleInterval(llvm::Constant& constant) {
    if (llvm::ConstantInt* c = llvm::dyn_cast<llvm::ConstantInt>(&constant)) {
        state = NORMAL;
        begin = c->getValue();
        end = c->getValue();
        return;
    }
    // Depending on how you want to handle undef values, you might want to consider them as any
    // value (e.g. 0).
    //     if (llvm::UndefValue* c = llvm::dyn_cast<llvm::UndefValue>(&constant)) {
    //         if (llvm::IntegerType* ty = llvm::dyn_cast<llvm::IntegerType>(c->getType())) {
    //             state = NORMAL;
    //             begin = APInt::getNullValue(ty->getBitWidth());
    //             end = begin;
    //         }
    //         state = TOP;
    //         return;
    //     }

    state = TOP;
}

SimpleInterval::SimpleInterval(APInt _begin, APInt _end) {
    assert(_begin.getBitWidth() == _end.getBitWidth());
    state = NORMAL;
    begin = _begin;
    end = _end;
}


// Internal helper functions

using APInt = llvm::APInt;

static APInt ap_smin(APInt a, APInt b) {
    return a.slt(b) ? a : b;
}
static APInt ap_umin(APInt a, APInt b) {
    return a.ult(b) ? a : b;
}

static SimpleInterval _icmp_ne(SimpleInterval a, SimpleInterval b) {
    // Basically, we can only do something if b is a single value that lies at of of the ends of a
    if (b.begin != b.end) return a;
    
    if (a.begin == a.end + 1) {
        // a is top, so just exclude the one value
        return SimpleInterval {b.begin + 1, b.begin - 1};
    } else if (a.begin == b.begin and a.begin == a.end) {
        // Single value, we want bottom
        return SimpleInterval {};
    } else if (a.begin == b.begin) {
        return SimpleInterval {a.begin + 1, a.end};
    } else if (a.end == b.begin) {
        return SimpleInterval {a.begin, a.end - 1};
    } else {
        return a;
    }
}

static SimpleInterval _icmp_ule_val(SimpleInterval a, APInt v) {
    if (a.begin.ugt(a.end)) {
        // Overflow
        return SimpleInterval {
            APInt::getNullValue(a.begin.getBitWidth()),
            a.begin.ule(v) ? v : ap_umin(a.end, v)
        };
    } else if (v.ult(a.begin)) {
        return SimpleInterval {};
    } else {
        return SimpleInterval {a.begin, ap_umin(a.end, v)};
    }
}

static SimpleInterval _icmp_ult_val(SimpleInterval a, APInt v) {
    if (v.isNullValue()) return SimpleInterval();
    return _icmp_ule_val(a, v-APInt(a.begin.getBitWidth(), 1));
}

static SimpleInterval _icmp_neg(SimpleInterval a) {
    if (a.isBottom()) return a;
    return {-a.end, -a.begin};
}

static SimpleInterval _icmp_inv(SimpleInterval a) {
    if (a.isBottom()) return a;
    return {~a.end, ~a.begin};
}

static SimpleInterval _icmp_shift(SimpleInterval a) {
    if (a.isBottom()) return a;
    APInt q = APInt::getSignedMinValue(a.begin.getBitWidth());
    a.begin += q;
    a.end += q;
    return a;
}

SimpleInterval SimpleInterval::interpret(
    llvm::Instruction& inst, std::vector<SimpleInterval> const& operands
) {    
    if (operands.size() != 2) return SimpleInterval {true};

    // We only deal with integer types
    llvm::IntegerType* type = llvm::dyn_cast<llvm::IntegerType>(inst.getType());
    if (not type) return SimpleInterval {true};
    
    unsigned bitWidth = inst.getOperand(0)->getType()->getIntegerBitWidth();
    assert(bitWidth == inst.getOperand(1)->getType()->getIntegerBitWidth());

    SimpleInterval a = operands[0];
    SimpleInterval b = operands[1];

    // Handle integer compare instructions. This is not really useful, as it just determines whether
    // the comparison can be true or false. The actual branching logic in the value set does a more
    // careful analysis of which conditions lead to a basic block, deriving upper and lower bounds
    // on the variables involved. However, always getting top for the results annoyed me. (In
    // theory, someone could also do computations with them.
    if (llvm::ICmpInst* icmp = llvm::dyn_cast<llvm::ICmpInst>(&inst)) {
        bool never_true  = refine_branch(icmp->getPredicate(),        *this, o).isBottom();
        bool never_false = refine_branch(icmp->getInversePredicate(), *this, o).isBottom();
        if (never_true and never_false) {
            return SimpleInterval {};
        } else if (never_true) {
            return SimpleInterval {APInt::getNullValue(1), APInt::getNullValue(1)};
        } else if (never_false) {
            return SimpleInterval {APInt::getMaxValue(1), APInt::getMaxValue(1)};
        } else {
            return SimpleInterval {true};
        }
    }
    
    // The following functions do not really want to deal with top. (Keep in mind that we do not
    // need to always returns top, e.g. when doing division.) So instead we pass the full interval.
    a = a._makeTopInterval(bitWidth);
    b = b._makeTopInterval(bitWidth);

#define DO_BINARY_OV(x)                                                 \
    case llvm::Instruction::x:                                          \
        if (a.isBottom() or b.isBottom()) return SimpleInterval {};     \
        return a._##x(b, inst.hasNoUnsignedWrap(), inst.hasNoSignedWrap())._makeTopSpecial();
#define DO_BINARY(x)                                                    \
    case llvm::Instruction::x:                                          \
        if (a.isBottom() or b.isBottom()) return SimpleInterval {};     \
        return a._##x(b)._makeTopSpecial();
#define DO_COMPARE(x)
    case llvm::Instruction::x:                                          \
        if (a.isBottom() or b.isBottom()) return SimpleInterval {};     \
        return a._##x(b)._makeTopSpecial();
    
    switch (inst.getOpcode()) {
        DO_BINARY_OV(Add);
        DO_BINARY_OV(Sub);
        DO_BINARY_OV(Mul);
        DO_BINARY(UDiv);
        DO_BINARY(URem);
        DO_BINARY(SRem);
    case llvm::Instruction::ICmp:
        return 
    default:
        return SimpleInterval {true};
    }

#undef DO_BINARY_OV
#undef DO_BINARY
}

SimpleInterval SimpleInterval::refine_branch(
    llvm::CmpInst::Predicate pred, llvm::Value& lhs, llvm::Value& rhs,
    SimpleInterval a1, SimpleInterval a2
) {
    // Here we do all the checks for top and bottom, so that the code below does not have to deal
    // with them.
    if (a1.isBottom() || a2.isBottom()) return SimpleInterval {};

    // We only deal with integer types
    llvm::IntegerType* type = llvm::dyn_cast<llvm::IntegerType>(lhs.getType());
    if (not type) return SimpleInterval {true};
    
    unsigned bitWidth = type->getBitWidth();
    assert(bitWidth == rhs.getType()->getIntegerBitWidth());
    
    a1 = a1._makeTopInterval(bitWidth);
    a2 = a2._makeTopInterval(bitWidth);

    return _refine_branch(pred, a1, a2)._makeTopSpecial();
}

SimpleInterval SimpleInterval::_refine_branch(
    llvm::CmpInst::Predicate pred, SimpleInterval a1, SimpleInterval a2
) {
    using Predicate = llvm::CmpInst::Predicate;
    
    switch (pred) {
    case Predicate::ICMP_EQ: return a1._narrow(a2);
    case Predicate::ICMP_NE: return _icmp_ne(a1, a2);
    case Predicate::ICMP_ULE: return _icmp_ule_val(a1, a2._umax());
    case Predicate::ICMP_ULT: return _icmp_ult_val(a1, a2._umax());

    // We can reduce to LE/LT by flipping all bits
    case Predicate::ICMP_UGE: return _icmp_inv(_icmp_ule_val(_icmp_inv(a1), ~a2._umin()));
    case Predicate::ICMP_UGT: return _icmp_inv(_icmp_ult_val(_icmp_inv(a1), ~a2._umin()));

    // We can reduce to the unsigned case by adding the smallest negative value. (So 1 << n-1)
    case Predicate::ICMP_SLE: return _icmp_shift(_refine_branch(Predicate::ICMP_ULE, _icmp_shift(a1), _icmp_shift(a2)));
    case Predicate::ICMP_SLT: return _icmp_shift(_refine_branch(Predicate::ICMP_ULT, _icmp_shift(a1), _icmp_shift(a2)));
    case Predicate::ICMP_SGE: return _icmp_shift(_refine_branch(Predicate::ICMP_UGE, _icmp_shift(a1), _icmp_shift(a2)));
    case Predicate::ICMP_SGT: return _icmp_shift(_refine_branch(Predicate::ICMP_UGT, _icmp_shift(a1), _icmp_shift(a2)));

    // This function is supposed to refine a1, so returning that is always fine
    default: return a1;
    }
}

SimpleInterval SimpleInterval::merge(Merge_op::Type op, SimpleInterval a, SimpleInterval b) {
    if (a.isBottom()) return b;
    if (b.isBottom()) return a;
    if (a.isTop() || b.isTop()) return SimpleInterval {true};

    // Note that T is handled above, so no need to convert the inputs
    
    switch (op) {
    case Merge_op::UPPER_BOUND: return a._upperBound(b)._makeTopSpecial();
    case Merge_op::WIDEN:       return a._widen     (b)._makeTopSpecial();
    case Merge_op::NARROW:      return a._narrow    (b)._makeTopSpecial();
    default:
        assert(false /* invalid op value */);
        return SimpleInterval {true};
    }
}


bool SimpleInterval::operator==(SimpleInterval o) const {
    return state == NORMAL
        ? o.state == NORMAL and begin == o.begin and end == o.end
        : state == o.state;
}

bool SimpleInterval::contains(APInt value) const {
    assert(value.getBitWidth() == begin.getBitWidth());

    if (state != NORMAL) return state == TOP;

    return _innerLe(value, end);
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, SimpleInterval a) {
    if (a.isBottom()) {
        os << "[]";
    } else if(a.isTop()) {
        os << "T";
    } else {
        os << "[" << a.begin.toString(10, true) << ", "
           << a.end.toString(10, true) << "]";
    }
    return os;
}

void SimpleInterval::printOut() const {
    if (state == BOTTOM) {
        llvm::errs() << "[]";
    } else if(state == TOP) {
        llvm::errs() << "T";
    } else {
        llvm::errs() << "[" << begin.toString(10, true) << ", "
           << end.toString(10, true) << "]";
    }
    llvm::errs() << '\n';
}

// If we are top we convert to an interval that contains every number instead. This makes the
// calculations easier, because there are fewer special cases to handle.
SimpleInterval SimpleInterval::_makeTopInterval(unsigned bitWidth) const {
    if (isTop()) {
        return SimpleInterval {
            APInt::getNullValue(bitWidth),
            APInt::getMaxValue (bitWidth)
        };
    } else {
        return *this;
    }
}
// This does the reverse transformation. If the interval contains every value, we return the special
// value top instead.
SimpleInterval SimpleInterval::_makeTopSpecial() const {
    if (begin == end + 1) {
        return SimpleInterval {true};
    } else {
        return *this;
    }
}

SimpleInterval SimpleInterval::_Add (SimpleInterval o, bool nuw, bool nsw) const {
    if (nuw && begin.ule(end) && o.begin.ule(o.end)) {
        bool ov;
        APInt r_begin = begin.uadd_ov(o.begin, ov);
        if (ov) return SimpleInterval(); // bottom
        APInt r_end = end.uadd_ov(o.end, ov);
        r_end = APInt::getMaxValue(begin.getBitWidth());
        return SimpleInterval(r_begin, r_end);
    }

    if (nsw && begin.sle(end) && o.begin.sle(o.end)) {
        bool ov;
        APInt r_begin = begin.sadd_ov(o.begin, ov);
        if (ov && !begin.isNegative()) return SimpleInterval(); // bottom
        if (ov &&  begin.isNegative()) r_begin = APInt::getSignedMinValue(begin.getBitWidth());
        APInt r_end = end.sadd_ov(o.end, ov);
        if (ov &&  end.isNegative())   return SimpleInterval(); // bottom
        if (ov && !end.isNegative())   r_end = APInt::getSignedMaxValue(begin.getBitWidth());
        return SimpleInterval(r_begin, r_end);
    }

    {
        bool ov;
        (end - begin).uadd_ov(o.end - o.begin, ov);
        if (ov) return SimpleInterval(true);
        APInt r_begin = begin + o.begin;
        APInt r_end   = end   + o.end;
        return SimpleInterval(r_begin, r_end);
    }
}


SimpleInterval SimpleInterval::_Sub (SimpleInterval o, bool nuw, bool nsw) const {
    return _Add(_icmp_neg(o), false, false);
}

SimpleInterval SimpleInterval::_Mul (SimpleInterval o, bool nuw, bool nsw) const {
    // Multiplication results do not depend on signedness. So we just flip signs
    // to find the best configuration. Also, just ignore nuw and nsw for now.

    APInt u_begin = APInt::getNullValue(begin.getBitWidth());
    APInt u_end = APInt::getNullValue(begin.getBitWidth());
    APInt u_size = APInt::getMaxValue(begin.getBitWidth());
  
    for (int i = 0; i < 4; ++i) {
        bool r_flip = (i ^ (i >> 1)) & 1; // Whether to flip result
    
        SimpleInterval lhs = *this;
        SimpleInterval rhs = o;
        if (i&1) {
            // Flip lhs
            lhs.begin = -end;
            lhs.end = -begin;
        }
        if (i&2) {
            // Flip rhs
            rhs.begin = -o.end;
            rhs.end = -o.begin;
        }

        // Check whether the unsigned multiplication overflows
        bool ov;
        APInt i_end = lhs._umax().umul_ov(rhs._umax(), ov);
        APInt i_begin = lhs._umin() * rhs._umin();
        if (!ov && (i_end - i_begin).ult(u_size)) {
            u_size = i_end - i_begin;
            assert(i_begin.ule(i_end));
            if (r_flip) {
                u_end = -i_begin;
                u_begin = -i_end;
            } else {
                u_begin = i_begin;
                u_end = i_end;
            }
        }
    }

    if (u_size == APInt::getMaxValue(begin.getBitWidth())) {
        return SimpleInterval(true);
    } else {
        return SimpleInterval(u_begin, u_end);
    }
}

SimpleInterval SimpleInterval::_UDiv (SimpleInterval o) const {
    // No interest in dividing by zero later
    APInt o_begin = o.begin;
    APInt o_end = o.end;
    int zeroflags = o_begin.isNullValue() | o_end.isNullValue() << 1;
    if (zeroflags == 1) {
        o_begin = APInt::getOneBitSet(begin.getBitWidth(), 0);
    } else if (zeroflags == 2) {
        o_end = APInt::getAllOnesValue(begin.getBitWidth());
    } else if (zeroflags == 3) {
        return SimpleInterval();
    }

    SimpleInterval r_result (begin, end);
    int wrapflags = begin.ugt(end) << 1 | o_begin.ugt(o_end);
    
    if (wrapflags == 0) {
        // No wraps. Life is simple here.
        r_result.begin = r_result.begin.udiv(o_end);
        r_result.end = r_result.end.udiv(o_begin);
    } else if (wrapflags == 1) {
        // The other one wraps, but we do not, so lets just extend the bottom (can only get smaller)
        r_result.begin = APInt::getMinValue(begin.getBitWidth());
    } else if (wrapflags == 2) {
        // We wrap, they do not.
        r_result.begin = APInt::getMinValue(begin.getBitWidth());
        r_result.end   = APInt::getMaxValue(begin.getBitWidth()).udiv(o_begin);
    } else {
        // Everyone is wrapping. Oh my! (Like Christmas.)
        r_result = SimpleInterval(true);
    }

    return SimpleInterval(r_result);
}


SimpleInterval SimpleInterval::_URem (SimpleInterval o) const {
    if (_umax().ule(o._umax())) {
        return SimpleInterval(APInt::getMinValue(begin.getBitWidth()), _umax());
    } else {
        return SimpleInterval(APInt::getMinValue(begin.getBitWidth()), o._umax());
    }
}

SimpleInterval SimpleInterval::_SRem (SimpleInterval o) const {
    SimpleInterval r {begin, end};

    // Can we have negative results?
    if (_smin().isNegative()) {
        // Yes. Find the smallest (i.e. most negative) one
        r.begin = _smin();
        if (o._smaxabsneg().sgt(r.begin)) r.begin = o._smaxabsneg();
    } else {
        r.begin = APInt::getNullValue(begin.getBitWidth());
    }

    // Can we have positive results?
    if (_smax().isNonNegative()) {
        // Yes. Find the largest one
        r.end = _smax();
        if (o._smaxabsneg().sgt(-r.end)) r.end = -o._smaxabsneg();
    } else {
        r.end = APInt::getNullValue(begin.getBitWidth());
    }

    return SimpleInterval(r);
}

SimpleInterval SimpleInterval::_upperBound(SimpleInterval o) const {
    int overflag = contains(o.begin) << 1 | contains(o.end);
    if (overflag == 0) {
        // We do not contain any of o, but the reverse may hold

        if (o.contains(begin)) {
            // We are in o
            return o;
        }
        
        // No overlap, so choose the smaller one
        if ((o.end-begin).ule(end-o.begin)) {
            return SimpleInterval {begin, o.end};
        } else {
            return SimpleInterval {o.begin, end};
        }
    } else if (overflag == 2) {
        // We only contain o.begin
        return SimpleInterval {begin, o.end};
    } else if (overflag == 1) {
        // We only contain o.end
        return SimpleInterval {o.begin, end};
    } else {
        // We have all of o. Still need to check whether they wrap the other way
        if (_innerLe(o.begin, o.end)) {
            return SimpleInterval {begin, end};
        } else {
            return SimpleInterval {true};
        }
    }
}

SimpleInterval SimpleInterval::_widen(SimpleInterval o) const {
    APInt incr = end - begin;;
    if (incr.uge(APInt::getSignedMaxValue(begin.getBitWidth()))) {
        // Too large already, return true
        return SimpleInterval(true);
    } 

    // Widen the sides that changed
    SimpleInterval r = _upperBound(o);
    int flags = (r.begin != begin) | (r.end != end) << 1;
    incr.ashrInPlace(flags == 3 ? 1 : 0); // Divide by two if we widen into both directions
    incr += incr.isNullValue(); // Always widen by at least 1
    if (flags & 1) r.begin -= incr;
    if (flags & 2) r.end   += incr;
    return r;
}

SimpleInterval SimpleInterval::_narrow(SimpleInterval o) const {
    int overflag = contains(o.begin) << 1 | contains(o.end);
    if (overflag == 0) {
        // We do not contain any of o, but the reverse may hold
        if (o.contains(begin)) {
            // We are in o
            return *this;
        } else {
            // No overlap
            return SimpleInterval {};
        }
    } else if (overflag == 2) {
        // We only contain o.begin
        return {o.begin, end};
    } else if (overflag == 1) {
        // We only contain o.end
        return {begin, o.end};
    } else {
        // We have all of o. They may wrap differently.
        if (_innerLe(o.begin, o.end)) {
            // They do not.
            return o;
        } else {
            // Here we could choose one of the two, but for the purpose of narrowing we need to return
            // some subset of *this.
            return *this;
        }
    }
}

bool SimpleInterval::operator<=(SimpleInterval o) const {
    if (state != NORMAL or o.state != NORMAL) {
        return state <= o.state;
    }
  
    return o._innerLe(begin, end) && o._innerLe(end, o.end);
}

APInt SimpleInterval::_umax() const {
    return begin.ugt(end) ? APInt::getMaxValue(begin.getBitWidth()) : end;
}
APInt SimpleInterval::_umin() const {
    return begin.ugt(end) ? APInt::getMinValue(begin.getBitWidth()) : begin;
}
APInt SimpleInterval::_smax() const {
    return begin.sgt(end) ? APInt::getSignedMaxValue(begin.getBitWidth()) : end;
}
APInt SimpleInterval::_smin() const {
    return begin.sgt(end) ? APInt::getSignedMinValue(begin.getBitWidth()) : begin;
}

APInt SimpleInterval::_smaxabsneg() const {
    if (begin.sgt(end)) {
        // Our interval contains a signed wrap
        return APInt::getSignedMinValue(begin.getBitWidth());
    } else {
        return ap_smin(begin.isNegative() ? begin : -begin, end.isNegative() ? end : -end);
    }
}

bool SimpleInterval::_innerLe(APInt a, APInt b) const {
    // Return whether a <= b relative to the interval. So if both a, b are inside, then a <= b iff a
    // is no farther from begin than b.
    return (a - begin).ule(b - begin);
}

} /* end of namespace pcpo */
