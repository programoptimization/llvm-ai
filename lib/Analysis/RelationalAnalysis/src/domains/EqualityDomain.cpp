#include <algorithm>
#include <utility>
#include <string>
#include <memory>
#include <iostream>
#include <tuple>
#include <sstream>
#include "../common/ClassType.h"
#include "EqualityDomain.h"
#include "../util.h"
#include "../common/Representative.h"
#include "../common/RepresentativeCompare.h"

namespace bra {
    EqualityDomain::EqualityDomain() {}

    /**
     * Handle the assignment of an add operation.
     * If the operation can be resolved to two constants being added, we can use the information.
     * Otherwise it is treated like an unknown assignment.
     *
     * [x <- y + z]
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_add(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                       std::shared_ptr<Representative> arg2) {
        /// Try whether or not we're lucky and have 2 constants being added
        if (arg1->getClassType() == RepresentativeType::Constant &&
            arg2->getClassType() == RepresentativeType::Constant) {
            int result = ((Constant *) arg1.get())->getValue() + ((Constant *) arg2.get())->getValue();
            transformConstantAssignment(destination, std::make_shared<Constant>(result));
        } else {
            /// Try whether or not we can resolve both variables to constants. Otherwise this is a non trivial case
            std::shared_ptr<Constant> const1 = getConstantIfResolvable(arg1);
            std::shared_ptr<Constant> const2 = getConstantIfResolvable(arg2);

            if (const1 != nullptr && const2 != nullptr) {
                int result = const1->getValue() + const2->getValue();
                transformConstantAssignment(destination, std::make_shared<Constant>(result));
            } else {
                // TODO: implement non trivial av + b (if possible) -> unknown assignment for now
                transformUnknownAssignment(destination);
            }
        }
    }


    /**
     * Handle the assignment of an fadd operation. Currently treated as an unknown assignment.
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_fadd(shared_ptr<bra::Variable> destination, shared_ptr<bra::Representative> arg1,
                                        shared_ptr<bra::Representative> arg2) {
        // TODO: we currently can't deal with floating point math
        transformUnknownAssignment(destination);
    }


    /**
     * Handle the assignment of a sub operation. Currently treated as an unknown assignment.
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_sub(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1,
                                       std::shared_ptr<Representative> arg2) {
        /// Try whether or not we're lucky and have 2 constants being subbed
        if (arg1->getClassType() == RepresentativeType::Constant &&
            arg2->getClassType() == RepresentativeType::Constant) {
            int result = ((Constant *) arg1.get())->getValue() - ((Constant *) arg2.get())->getValue();
            transformConstantAssignment(destination, std::make_shared<Constant>(result));
        } else {
            /// Try whether or not we can resolve both variables to constants. Otherwise this is a non trivial case
            std::shared_ptr<Constant> const1 = getConstantIfResolvable(arg1);
            std::shared_ptr<Constant> const2 = getConstantIfResolvable(arg2);

            if (const1 != nullptr && const2 != nullptr) {
                int result = const1->getValue() - const2->getValue();
                transformConstantAssignment(destination, std::make_shared<Constant>(result));
            } else {
                // TODO: implement non trivial av - b (if possible) -> unknown assignment for now
                transformUnknownAssignment(destination);
            }
        }
    }


    /**
     * Handle the assignment of an fsub operation. Currently treated as an unknown assignment.
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_fsub(shared_ptr<bra::Variable> destination, shared_ptr<bra::Representative> arg1,
                                        shared_ptr<bra::Representative> arg2) {
        // TODO: we currently can't deal with floating point math
        transformUnknownAssignment(destination);
    }


    /**
     * Handle the assignment of a mul operation. Currently treated as an unknown assignment.
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_mul(shared_ptr<bra::Variable> destination, shared_ptr<bra::Representative> arg1,
                                       shared_ptr<bra::Representative> arg2) {
        /// Try whether or not we're lucky and have 2 constants being muled
        if (arg1->getClassType() == RepresentativeType::Constant &&
            arg2->getClassType() == RepresentativeType::Constant) {
            int result = ((Constant *) arg1.get())->getValue() * ((Constant *) arg2.get())->getValue();
            transformConstantAssignment(destination, std::make_shared<Constant>(result));
        } else {
            /// Try whether or not we can resolve both variables to constants. Otherwise this is a non trivial case
            std::shared_ptr<Constant> const1 = getConstantIfResolvable(arg1);
            std::shared_ptr<Constant> const2 = getConstantIfResolvable(arg2);

            if (const1 != nullptr && const2 != nullptr) {
                int result = const1->getValue() * const2->getValue();
                transformConstantAssignment(destination, std::make_shared<Constant>(result));
            } else {
                // TODO: implement non trivial av * b (if possible) -> unknown assignment for now
                transformUnknownAssignment(destination);
            }
        }
    }


    /**
     * Handle the assignment of an fmul operation. Currently treated as an unknown assignment.
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_fmul(shared_ptr<bra::Variable> destination, shared_ptr<bra::Representative> arg1,
                                        shared_ptr<bra::Representative> arg2) {
        // TODO: we currently can't deal with floating point math
        transformUnknownAssignment(destination);
    }


    /**
     * Handle the assignment of a udiv operation. Currently treated as an unknown assignment.
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_udiv(shared_ptr<bra::Variable> destination, shared_ptr<bra::Representative> arg1,
                                        shared_ptr<bra::Representative> arg2) {
        // TODO: we currently can't deal with floating point math
        transformUnknownAssignment(destination);
    }


    /**
     * Handle the assignment of an sdiv operation. Currently treated as an unknown assignment.
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_sdiv(shared_ptr<bra::Variable> destination, shared_ptr<bra::Representative> arg1,
                                        shared_ptr<bra::Representative> arg2) {
        // TODO: we currently can't deal with floating point math
        transformUnknownAssignment(destination);
    }


    /**
     * Handle the assignment of an fdiv operation. Currently treated as an unknown assignment.
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_fdiv(shared_ptr<bra::Variable> destination, shared_ptr<bra::Representative> arg1,
                                        shared_ptr<bra::Representative> arg2) {
        // TODO: we currently can't deal with floating point math
        transformUnknownAssignment(destination);
    }


    /**
     * Handle the assignment of a urem operation. Currently treated as an unknown assignment.
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_urem(shared_ptr<bra::Variable> destination, shared_ptr<bra::Representative> arg1,
                                        shared_ptr<bra::Representative> arg2) {
        // TODO: implement
        transformUnknownAssignment(destination);
    }

    /**
     * Handle the assignment of an srem operation. Currently treated as an unknown assignment.
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_srem(shared_ptr<bra::Variable> destination, shared_ptr<bra::Representative> arg1,
                                        shared_ptr<bra::Representative> arg2) {
        // TODO: implement
        transformUnknownAssignment(destination);
    }

    /**
     * Handle the assignment of a modulo operation. Currently treated as an unknown assignment.
     *
     * [x <- y%2]
     *
     * @param destination the variable being assigned to
     * @param arg1 the first operand
     * @param arg2 the second operand
     */
    void EqualityDomain::transform_frem(shared_ptr<bra::Variable> destination, shared_ptr<bra::Representative> arg1,
                                        shared_ptr<bra::Representative> arg2) {
        // TODO: we currently can't deal with floating point math
        transformUnknownAssignment(destination);
    }

