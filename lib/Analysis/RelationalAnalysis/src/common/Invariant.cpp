#include "Invariant.h"

using std::string;
using std::shared_ptr;
using std::set;

namespace bra {
    string Invariant::toString() {
        string ret;
        for (auto it = eqClass.cbegin(); it != eqClass.cend(); ++it) {
            ret += (*it)->toString();
            if (it != std::prev(eqClass.end())) {
                ret += " = ";
            }
        }
        return ret;
    }

    Invariant::Invariant(const set<shared_ptr<Representative>> &eqClass) : eqClass(eqClass) {}

    const set<shared_ptr<Representative>> &Invariant::getEqClass() const {
        return eqClass;
    }
}