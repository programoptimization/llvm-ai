#ifndef LLVM_REPRESENTATIVEHASHER_H
#define LLVM_REPRESENTATIVEHASHER_H

#include <functional>
#include "Representative.h"
#include "Variable.h"
#include "Constant.h"

namespace std {

    template<>
    struct hash<shared_ptr<bra::Representative>> {
        size_t operator()(const shared_ptr<bra::Representative>) const;
    };

    template<>
    struct hash<shared_ptr<bra::Variable>> {
        size_t operator()(const shared_ptr<bra::Variable>) const;
    };

}

#endif //LLVM_REPRESENTATIVEHASHER_H