    /**
     * Handle an assignment of some constant or another variable to a variable
     *
     * @param destination the variable being assigned to
     * @param arg1 the assigned value
     */
    void EqualityDomain::transform_store(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1) {
        if (arg1->getClassType() == RepresentativeType::Constant) {
            std::shared_ptr<Constant> con = std::static_pointer_cast<Constant>(arg1);
            transformConstantAssignment(destination, con);
        } else if (arg1->getClassType() == RepresentativeType::Variable) {
            std::shared_ptr<Variable> var = std::static_pointer_cast<Variable>(arg1);
            transformVariableAssignment(destination, var);
        }
    }

    /**
     * As of now, this is just an alias for transform_store(), as both are handled equally
     *
     * @param destination the variable being assigned to
     * @param arg1 the assigned value
     */
    void EqualityDomain::transform_load(std::shared_ptr<Variable> destination, std::shared_ptr<Representative> arg1) {
        transform_store(destination, arg1);
    }

    /**
     * Handle assignment of an unknown value to a variable : [x <- ?]
     *
     * @param variable (x)
     */
    void EqualityDomain::transformUnknownAssignment(const std::shared_ptr<Variable> variable) {
        // Do nothing (Single static assignment)
        DEBUG_OUTPUT(std::string(YELLOW)
                             +"[" + variable->toString() + " <- ?]#" + std::string(NO_COLOR));
    }

