#include <string>
#include "RepresentativeHasher.h"
#include "../util.h"

using namespace std;

size_t std::hash<shared_ptr<bra::Representative>>::operator()(const std::shared_ptr<bra::Representative> repr) const {
    return repr->hash();
}

size_t std::hash<std::shared_ptr<bra::Variable>>::operator()(const std::shared_ptr<bra::Variable> var) const {
    return var->hash();
}
