#ifndef EQUALITYDOMAIN_H
#define EQUALITYDOMAIN_H

#include <iostream>
#include <unordered_map>
#include <set>
#include <memory>
#include <map>
#include "../common/AbstractDomain.h"
#include "../common/Representative.h"
#include "../common/Variable.h"
#include "../common/Constant.h"
#include "../common/Invariant.h"
#include "../common/RepresentativeHasher.h"
#include "../common/RepresentativeCompare.h"

namespace bra {
    class EqualityDomain : public AbstractDomain {
    public:
        EqualityDomain();

        std::shared_ptr<AbstractDomain> copyEQ(std::shared_ptr<AbstractDomain> other);

        std::string toString() const override;

        std::string listInvariants() const override;

        std::shared_ptr<AbstractDomain> leastUpperBound(std::vector<std::shared_ptr<AbstractDomain>> domains) override;

        std::shared_ptr<AbstractDomain>
        leastUpperBound(std::shared_ptr<AbstractDomain>, std::shared_ptr<AbstractDomain>) override;

        std::shared_ptr<AbstractDomain> bottom() override;

        bool isBottom() override;

        /// Implementation of AbstractDomain virtual functions
        void transform_add(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                           std::shared_ptr<Representative> arg2) override;

        void transform_fadd(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                            std::shared_ptr<Representative> arg2) override;

        void transform_sub(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                           std::shared_ptr<Representative> arg2) override;

        void transform_fsub(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                            std::shared_ptr<Representative> arg2) override;

        void transform_mul(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                           std::shared_ptr<Representative> arg2) override;

        void transform_fmul(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                            std::shared_ptr<Representative> arg2) override;

        void transform_udiv(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                            std::shared_ptr<Representative> arg2) override;

        void transform_sdiv(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                            std::shared_ptr<Representative> arg2) override;

        void transform_fdiv(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                            std::shared_ptr<Representative> arg2) override;

        void transform_urem(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                            std::shared_ptr<Representative> arg2) override;

        void transform_srem(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                            std::shared_ptr<Representative> arg2) override;

        void transform_frem(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                            std::shared_ptr<Representative> arg2) override;

        void transform_store(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1) override;

        void transform_load(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1) override;

        std::vector<std::shared_ptr<Variable>> getAllVariables();

        DomainType getClassType() const override;

        /// Friend helper for stream output
        friend std::ostream &operator<<(std::ostream &, const EqualityDomain &);

        /// Helper operator
        bool operator==(const AbstractDomain &) const override;

        // TODO should be protected but is referenced in test code:
        //protected:
        void transformUnknownAssignment(const shared_ptr<Variable>);

        void transformConstantAssignment(std::shared_ptr<Variable>, std::shared_ptr<Constant>);

        void transformVariableAssignment(std::shared_ptr<Variable>, std::shared_ptr<Variable>);

    private:
        std::map<std::shared_ptr<Representative>,
                std::shared_ptr<std::set<std::shared_ptr<Variable>, RepresentativeCompare>>,
                RepresentativeCompare> forwardMap;

        std::map<std::shared_ptr<Variable>, std::shared_ptr<Representative>, RepresentativeCompare> backwardMap;

        /// Helper functions
        std::shared_ptr<Representative>
        chooseRepr(std::tuple<std::shared_ptr<Representative>, std::shared_ptr<Representative>>,
                   std::set<std::shared_ptr<Variable>, RepresentativeCompare>);

        void insertConstantIntoForwardMap(std::shared_ptr<Representative>, std::shared_ptr<Variable>);

        void insertConstantIntoBackwardMap(std::shared_ptr<Representative>, std::shared_ptr<Variable>);

        void
        addConstantAssignmentToDomain(const shared_ptr<Representative>, const shared_ptr<Variable>);

        void addVariableAssignmentToDomain(const shared_ptr<Variable>, const shared_ptr<Variable>);

        void removeVariableFromDomain(const shared_ptr<Variable>);

        /// Helper
        std::shared_ptr<Constant> getConstantIfResolvable(std::shared_ptr<Representative>) const;
    };

}

#endif //EQUALITYDOMAIN_H