    /**
     * Handle assignment of a constant to a variable : [x <- c]
     *
     * @param variable (x)
     * @param constant (c)
     */
    void EqualityDomain::transformConstantAssignment(const std::shared_ptr<Variable> variable,
                                                     const std::shared_ptr<Constant> constant) {
        DEBUG_OUTPUT(std::string(YELLOW)
                             +"[" + variable->toString() + " <- '" + constant->toString() + "']#" +
                             std::string(NO_COLOR));
        this->addConstantAssignmentToDomain(constant, variable);
    }

    /**
     * Handle assignment of a variable to another : [x <- y]
     * @param variable (x)
     * @param assignedValue (y)
     */
    void EqualityDomain::transformVariableAssignment(const std::shared_ptr<Variable> variable,
                                                     const std::shared_ptr<Variable> assignedValue) {
        DEBUG_OUTPUT(std::string(YELLOW)
                             +"[" + variable->toString() + " <- " + assignedValue->toString() + "]#" +
                             std::string(NO_COLOR));
        return this->addVariableAssignmentToDomain(assignedValue, variable);
    }

    /**
     * Insert a constant assignment into the domain,
     * thereby replacing the variable it is assigned to, should it already exist.
     * [x <- c]
     *
     * @param key the constant being assigned to a variable (c)
     * @param var the variable being assigned to (x)
     */
    void EqualityDomain::addConstantAssignmentToDomain(const std::shared_ptr<Representative> key,
                                                       const std::shared_ptr<Variable> var) {
        removeVariableFromDomain(var);
        insertConstantIntoForwardMap(key, var);
        insertConstantIntoBackwardMap(key, var);
    }

    /**
     * Insert a variable assignment into the domain, replacing the destination variable if it already
     * exists there, otherwise creating a new eqClass
     *
     * This corresponds to the following transformation:
     * [x <- y]
     *
     * @param var the variable being assigned (y)
     * @param key the destination variable being assigned to (x)
     */
    void EqualityDomain::addVariableAssignmentToDomain(const std::shared_ptr<Variable> key,
                                                       const std::shared_ptr<Variable> var) {
        // TODO refactor this method as it is too large
        shared_ptr<set<shared_ptr<Variable>, RepresentativeCompare>> eqClass;

        // Find existing repr var if any
        auto itBackward = backwardMap.find(key);
        shared_ptr<Representative> reprVar = nullptr;
        shared_ptr<Variable> varToAdd = nullptr;
        if (itBackward != backwardMap.end()) {
            reprVar = itBackward->second;
            varToAdd = var;
        }

        if (reprVar != nullptr) {
            removeVariableFromDomain(varToAdd);

            // if there is an existing entry -> look for representative in forwardMap to obtain eqClass
            auto itForward = forwardMap.find(reprVar);
            if (reprVar->getClassType() == RepresentativeType::Constant) { // Constant case
                addConstantAssignmentToDomain(reprVar, varToAdd); // treat like a constant
            } else {
                // Insert into eq class in forwardMap
                eqClass = itForward->second;

                auto varIt = eqClass->find(varToAdd);
                if (varIt == eqClass->end()) {
                    // simply insert
                    eqClass->insert(varToAdd);

                    // Update representative for equality class in both maps
                    shared_ptr<Representative> newRepr = *eqClass->begin();
                    forwardMap.erase(reprVar);
                    forwardMap.insert({newRepr, eqClass});
                    for (auto it : *eqClass) {
                        backwardMap.erase(it);
                        backwardMap.insert({it, newRepr});
                    }
                }
            }
        } else {
            // Make sure that dest has no eq class yet
            removeVariableFromDomain(var);

            // Insert new eqClass into forward map
            auto newEqClass = std::make_shared<set<shared_ptr<Variable>, RepresentativeCompare>>();
            newEqClass->insert(key);
            newEqClass->insert(var);
            shared_ptr<Representative> newRepr = *newEqClass->begin();
            forwardMap.insert({newRepr, newEqClass}); //insert tuple to map

            // Insert into backward map
            for (auto it : *newEqClass) {
                backwardMap.insert({it, newRepr});
            }
        }
    }

