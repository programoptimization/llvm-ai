#ifndef CALL_STRING_DOMAIN_H_
#define CALL_STRING_DOMAIN_H_

#include "../abstract_domain/AbstractDomain.h"
#include <memory>
#include <src/abstract_domain/AbstractDomain.h>

namespace pcpo {

using std::shared_ptr;

class CallStringDomain : public AbstractDomain {
public:
  /// Destructor
  virtual ~CallStringDomain() = default;

  /// Class info
  DomainType getDomainType() const override;

  /// Lattice interface
  bool operator<=(CallStringDomain &other) override;
  shared_ptr<CallStringDomain> leastUpperBound(CallStringDomain &other) override;

  shared_ptr<CallStringDomain> widen()  override;

  bool requiresWidening() override {
    // The default is that widening is neither supported nor required
    return false;
  }

  /// Binary Arithmetic Operations
  virtual shared_ptr<CallStringDomain>
  add(unsigned numBits, CallStringDomain &other, bool nuw, bool nsw) override;
  virtual shared_ptr<CallStringDomain>
  sub(unsigned numBits, CallStringDomain &other, bool nuw, bool nsw) override;
  virtual shared_ptr<CallStringDomain>
  mul(unsigned numBits, CallStringDomain &other, bool nuw, bool nsw) override;
  virtual shared_ptr<CallStringDomain> udiv(unsigned numBits,
                                          CallStringDomain &other) override;
  virtual shared_ptr<CallStringDomain> sdiv(unsigned numBits,
                                          CallStringDomain &other) override;
  virtual shared_ptr<CallStringDomain> urem(unsigned numBits,
                                          CallStringDomain &other) override;
  virtual shared_ptr<CallStringDomain> srem(unsigned numBits,
                                          CallStringDomain &other) override;

  /// Binary Bitwise Operations
  virtual shared_ptr<CallStringDomain>
  shl(unsigned numBits, CallStringDomain &other, bool nuw, bool nsw) override;
  virtual shared_ptr<CallStringDomain> lshr(unsigned numBits,
                                          CallStringDomain &other) override;
  virtual shared_ptr<CallStringDomain> ashr(unsigned numBits,
                                          CallStringDomain &other) override;
  virtual shared_ptr<CallStringDomain> and_(unsigned numBits,
                                          CallStringDomain &other) override;
  virtual shared_ptr<CallStringDomain> or_(unsigned numBits,
                                         CallStringDomain &other) override;
  virtual shared_ptr<CallStringDomain> xor_(unsigned numBits,
                                          CallStringDomain &other) override;

  /// Other operations
  virtual std::pair<shared_ptr<CallStringDomain>, shared_ptr<CallStringDomain>>
  icmp(CmpInst::Predicate pred, unsigned numBits, CallStringDomain &other) override;

  /// Member functions
  virtual bool contains(APInt &value) const override;
  virtual unsigned getBitWidth() const override;
  virtual bool isTop() const override;
  virtual bool isBottom() const override;
  virtual size_t size() const override;

  /// Member functions for API
  virtual APInt getValueAt(uint64_t i) const override;
  virtual APInt getUMin() const override;
  virtual APSInt getSMin() const override;
  virtual APInt getUMax() const override;
  virtual APSInt getSMax() const override;

  /// print
  virtual llvm::raw_ostream &print(llvm::raw_ostream &os) override;
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       CallStringDomain &bs) {
    return bs.print(os);
  }
  virtual void printOut() const override;
};
} // namespace pcpo

#endif // CALL_STRING_DOMAIN_H_
