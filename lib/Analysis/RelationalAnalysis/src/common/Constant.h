//===----------------------------------------------------------------------===//
//
// Implementation of lib/Analysis/RelationalAnalysis/src/common/Representative.h
// A constant has a specific int value
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CONSTANT_H
#define LLVM_CONSTANT_H

#include <functional>
#include <memory>
#include "Representative.h"

namespace bra {

    enum class RepresentativeType;

    class Variable;

    class Constant : public Representative {
    public:
        Constant(int value);

        ~Constant() {};

        int getValue() const;

        RepresentativeType getClassType() const override;

        size_t hash() const override;

        friend std::ostream &operator<<(std::ostream &, const std::shared_ptr<Constant>);


        std::string toString() const override;

        std::string toDotString() const override;

        int id = 2;

        /// Helper comparator
        bool operator==(const Representative &) const override;

    private:
        int value;
    };

}

#endif //LLVM_CONSTANT_H