    /**
     * Insert a constant into the forward map, becoming a new representative for its eqClass
     * (c, {x,y})
     * [x <- c]
     *
     * @param constant the constant to be inserted
     * @param var the variable the constant was assigned to
     */
    void EqualityDomain::insertConstantIntoForwardMap(const std::shared_ptr<Representative> constant,
                                                      const std::shared_ptr<Variable> var) {
        auto itForward = forwardMap.find(constant); //iterator for forwardMap
        std::shared_ptr<std::set<std::shared_ptr<Variable>, RepresentativeCompare>> eqClass; //shared_ptr on set (=equality class)

        if (itForward != forwardMap.end()) {
            // go through forwardMap if not empty
            eqClass = itForward->second; // get eq class
            auto varIt = eqClass->find(var);
            if (varIt == eqClass->end()) {
                eqClass->insert(var); // add variable to eq class
            }
        } else {
            // No EQ class found
            std::set<std::shared_ptr<Variable>, RepresentativeCompare> newSet;
            newSet.insert(var);
            eqClass = std::make_shared<std::set<std::shared_ptr<Variable>, RepresentativeCompare>>(newSet);
            forwardMap.insert({constant, eqClass}); // insert tuple to map
        }
    }

    /**
     * Insert a constant into the backward map, mapping the variable to it
     * (x, {c})
     * @param constant the constant to insert into the eqClass
     * @param var the key variable of the eqClass
     */
    void EqualityDomain::insertConstantIntoBackwardMap(const std::shared_ptr<Representative> constant,
                                                       const std::shared_ptr<Variable> var) {
        auto itBackward = backwardMap.find(var);
        if (itBackward != backwardMap.end()) {
            // if variable is already in map -> overwrite value
            const RepresentativeCompare c;
            if (!c.operator()(itBackward->second, constant)) {
                itBackward->second = constant;
            }
        } else {
            // otherwise add new pair
            backwardMap.insert({var, constant});
        }
    }

    /**
     * Remove a variable from the domain
     *
     * @param var the variable to remove
     */
    void EqualityDomain::removeVariableFromDomain(const std::shared_ptr<Variable> var) {
        auto bwmIt = backwardMap.find(var);
        if (bwmIt == backwardMap.end())
            return;

        //look for representative and remove from backwardMap
        std::shared_ptr<Representative> eqRepr = bwmIt->second;
        backwardMap.erase(var);

        //erase from forwardMap
        std::shared_ptr<std::set<std::shared_ptr<Variable>, RepresentativeCompare>> eqClass = forwardMap.find(
                eqRepr)->second;//find respective eq class

        eqClass->erase(var);
        if (eqClass->empty()) {//if this was last element in set -> remove from map
            forwardMap.erase(eqRepr);
        } else if (eqRepr ==
                   var) { //if current repr is not in the eqClass anymore -> replace with first element in eqClass
            const std::shared_ptr<Variable> &repr = eqClass->begin().operator*();
            forwardMap.erase(eqRepr);
            forwardMap.insert({repr, eqClass});
            for (const std::shared_ptr<Variable> &eqMember : *eqClass) {
                auto it = backwardMap.find(eqMember);
                if (it != backwardMap.end()) {
                    it->second = repr;
                } else {
                    backwardMap.insert({eqMember, repr});
                }
            }
        }
    }

