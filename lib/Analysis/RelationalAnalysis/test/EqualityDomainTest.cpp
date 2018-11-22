#include "gtest/gtest.h"
#include "../src/domains/EqualityDomain.h"
#include "../src/common/Constant.h"
#include "../src/common/Variable.h"

using namespace llvm;
using namespace bra;
using namespace std;

/**
 * Test if the constructor produces a well-formed domain
 */
TEST(EqTest, testEmptyEqualityDomain) {
    EqualityDomain eqd;
    ASSERT_EQ(eqd.toString(), "EqualityDomain (\n\tforwardMap {}\n  -> backwardMap {}");
}

/**
 * test if the constant assignment works properly
 */
TEST(EqTest, testTransformConstantAssignment) {
    EqualityDomain eqd;
    auto pConstant = make_shared<Constant>(42);
    auto pVar = make_shared<Variable>("x");
    eqd.transformConstantAssignment(pVar, pConstant);
    ASSERT_EQ(eqd.toString(), "EqualityDomain (\n\tforwardMap {(42: {x})}\n  -> backwardMap {(x, 42)}");
}

/**
 * Tests if the lub catches equalities between variables
 *
 * lub of {(42, {x})} and {(x, {x, y})} should yield {(42, {x, y})}
 */
TEST(EqTest, testLeastUpperBound) {
    auto eqd1 = make_shared<EqualityDomain>();
    auto eqd2 = make_shared<EqualityDomain>();
    auto pConstant = make_shared<Constant>(42);
    auto pVar = make_shared<Variable>("x");
    auto pVar2 = make_shared<Variable>("y");
    eqd1->transformConstantAssignment(pVar, pConstant);
    eqd2->transformVariableAssignment(pVar, pVar2);
    ASSERT_EQ(eqd1->leastUpperBound({eqd1, eqd2}).get()->toString(),
            "EqualityDomain (\n\tforwardMap {(42: {x, y})}\n  -> backwardMap {(x, 42), (x, y), (y, 42)}");
}

/**
 * Tests if the lub catches hidden equalities between variables
 *
 * E.g. lub of {(42, {x,y})} and {(5, {x,y})} should yield {(x,{x,y})}
 */
TEST(EqTest, testLeastUpperBound2) {
    auto eqd1 = make_shared<EqualityDomain>();
    auto eqd2 = make_shared<EqualityDomain>();
    auto pConstant = make_shared<Constant>(42);
    auto pVar = make_shared<Variable>("x");
    auto pConstant2 = make_shared<Constant>(5);
    auto pVar2 = make_shared<Variable>("y");

    eqd1->transformConstantAssignment(pVar, pConstant);
    eqd1->transformConstantAssignment(pVar2, pConstant);
    eqd2->transformConstantAssignment(pVar, pConstant2);
    eqd2->transformConstantAssignment(pVar2, pConstant2);
    ASSERT_EQ(eqd1->leastUpperBound({eqd1, eqd2}).get()->toString(),
              "EqualityDomain (\n\tforwardMap {(x: {x, y})}\n  -> backwardMap {(x, y), (y, x)}");
}

/**
 * Test if the bottom method returns a well-formed (empty) domain
 */
TEST(EqTest, testBottom) {
    auto eqd = make_shared<EqualityDomain>();
    ASSERT_EQ(eqd->bottom()->toString(), "EqualityDomain (\n\tforwardMap {}\n  -> backwardMap {}");
}