#ifndef LLVM_REPRESENTATIVECOMPARE_H
#define LLVM_REPRESENTATIVECOMPARE_H

#include <memory>
#include <tuple>
#include "ClassType.h"
#include "Representative.h"
#include "Constant.h"
#include "Variable.h"

namespace bra {

    struct RepresentativeCompare {
        inline bool
        operator()(const std::shared_ptr<Representative> lhs, const std::shared_ptr<Representative> rhs) const {
            auto lType = lhs->getClassType();
            auto rType = rhs->getClassType();

            // Same type, order by attribute.
            if (lType == rType) {
                if (lType == RepresentativeType::Variable) {
                    const std::shared_ptr<Variable> lhsCast = std::static_pointer_cast<Variable>(lhs);
                    const std::shared_ptr<Variable> rhsCast = std::static_pointer_cast<Variable>(rhs);
                    return lhsCast->getName() < rhsCast->getName();
                }
                if (lType == RepresentativeType::Constant) {
                    const std::shared_ptr<Constant> lhsCast = std::static_pointer_cast<Constant>(lhs);
                    const std::shared_ptr<Constant> rhsCast = std::static_pointer_cast<Constant>(rhs);
                    return lhsCast->getValue() < rhsCast->getValue();
                }
            }
            // Different type, order Constant first
            return lType == RepresentativeType::Constant;
        }

        inline bool
        operator()(const std::tuple<std::shared_ptr<Representative>, std::shared_ptr<Representative>> lhs,
                   const std::tuple<std::shared_ptr<Representative>, std::shared_ptr<Representative>> rhs) const {
            std::shared_ptr<Representative> r11 = std::get<0>(lhs);
            std::shared_ptr<Representative> r12 = std::get<0>(rhs);
            std::shared_ptr<Representative> r21 = std::get<1>(lhs);
            std::shared_ptr<Representative> r22 = std::get<1>(rhs);
            return (r11->toString() + r21->toString()) < (r12->toString() + r22->toString());
        }
    };

}


#endif //LLVM_REPRESENTATIVECOMPARE_H