    /**
     * Helper method that tries to find mapped constant for a representative
     *
     * @param rep the representative to look up
     * @return a shared_ptr to the mapped constant, or nullptr
     */
    std::shared_ptr<Constant> EqualityDomain::getConstantIfResolvable(std::shared_ptr<Representative> rep) const {
        if (rep->getClassType() == RepresentativeType::Variable) {
            // Try to resolve variable
            auto var = std::static_pointer_cast<Variable>(rep);

            // Search for variable in backwardMap:
            auto it = backwardMap.find(var);
            if (it != backwardMap.end()) {
                // Check if it->second is a constant. if it is, return that and else null_ptr
                if (it->second->getClassType() == RepresentativeType::Constant) {
                    return std::static_pointer_cast<Constant>(it->second);
                }
            }
        } else if (rep->getClassType() == RepresentativeType::Constant) {
            return std::static_pointer_cast<Constant>(rep);
        }

        // Otherwise we can't resolve to constant

        return nullptr;
    }

    /**
     * Return the set of Variables in this domain
     *
     * @return a vector containing shared pointers to the variables in this domain
     */
    std::vector<std::shared_ptr<Variable>> EqualityDomain::getAllVariables() {
        std::vector<std::shared_ptr<Variable>> res;

        for (auto &it : backwardMap) {
            res.push_back(it.first);
        }

        return res;
    }

    /// Human readable output (f.e. DEBUG)
    std::ostream &operator<<(std::ostream &stream, const EqualityDomain &dom) {
        stream << dom.toString();
        return stream;
    }

    /**
     * Construct a string representation of this domain
     *
     * @return the string representing this domain
     */
    std::string EqualityDomain::toString() const {
        std::string ret = "{";
        for (auto tmp = this->forwardMap.begin(); tmp != this->forwardMap.end(); tmp++) {
            ret += "(" + (tmp->first->toString()) + ": {";
            for (auto var = tmp->second->begin(); var != tmp->second->end(); var++) {
                ret += (*var)->toString();
                if (std::next(var) != tmp->second->end()) {
                    ret += ", ";
                }
            }
            ret += "})";
            if (std::next(tmp) != this->forwardMap.end()) {
                ret += ", ";
            }
        }
        return ret + "}";
    }

    /**
     * Return a domain representing the bottom lattice, i.e. a domain that contains no information on equalities
     *
     * @return the constructed domain
     */
    std::shared_ptr<AbstractDomain> EqualityDomain::bottom() {
        // TODO make this a static method somehow
        return std::make_shared<EqualityDomain>();
    }

    /**
     * Return whether this domain is equal to ⟂ (both maps are empty)
     *
     * @return true if this domain is equal ⟂, false otherwise
     */
    bool EqualityDomain::isBottom() {
        // TODO there should never be a case where only one map is empty. throw an exception?
        return backwardMap.empty() && forwardMap.empty();
    }

    DomainType EqualityDomain::getClassType() const {
        return DomainType::EqualityDomain;
    }

    /**
     * copy a given AbstractDomain and return a reference to it. The given Domain will be cast to EqualityDomain first.
     * @param other an AbstractDomain to copy
     * @return a shared pointer to the copied domain
     */
    std::shared_ptr<AbstractDomain> EqualityDomain::copyEQ(std::shared_ptr<AbstractDomain> other) {
        // TODO
        // - does it make sense to copy an abstract domain here? The code below expects an equalitydomain
        // - this should be a static method
        // - does this method really need to return a shared_ptr?
        std::shared_ptr<EqualityDomain> dom = std::static_pointer_cast<EqualityDomain>(other);
        std::shared_ptr<EqualityDomain> copy = std::make_shared<EqualityDomain>();
        copy->backwardMap = dom->backwardMap;
        copy->forwardMap = dom->forwardMap;
        for (auto it = copy->forwardMap.begin(); it != copy->forwardMap.end(); it++) {
            /// Copy constructor
            std::shared_ptr<std::set<std::shared_ptr<Variable>, RepresentativeCompare>> cop = std::make_shared<std::set<std::shared_ptr<Variable>, RepresentativeCompare>>();
            for (auto varp : *it->second) {
                cop->insert(varp);
            }
            it->second = cop;
        }
        return std::static_pointer_cast<AbstractDomain>(copy);
    }

