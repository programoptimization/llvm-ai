#include "value_set.h"

namespace pcpo {

char const* get_predicate_name(llvm::CmpInst::Predicate pred) {
    using Predicate = llvm::CmpInst::Predicate;
    switch (pred) {
    case Predicate::ICMP_EQ: return "==";
    case Predicate::ICMP_NE: return "!=";
    case Predicate::ICMP_ULE: return "u<=";
    case Predicate::ICMP_ULT: return "u<";
    case Predicate::ICMP_UGE: return "u>=";
    case Predicate::ICMP_UGT: return "u>";
    case Predicate::ICMP_SLE: return "s<=";
    case Predicate::ICMP_SLT: return "s<";
    case Predicate::ICMP_SGE: return "s>=";
    case Predicate::ICMP_SGT: return "s>";
    default: return "??";
    }
}

} /* end of namespace pcpo */
