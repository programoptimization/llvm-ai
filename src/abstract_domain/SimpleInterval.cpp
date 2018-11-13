#include <llvm/ADT/APInt.h>
#include <llvm/Support/raw_os_ostream.h>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <vector>

#include "SimpleInterval.h"
#include "AbstractDomain.h"
#include "Util.h"
#include "BoundedSet.h"

namespace pcpo {
using llvm::APInt;
using llvm::APIntOps::GreatestCommonDivisor;
using std::vector;

unsigned SimpleInterval::debug_id_gen;


SimpleInterval::SimpleInterval(const SimpleInterval& other)
    : bitWidth(other.bitWidth),
      begin(other.begin),
      end(other.end),
      isBot(other.isBot),
      debug_id(++debug_id_gen) {debug_break();}

SimpleInterval::SimpleInterval(APInt begin, APInt end)
    : bitWidth(begin.getBitWidth()),
      begin(begin), end(end),
      isBot(false),
      debug_id(++debug_id_gen)  {
  assert(end.getBitWidth() == bitWidth);
  debug_break();
}

SimpleInterval::SimpleInterval(APInt value)
    : bitWidth(value.getBitWidth()),
      begin(value), end(value),
      isBot{false},
      debug_id(++debug_id_gen)  {debug_break();}

SimpleInterval::SimpleInterval(bool isTop, unsigned bitWidth)
    : bitWidth(bitWidth),
      begin(APInt::getMinValue(bitWidth)),
      end(APInt::getMaxValue(bitWidth)),
      isBot(!isTop),
      debug_id(++debug_id_gen)  {debug_break();}

SimpleInterval::SimpleInterval(unsigned bitWidth, uint64_t begin,
    uint64_t end)
    : bitWidth(bitWidth),
      begin(APInt(bitWidth, begin)),
      end(APInt(bitWidth, end)),
      isBot(false),
      debug_id(++debug_id_gen) {debug_break();}

SimpleInterval::SimpleInterval(BoundedSet &set)
    : bitWidth(set.getBitWidth()),
      debug_id(++debug_id_gen) {
  debug_break();
  if (set.isBottom()) {
    // create this a bottom
    isBot = true;
  } else if (set.isTop()) {
    isBot = false;
    begin = APInt(bitWidth, 0);
    end = APInt::getMaxValue(bitWidth);
  } else {
    // Always choose the smalles interval containing the values and 0
    APInt min = APInt::getMaxValue(bitWidth);
    APInt max = APInt::getMinValue(bitWidth);
    for (auto const& i: set.getValues()) {
      if (i.slt(min)) min = i;
      if (i.sgt(max)) max = i;
    }
    begin = min;
    end = max;
    isBot = false;
  }
}

bool SimpleInterval::operator==(const SimpleInterval &o) {
  return isBot == o.isBot ? isBot || (begin == o.begin && end == o.end) : false;
}

APInt SimpleInterval::umax() const{
  if (begin.ugt(end)) {
    // Our interval contains an unsigned wrap
    return APInt::getMaxValue(bitWidth);
  } else {
    return end;
  }
}

APInt SimpleInterval::umin() const{
  if (begin.ugt(end)) {
    // Our interval contains an unsigned wrap
    return APInt::getMinValue(bitWidth);
  } else {
    return begin;
  }
}

APInt SimpleInterval::smax() const{
  if (begin.sgt(end)) {
    // Our interval contains a signed wrap
    return APInt::getSignedMaxValue(bitWidth);
  } else {
    return end;
  }
}

APInt SimpleInterval::smin() const{
  if (begin.sgt(end)) {
    // Our interval contains a signed wrap
    return APInt::getSignedMinValue(bitWidth);
  } else {
    return begin;
  }
}

#define SIMPLE_INTERVAL_WRAPPER(x) \
  shared_ptr<AbstractDomain> SimpleInterval::x(unsigned numBits, AbstractDomain &o_) { \
    assert(numBits == bitWidth);                                        \
    SimpleInterval& o = static_cast<SimpleInterval&>(o_);               \
    return std::make_shared<SimpleInterval>(_##x(o));                   \
  }
#define SIMPLE_INTERVAL_WRAPPER2(x) \
  shared_ptr<AbstractDomain> SimpleInterval::x(unsigned numBits, AbstractDomain &o_, bool nuw, bool nsw) { \
    assert(numBits == bitWidth);                                        \
    SimpleInterval& o = static_cast<SimpleInterval&>(o_);               \
    return std::make_shared<SimpleInterval>(_##x(o, nuw, nsw));         \
  }
#define SIMPLE_INTERVAL_WRAPPER3(x) \
  shared_ptr<AbstractDomain> SimpleInterval::x(AbstractDomain &o_) {    \
    SimpleInterval& o = static_cast<SimpleInterval&>(o_);               \
    return std::make_shared<SimpleInterval>(_##x(o));                   \
  }

SIMPLE_INTERVAL_WRAPPER2(add)
SIMPLE_INTERVAL_WRAPPER2(sub)
SIMPLE_INTERVAL_WRAPPER2(mul)
SIMPLE_INTERVAL_WRAPPER(udiv)
SIMPLE_INTERVAL_WRAPPER(urem)
SIMPLE_INTERVAL_WRAPPER(srem)
SIMPLE_INTERVAL_WRAPPER3(leastUpperBound)
SIMPLE_INTERVAL_WRAPPER3(intersect)
SIMPLE_INTERVAL_WRAPPER3(widen)

SimpleInterval SimpleInterval::_add (SimpleInterval const& o, bool nuw, bool nsw) {
  if (isBot || o.isBot) return SimpleInterval();

  if (nuw && begin.ule(end) && o.begin.ule(o.end)) {
    bool ov;
    APInt r_begin = begin.uadd_ov(o.begin, ov);
    if (ov) return SimpleInterval(); // bottom
    APInt r_end = end.uadd_ov(o.end, ov);
    r_end = APInt::getMaxValue(bitWidth);
    return SimpleInterval(r_begin, r_end);
  }

  if (nsw && begin.sle(end) && o.begin.sle(o.end)) {
    bool ov;
    APInt r_begin = begin.sadd_ov(o.begin, ov);
    if (ov && !begin.isNegative()) return SimpleInterval(); // bottom
    if (ov &&  begin.isNegative()) r_begin = APInt::getSignedMinValue(bitWidth);
    APInt r_end = end.sadd_ov(o.end, ov);
    if (ov &&  end.isNegative())   return SimpleInterval(); // bottom
    if (ov && !end.isNegative())   r_end = APInt::getSignedMaxValue(bitWidth);
    return SimpleInterval(r_begin, r_end);
  }

  {
    bool ov;
    (end - begin).uadd_ov(o.end - o.begin, ov);
    if (ov) return SimpleInterval(true, bitWidth);
    APInt r_begin = begin + o.begin;
    APInt r_end   = end   + o.end;
    return SimpleInterval(r_begin, r_end);
  }
}


SimpleInterval SimpleInterval::_sub (SimpleInterval const& o, bool nuw, bool nsw) {
  if (isBot || o.isBot) return SimpleInterval();
  return _add(_icmp_neg(o), false, false);
}

SimpleInterval SimpleInterval::_mul (SimpleInterval const& o, bool nuw, bool nsw) {
  if (isBot || o.isBot) return SimpleInterval();

  // Multiplication results do not depend on signedness. So we just flip signs
  // to find the best configuration. Also, just ignore nuw and nsw for now.

  APInt u_begin = APInt::getNullValue(bitWidth);
  APInt u_end = APInt::getNullValue(bitWidth);
  APInt u_size = APInt::getMaxValue(bitWidth);
  
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
    APInt i_end = lhs.umax().umul_ov(rhs.umax(), ov);
    APInt i_begin = lhs.umin() * rhs.umin();
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

  if (u_size == APInt::getMaxValue(bitWidth)) {
    return SimpleInterval(true, bitWidth);
  } else {
    return SimpleInterval(u_begin, u_end);
  }
}

SimpleInterval SimpleInterval::_udiv (SimpleInterval const& o) {
  if (isBot || o.isBot) return SimpleInterval();
  
  // No interest in dividing by zero later
  APInt o_begin = o.begin;
  APInt o_end = o.end;
  int zeroflags = o_begin.isNullValue() | o_end.isNullValue() << 1;
  if (zeroflags == 1) {
    o_begin = APInt::getOneBitSet(bitWidth, 0);
  } else if (zeroflags == 2) {
    o_end = APInt::getAllOnesValue(bitWidth);
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
    r_result.begin = APInt::getMinValue(bitWidth);
  } else if (wrapflags == 2) {
    // We wrap, they do not.
    r_result.begin = APInt::getMinValue(bitWidth);
    r_result.end   = APInt::getMaxValue(bitWidth).udiv(o_begin);
  } else {
    // Everyone is wrapping. Oh my! (Like Christmas.)
    r_result = SimpleInterval(true, bitWidth);
  }

  return SimpleInterval(r_result);
}


SimpleInterval SimpleInterval::_urem (SimpleInterval const& o) {
  if (isBot || o.isBot) return SimpleInterval();

  if (umax().ule(o.umax())) {
    return SimpleInterval(APInt::getMinValue(bitWidth), umax());
  } else {
    return SimpleInterval(APInt::getMinValue(bitWidth), o.umax());
  }
}

static APInt ap_smin(APInt a, APInt b) {
  return a.slt(b) ? a : b;
}
//static APInt ap_smax(APInt a, APInt b) {
//  return a.sgt(b) ? a : b;
//}

static APInt ap_umin(APInt a, APInt b) {
  return a.ult(b) ? a : b;
}
//static APInt ap_umax(APInt a, APInt b) {
//  return a.ugt(b) ? a : b;
//}

APInt SimpleInterval::smaxabsneg() const{
  if (begin.sgt(end)) {
    // Our interval contains a signed wrap
    return APInt::getSignedMinValue(bitWidth);
  } else {
    return ap_smin(begin.isNegative() ? begin : -begin, end.isNegative() ? end : -end);
  }
}

SimpleInterval SimpleInterval::_srem (SimpleInterval const& o) {
  if (isBot || o.isBot) return SimpleInterval();

  SimpleInterval r (begin, end);

  // Can we have negative results?
  if (smin().isNegative()) {
    // Yes. Find the smallest (i.e. most negative) one
    r.begin = smin();
    if (o.smaxabsneg().sgt(r.begin)) r.begin = o.smaxabsneg();
  } else {
    r.begin = APInt::getNullValue(bitWidth);
  }

  // Can we have positive results?
  if (smax().isNonNegative()) {
    // Yes. Find the largest one
    r.end = smax();
    if (o.smaxabsneg().sgt(-r.end)) r.end = -o.smaxabsneg();
  } else {
    r.end = APInt::getNullValue(bitWidth);
  }

  return SimpleInterval(r);
}

// All operations below always return top, even if one of the operands are bottom.

shared_ptr<AbstractDomain> SimpleInterval::sdiv(unsigned numBits,
    AbstractDomain &other) {
  return SimpleInterval::create_top(numBits);
}

shared_ptr<AbstractDomain> SimpleInterval::shl(unsigned numBits,
  AbstractDomain &other, bool nuw, bool nsw) {
  return SimpleInterval::create_top(numBits);
}

shared_ptr<AbstractDomain> SimpleInterval::lshr(unsigned numBits,
    AbstractDomain &other) {
  return SimpleInterval::create_top(numBits);
}

shared_ptr<AbstractDomain> SimpleInterval::ashr(unsigned numBits,
    AbstractDomain &other) {
  return SimpleInterval::create_top(numBits);
}

shared_ptr<AbstractDomain> SimpleInterval::and_(unsigned numBits,
    AbstractDomain &other) {
  return SimpleInterval::create_top(numBits);
}

shared_ptr<AbstractDomain> SimpleInterval::or_(unsigned numBits,
    AbstractDomain &other) {
  return SimpleInterval::create_top(numBits);
}

shared_ptr<AbstractDomain> SimpleInterval::xor_(unsigned numBits,
    AbstractDomain &other) {
  return SimpleInterval::create_top(numBits);
}

SimpleInterval SimpleInterval::_icmp_ule_val(SimpleInterval a, APInt v) {
  SimpleInterval r(true, a.bitWidth);
  if (a.begin.ugt(a.end)) {
    // Overflow
    r.begin = APInt::getNullValue(a.bitWidth);
    r.end = a.begin.ule(v) ? v : ap_umin(a.end, v);
  } else if (v.ult(a.begin)) {
    r = SimpleInterval();
  } else {
    r.begin = a.begin;
    r.end = ap_umin(a.end, v);
  }
  return r;
}

SimpleInterval SimpleInterval::_icmp_ult_val(SimpleInterval a, APInt v) {
  SimpleInterval r(true, a.bitWidth);
  if (v.isNullValue()) return SimpleInterval();
  return _icmp_ule_val(a, v-APInt(a.bitWidth, 1));
}

SimpleInterval SimpleInterval::_icmp_neg(SimpleInterval a) {
  if (a.isBottom()) return a;
  SimpleInterval r(true, a.bitWidth);
  r.begin = -a.end;
  r.end = -a.begin;
  return r;
}

SimpleInterval SimpleInterval::_icmp_inv(SimpleInterval a) {
  if (a.isBottom()) return a;
  SimpleInterval r(true, a.bitWidth);
  r.begin = ~a.end;
  r.end = ~a.begin;
  return r;
}

SimpleInterval SimpleInterval::_icmp_shift(SimpleInterval a) {
  if (a.isBottom()) return a;
  APInt q = APInt::getSignedMinValue(a.bitWidth);
  a.begin += q;
  a.end += q;
  return a;
}

void SimpleInterval::_icmp(CmpInst::Predicate pred, SimpleInterval a1, SimpleInterval a2, SimpleInterval* r1, SimpleInterval* r2) {
  APInt q = APInt::getSignedMinValue(a1.bitWidth);
  if (a1.isBottom() || a2.isBottom()) {
    *r1 = SimpleInterval();
    *r2 = SimpleInterval();
  }
  if (pred == CmpInst::Predicate::ICMP_EQ) {
    *r1 = a1._intersect(a2);
    *r2 = a1._leastUpperBound(a2);
  } else if (pred == CmpInst::Predicate::ICMP_NE) {
    _icmp(CmpInst::Predicate::ICMP_EQ, a1, a2, r2, r1);
  } else if (pred == CmpInst::Predicate::ICMP_ULE) {
    *r1 = _icmp_ule_val(a1, a2.umax());
    *r2 = _icmp_inv(_icmp_ult_val(_icmp_inv(a1), ~a2.umin()));
  } else if (pred == CmpInst::Predicate::ICMP_ULT) {
    *r1 = _icmp_ult_val(a1, a2.umax());
    *r2 = _icmp_inv(_icmp_ule_val(_icmp_inv(a1), ~a2.umin()));
  } else if (pred == CmpInst::Predicate::ICMP_UGE) {
    _icmp(CmpInst::Predicate::ICMP_ULT, a1, a2, r2, r1);
  } else if (pred == CmpInst::Predicate::ICMP_UGT) {
    _icmp(CmpInst::Predicate::ICMP_ULE, a1, a2, r2, r1);
  } else if (pred == CmpInst::Predicate::ICMP_SLE) {
    _icmp(CmpInst::Predicate::ICMP_ULE, _icmp_shift(a1), _icmp_shift(a2), r1, r2);
    *r1 = _icmp_shift(*r1); *r2 = _icmp_shift(*r2);
  } else if (pred == CmpInst::Predicate::ICMP_SLT) {
    _icmp(CmpInst::Predicate::ICMP_ULE, _icmp_shift(a1), _icmp_shift(a2), r1, r2);
    *r1 = _icmp_shift(*r1); *r2 = _icmp_shift(*r2);
  } else if (pred == CmpInst::Predicate::ICMP_SGE) {
    _icmp(CmpInst::Predicate::ICMP_ULE, _icmp_shift(a1), _icmp_shift(a2), r1, r2);
    *r1 = _icmp_shift(*r1); *r2 = _icmp_shift(*r2);
  } else if (pred == CmpInst::Predicate::ICMP_SGT) {
    _icmp(CmpInst::Predicate::ICMP_ULE, _icmp_shift(a1), _icmp_shift(a2), r1, r2);
    *r1 = _icmp_shift(*r1); *r2 = _icmp_shift(*r2);
  } else {
    *r1 = SimpleInterval(true, a1.bitWidth);
    *r2 = SimpleInterval(true, a1.bitWidth);
  }
}

std::pair<shared_ptr<AbstractDomain>, shared_ptr<AbstractDomain>>
SimpleInterval::icmp(CmpInst::Predicate pred, unsigned numBits,
                      AbstractDomain &o_) {

  SimpleInterval& o = static_cast<SimpleInterval&>(o_);
  if (isBot || o.isBot) return {std::make_shared<SimpleInterval>(), std::make_shared<SimpleInterval>()};

  // numBits == 1 should hold here

  SimpleInterval r1(true, bitWidth), r2(true, bitWidth);
  _icmp(pred, *this, o, &r1, &r2);
  return {std::make_shared<SimpleInterval>(r1), std::make_shared<SimpleInterval>(r2)};
}

size_t SimpleInterval::size() const {
  return (end - begin).getZExtValue();
}

bool SimpleInterval::innerLe(APInt a, APInt b) const {
  // Return whether a <= b relative to the interval. So if both a, b are inside, then a <= b iff a is no farther from begin than b.
  return (a - begin).ule(b - begin);
}

SimpleInterval SimpleInterval::_leastUpperBound(SimpleInterval const& o) {
  if (isBot)   return SimpleInterval(o);
  if (o.isBot) return SimpleInterval(*this);

  SimpleInterval r (begin, end);

  int overflag = contains(o.begin) << 1 | contains(o.end);
  if (overflag == 0) {
    // We do not contain any of o, but the reverse may hold
    if (o.contains(begin)) {
      // We are in o
      r = o;
    }
    // No overlap, so choose the smaller one
    else if ((o.end-begin).ule(end-o.begin)) {
      r.begin = begin;
      r.end = o.end;
    } else {
      r.begin = o.begin;
      r.end = end;
    }
  } else if (overflag == 2) {
    // We only contain o.begin
    r.begin = begin;
    r.end = o.end;
  } else if (overflag == 1) {
    // We only contain o.end
    r.begin = o.begin;
    r.end = end;
  } else {
    // We have all of o. Still need to check whether they wrap the other way
    if (innerLe(o.begin, o.end)) {
      r.begin = begin;
      r.end = end;
    } else {
      r = SimpleInterval(true, bitWidth);
    }
  }

  return SimpleInterval(r);
}

SimpleInterval SimpleInterval::_intersect(SimpleInterval const& o) {
  if (isBot || o.isBot)   return SimpleInterval();

  SimpleInterval r (true, bitWidth);

  int overflag = contains(o.begin) << 1 | contains(o.end);
  if (overflag == 0) {
    // We do not contain any of o, but the reverse may hold
    if (o.contains(begin)) {
      // We are in o
      r = *this;
    } else {
      // No overlap
      r = SimpleInterval();
    }
  } else if (overflag == 2) {
    // We only contain o.begin
    r.begin = o.begin;
    r.end = end;
  } else if (overflag == 1) {
    // We only contain o.end
    r.begin = begin;
    r.end = o.end;
  } else {
    // We have all of o. They may wrap differently.
    if (innerLe(o.begin, o.end)) {
      // They do not.
      return o;
    } else {
      // Here we could choose one of the two, but for the purpose of narrowing we need to return
      // some superset of *this.
      return *this;
    }
  }

  return SimpleInterval(r);
}

bool SimpleInterval::operator<=(AbstractDomain &o_) {
  SimpleInterval& o = static_cast<SimpleInterval&>(o_);
  if (isBot || o.isBot) return isBot;
  
  assert(getBitWidth() == o.getBitWidth());

  return o.isTop() || (o.innerLe(begin, end) && o.innerLe(end, o.end));
}

bool SimpleInterval::contains(APInt value) const {
  assert(value.getBitWidth() == bitWidth);

  if (isBot) return false;

  return innerLe(value, end);
}

bool SimpleInterval::isTop() const {
  if (isBot) {
    return false;
  } else {
    return end + 1 == begin;
  }
}

SimpleInterval SimpleInterval::_widen(SimpleInterval const& o) {
  if (isBot) return o;
  if (o.isBot) return *this;

  APInt incr = end - begin;;
  if (incr.uge(APInt::getSignedMaxValue(bitWidth))) {
    // Too large already, return true
    return SimpleInterval(true, bitWidth);
  } 

  // Widen the sides that changed
  SimpleInterval r = _leastUpperBound(o);
  int flags = (r.begin != begin) | (r.end != end) << 1;
  incr.ashrInPlace(flags == 3 ? 1 : 0);
  incr += incr.isNullValue(); // Always widen by at least 1
  if (flags & 1) r.begin -= incr;
  if (flags & 2) r.end   += incr;
  return r;
}

bool SimpleInterval::requiresWidening() {
  // This AD requires widening to ensure speedy termination
  return true;
}


llvm::raw_ostream &SimpleInterval::print(llvm::raw_ostream &os) {
  if (isBot) {
    os << "[]" << "_i" << bitWidth;
  } else if(isTop()) {
    os << "T" << "_i" << bitWidth;
  }
  else {
    os << "[" << begin.toString(10, false) << ", "
       << end.toString(10, false) << "]_i" << bitWidth;
  }
  return os;
}

void SimpleInterval::printOut() const {
  errs() << "SimpleInterval@" << debug_id << "=";
  if (isBot) {
    errs() << "[]" << "_" << bitWidth;
  } else if(isTop()) {
    errs() << "T" << "_" << bitWidth;
  }
  else {
    errs() << "[" << begin.toString(10, false) << ", "
           << end.toString(10, false) << "]_i" << bitWidth;
  }
  errs() << '\n';
}

} // namespace pcpo