    /**
     * Construct the least upper bound w.r.t. the join operation for a given set of domains
     *
     * @param domains the domains to join
     * @return a domain representing the least upper bound of the input domains
     */
    std::shared_ptr<AbstractDomain>
    EqualityDomain::leastUpperBound(std::vector<std::shared_ptr<AbstractDomain>> domains) {
        // TODO make this a static method somehow
        if (domains.size() == 0) {
            return bottom();
        }

        // Use associativity
        // TODO: refactor into copyEQDom function -> copy constructor??
        std::shared_ptr<AbstractDomain> res = copyEQ(domains[0]);
        for (auto domIt = ++domains.begin(); domIt != domains.end(); domIt++) {
            res = leastUpperBound(res, *domIt);
        }

        return res;
    }

    /**
     * Construct the least upper bound w.r.t. the join operation for a given set of domains
     *
     * @param domains the domains to join
     * @return a domain representing the least upper bound of the input domains
     */
    std::shared_ptr<AbstractDomain>
    EqualityDomain::leastUpperBound(std::shared_ptr<AbstractDomain> d1, std::shared_ptr<AbstractDomain> d2) {
        // TODO refactor, this method is too complex
        // TODO make this a static method somehow
        if (d1->getClassType() != DomainType::EqualityDomain || d2->getClassType() != DomainType::EqualityDomain) {
            // TODO: probably should throw runtime error
            DEBUG_ERR("Can not calculate leastUpperBounds of non equality domains");
            return d1->bottom();
        }

        std::shared_ptr<EqualityDomain> dom1 = std::static_pointer_cast<EqualityDomain>(d1);
        std::shared_ptr<EqualityDomain> dom2 = std::static_pointer_cast<EqualityDomain>(d2);

        if (d1->isBottom()) return copyEQ(d2);
        if (d2->isBottom()) return copyEQ(d1);

        // Step 1: find all variables
        std::vector<std::shared_ptr<Variable>> variables1 = dom1->getAllVariables();
        std::vector<std::shared_ptr<Variable>> variables2 = dom2->getAllVariables();
        std::set<std::shared_ptr<Variable>> variables(variables1.begin(), variables1.end());
        variables.insert(variables2.begin(), variables2.end());

        // Step 2: find pairs (t1,t2) of eqClass representatives for each variable and group variables with matching pairs together
        std::map<std::tuple<std::shared_ptr<Representative>, std::shared_ptr<Representative>>, std::shared_ptr<std::set<std::shared_ptr<Variable>, RepresentativeCompare>>, RepresentativeCompare> t1t2Mapping;
        for (std::shared_ptr<Variable> var : variables) {
            auto t1It = dom1->backwardMap.find(var);
            auto t2It = dom2->backwardMap.find(var);

            std::shared_ptr<Representative> t1 =
                    t1It == dom1->backwardMap.end() ? std::static_pointer_cast<Representative>(var) : t1It->second;
            std::shared_ptr<Representative> t2 =
                    t2It == dom2->backwardMap.end() ? std::static_pointer_cast<Representative>(var) : t2It->second;

            std::tuple<std::shared_ptr<Representative>, std::shared_ptr<Representative>> tuple = std::make_tuple(t1,
                                                                                                                 t2);
            std::shared_ptr<std::set<std::shared_ptr<Variable>, RepresentativeCompare>> vars;
            auto varsIt = t1t2Mapping.find(tuple);
            if (varsIt == t1t2Mapping.end()) {
                vars = std::make_shared<std::set<std::shared_ptr<Variable>, RepresentativeCompare>>();
                t1t2Mapping.insert({tuple, vars});
            } else {
                vars = varsIt->second;
            }
            vars->insert(var);
        }

        // Step 3: filter tautology classes (t_5 = t_5)
        // NOTE: before continuing to read this code, buckle up, place your monitor out of punching range and
        // make sure to wear your eye protecting glasses: This code WILL cause suffering and pain
        // TODO: prettify/help ._.
        label:
        for (auto it : t1t2Mapping) {
            std::shared_ptr<Representative> t1 = std::get<0>(it.first);
            std::shared_ptr<Representative> t2 = std::get<1>(it.first);

            if (t1->getClassType() == RepresentativeType::Constant &&
                t2->getClassType() == RepresentativeType::Constant) {
                std::shared_ptr<Constant> c1 = std::static_pointer_cast<Constant>(t1);
                std::shared_ptr<Constant> c2 = std::static_pointer_cast<Constant>(t2);

                if (c1->getValue() == c2->getValue())
                    continue;
            }

            if (it.second->size() == 1) {
                t1t2Mapping.erase(it.first);
                goto label;
            }
        }


        // Step 4: generate new Domain with equality classes from those pairs
        // TODO: use bottom() to generate new domain
        std::shared_ptr<EqualityDomain> resDom = std::make_shared<EqualityDomain>();
        for (auto it = t1t2Mapping.begin(); it != t1t2Mapping.end(); it++) {
            // Find representative (either both are the same constant, or a new repr has to be chosen)
            std::shared_ptr<std::set<std::shared_ptr<Variable>, RepresentativeCompare>> eqClass = it->second;
            std::shared_ptr<Representative> newRepr = chooseRepr(it->first, *eqClass);

            resDom->forwardMap.insert({newRepr, eqClass});
            for (auto var : *eqClass) {
                resDom->backwardMap.insert({var, newRepr});
            }
        }


        return resDom;
    }

