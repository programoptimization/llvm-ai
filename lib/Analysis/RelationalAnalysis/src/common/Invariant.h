#ifndef LLVM_INVARIANT_H
#define LLVM_INVARIANT_H

#include <set>
#include "Representative.h"
#include "AbstractDomain.h"

using std::string;
using std::set;
using std::shared_ptr;

namespace bra {
    class Invariant {
    private:
        set<shared_ptr<Representative>> eqClass;
    public:
        string toString();
        explicit Invariant(const set<shared_ptr<Representative>> &eqClass);
        const set<shared_ptr<Representative>> &getEqClass() const;
    };
}


#endif //LLVM_INVARIANT_H
