#ifndef LLVM_ABSTRACTDOMAIN_H
#define LLVM_ABSTRACTDOMAIN_H

#include <vector>
#include <memory>
#include <string>
#include "Variable.h"
#include "ClassType.h"

namespace bra {
    class AbstractDomain {
    public:
        /// Joins multiple domains by means of calculating the leastUpperBounds.
        /// Returns a new AbstractDomain representing the result
        ///
        /// this should never be called directly, instead the subclasses should implement this
        virtual std::string toString() const = 0;

        virtual std::string listInvariants() const = 0;

        virtual std::shared_ptr<AbstractDomain>
        leastUpperBound(std::vector<std::shared_ptr<AbstractDomain>> domains) = 0;

        virtual std::shared_ptr<AbstractDomain>
        leastUpperBound(std::shared_ptr<AbstractDomain>, std::shared_ptr<AbstractDomain>) = 0;

        AbstractDomain() {};

        virtual ~AbstractDomain() {};

        virtual DomainType getClassType() const = 0;

        virtual std::shared_ptr<AbstractDomain> bottom() = 0;

        virtual bool isBottom() = 0;

        /// Helper operator
        virtual bool operator==(const AbstractDomain &) const = 0;

        /// Interactions with visiting instructions go here. Return value indicates whether or not domain has been modified
        virtual void transform_add(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                   std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_fadd(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                    std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_sub(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                   std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_fsub(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                    std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_mul(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                   std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_fmul(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                    std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_udiv(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                    std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_sdiv(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                    std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_fdiv(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                    std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_urem(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                    std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_srem(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                    std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_frem(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                    std::shared_ptr<Representative> arg2) = 0;

        virtual void transform_store(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1) = 0;

        virtual void transform_load(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1) = 0;

    };
}

#endif //LLVM_ABSTRACTDOMAIN_H