    /**
     * Choose a representative from an eqClass. Only called when calculating the least upper bound of two domains.
     * @param tuple a tuple of representatives from the two domains. If both are constants and equal, they make up
     *  the new representative
     * @param eqClass the eqClass constructed from the joined domains. If no constant representative can be identified,
     *  the first variable in the class will be used as the new representative
     * @return
     *  the new representative for the eqClass
     */
    std::shared_ptr<Representative>
    EqualityDomain::chooseRepr(std::tuple<std::shared_ptr<Representative>, std::shared_ptr<Representative>> tuple,
                               std::set<std::shared_ptr<Variable>, RepresentativeCompare> eqClass) {
        std::shared_ptr<Representative> repr1 = std::get<0>(tuple);
        std::shared_ptr<Representative> repr2 = std::get<1>(tuple);

        if (repr1->getClassType() == RepresentativeType::Constant &&
            repr2->getClassType() == RepresentativeType::Constant) {
            std::shared_ptr<Constant> c1 = std::static_pointer_cast<Constant>(repr1);
            std::shared_ptr<Constant> c2 = std::static_pointer_cast<Constant>(repr2);

            // TODO: reimplement comparator and use that
            if (c1->getValue() == c2->getValue()) {
                return std::static_pointer_cast<Representative>(c1);
            }
        }

        // Choose a repr from eqClass
        return std::static_pointer_cast<Representative>(*eqClass.begin());
    }

    /**
     * Construct a string containing all the equivalence classes (vlg. Invariants) of the domain
     *
     * @return the constructed string
     */
    std::string EqualityDomain::listInvariants() const {
        if (forwardMap.size() == 0) {
            return "|{Invariants: ⟂}";
        }

        std::string ret = "|{Invariants:";
        for (auto it = forwardMap.cbegin(); it != forwardMap.cend(); it++) {
            ret += "}|{" + it->first->toDotString();
            auto eqIt = it->second->begin();
            if (it->first->getClassType() == RepresentativeType::Variable) {
                eqIt = std::next(eqIt);
            }
            for (; eqIt != it->second->end(); eqIt++) {
                std::shared_ptr<Variable> var = std::static_pointer_cast<Variable>(*eqIt);
                ret += " = " + var->toDotString();
            }
        }

        return ret + "}";
    }


    /**
     * Compare for equality with another domain
     *
     * @param other another domain to compare this with
     * @return true if both domains are equal, false otherwise
     */
    bool EqualityDomain::operator==(const AbstractDomain &other) const {
        if (other.getClassType() != DomainType::EqualityDomain)
            return false;

        const EqualityDomain &otherEQ = (const EqualityDomain &) other;
        if (otherEQ.backwardMap.size() != this->backwardMap.size())
            return false;

        /// Since backwardMap and forwardMap are kept in sync, it should suffice to compare backward maps
        for (auto mapping : backwardMap) {
            // Find mapping with same repr in other.backwardMap
            auto it = otherEQ.backwardMap.find(mapping.first);
            if (it == otherEQ.backwardMap.end())
                return false;
            if (!(*it->second == *mapping.second))
                return false;
        }

        return true;
    }
}
