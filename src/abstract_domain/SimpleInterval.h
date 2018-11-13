#ifndef SIMPLE_INTERVAL_H_
#define SIMPLE_INTERVAL_H_
#include <cstdint>
#include <llvm/ADT/APInt.h>
#include <functional>
#include <iostream>
#include <set>
#include "AbstractDomain.h"
#include "Util.h"
#include "BoundedSet.h"

namespace pcpo {
using llvm::APInt;

class SimpleInterval : public AbstractDomain {
private:
  void debug_break() {
    // You can set a breakpoint here
    debug_id = debug_id;
  }
  
public:

  /// Constructor: Bottom
  SimpleInterval() : isBot(true), debug_id(++debug_id_gen) {debug_break();}
  /// Constructor: Top
  SimpleInterval(bool isTop, unsigned bitWidth);
  /// Constructor: Constant
  SimpleInterval(APInt value);
  /// Constructor: Interval with APInt
  SimpleInterval(APInt begin, APInt end);
  /// Constructor: Interval with uint64_t
  SimpleInterval(unsigned bitWidth, uint64_t begin, uint64_t end);
  //SimpleInterval(unsigned numBits, std::initializer_list<uint64_t> vals);
  /// Constructor: From BoundedSet
  SimpleInterval(BoundedSet &set);

  /// Bottom SI
  static shared_ptr<AbstractDomain> create_bottom(unsigned bitWidth)
      { return std::shared_ptr<AbstractDomain>(new SimpleInterval(false, bitWidth)); }
  /// Top SI
  static shared_ptr<AbstractDomain> create_top(unsigned bitWidth)
      { return std::shared_ptr<AbstractDomain>(new SimpleInterval(true, bitWidth)); }

  /// Copy constructor
  SimpleInterval(const SimpleInterval& other);
  /// Copy assignment
  SimpleInterval& operator=(const SimpleInterval& other) = default;

  /// Comparison Operators
  bool operator==(const SimpleInterval &other);
  bool operator!=(const SimpleInterval &other) {return !(operator==(other));}
  bool operator<=(AbstractDomain &other);

  /// Binary Arithmetic Operations
  shared_ptr<AbstractDomain> add(unsigned numBits, AbstractDomain &other, bool nuw, bool nsw);
  shared_ptr<AbstractDomain> sub(unsigned numBits, AbstractDomain &other, bool nuw, bool nsw);
  shared_ptr<AbstractDomain> mul(unsigned numBits, AbstractDomain &other, bool nuw, bool nsw);
  shared_ptr<AbstractDomain> udiv(unsigned numBits, AbstractDomain &other);
  shared_ptr<AbstractDomain> sdiv(unsigned numBits, AbstractDomain &other);
  shared_ptr<AbstractDomain> urem(unsigned numBits, AbstractDomain &other);
  shared_ptr<AbstractDomain> srem(unsigned numBits, AbstractDomain &other);

  /// Binary Bitwise Operations
  shared_ptr<AbstractDomain> shl(unsigned numBits, AbstractDomain &other, bool nuw, bool nsw);
  shared_ptr<AbstractDomain> lshr(unsigned numBits, AbstractDomain &other);
  shared_ptr<AbstractDomain> ashr(unsigned numBits, AbstractDomain &other);
  shared_ptr<AbstractDomain> and_(unsigned numBits, AbstractDomain &other);
  shared_ptr<AbstractDomain> or_(unsigned numBits, AbstractDomain &other);
  shared_ptr<AbstractDomain> xor_(unsigned numBits, AbstractDomain &other);

  /// Other operations:
  /// Subsets for predicate
  std::pair<shared_ptr<AbstractDomain>, shared_ptr<AbstractDomain>>
  subsetsForPredicate(
          AbstractDomain &other,
          CmpInst::Predicate pred);

  /// Functions for icmp predicates
  std::pair<shared_ptr<AbstractDomain>, shared_ptr<AbstractDomain>>
  subsetsForPredicateEQ(SimpleInterval &A, SimpleInterval &B);
  std::pair<shared_ptr<AbstractDomain>, shared_ptr<AbstractDomain>>
  subsetsForPredicateSLE(SimpleInterval &A, SimpleInterval &B);
  std::pair<shared_ptr<AbstractDomain>, shared_ptr<AbstractDomain>>
  subsetsForPredicateSLT(SimpleInterval &A, SimpleInterval &B);
  std::pair<shared_ptr<AbstractDomain>, shared_ptr<AbstractDomain>>
  subsetsForPredicateULE(SimpleInterval &A, SimpleInterval &B);
  std::pair<shared_ptr<AbstractDomain>, shared_ptr<AbstractDomain>>
  subsetsForPredicateULT(SimpleInterval &A, SimpleInterval &B);

  /// icmp
  std::pair<shared_ptr<AbstractDomain>, shared_ptr<AbstractDomain>>
  icmp(CmpInst::Predicate pred, unsigned numBits, AbstractDomain &other);

  // Check whether this is a wrap around interval
  bool isWrapAround() const;

  // Conduct an overapproximated intersection of two intervals.
  //shared_ptr<AbstractDomain> intersectWithBounds(const SimpleInterval &first,
  //                                               const SimpleInterval &second);

  virtual shared_ptr<AbstractDomain> leastUpperBound(AbstractDomain& other);
  virtual shared_ptr<AbstractDomain> intersect(AbstractDomain& other);
  virtual shared_ptr<AbstractDomain> widen(AbstractDomain& other);

  DomainType getDomainType() const { return simpleInterval; };

  virtual bool requiresWidening();

  /// Member functions
  unsigned getBitWidth() const { return bitWidth; }
  bool isTop() const;
  bool isBottom() const { return isBot; }
  bool contains(APInt value) const;
  size_t size() const;

  /// Member functions for API
  APInt getValueAt(uint64_t i) const { return begin + i; }
  APInt getUMin() const { return umin(); }
  APSInt getSMin() const { return APSInt(smin(),false); }
  APInt getUMax() const { return umax(); }
  APSInt getSMax() const { return APSInt(smax(),false); }

  /// Print
  friend std::ostream &operator<<(std::ostream &os, const SimpleInterval &bs);
  virtual llvm::raw_ostream &print(llvm::raw_ostream &os);
  void printOut() const;

private:
  friend void testSimpleDomain(uint32_t, uint32_t, uint64_t*);
    
  SimpleInterval _add (SimpleInterval const& o, bool nuw, bool nsw);
  SimpleInterval _sub (SimpleInterval const& o, bool nuw, bool nsw);
  SimpleInterval _mul (SimpleInterval const& o, bool nuw, bool nsw);
  SimpleInterval _udiv(SimpleInterval const& o);
  SimpleInterval _urem(SimpleInterval const& o);
  SimpleInterval _srem(SimpleInterval const& o);
  SimpleInterval _leastUpperBound(SimpleInterval const& o);
  SimpleInterval _intersect(SimpleInterval const& o);
  SimpleInterval _widen(SimpleInterval const& o);
  static void _icmp(CmpInst::Predicate pred, SimpleInterval a1, SimpleInterval a2, SimpleInterval* r1, SimpleInterval* r2);
  static SimpleInterval _icmp_ule_val(SimpleInterval a, APInt v);
  static SimpleInterval _icmp_ult_val(SimpleInterval a, APInt v);
  static SimpleInterval _icmp_neg(SimpleInterval a);
  static SimpleInterval _icmp_inv(SimpleInterval a);
  static SimpleInterval _icmp_shift(SimpleInterval a);

  APInt umax() const;
  APInt umin() const;
  APInt smax() const;
  APInt smaxabsneg() const;
  APInt smin() const;

  bool innerLe(APInt a, APInt b) const;
  
  unsigned bitWidth;
  APInt begin;
  APInt end;
  bool isBot;
  unsigned debug_id;
  static unsigned debug_id_gen;
};

} // namespace pcpo
#endif
