#include "vsa.h"

#include "api/vsa_result.h"
#include "fixpoint/vsa_visitor.h"
#include "fixpoint/worklist.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <queue>

#define DEBUG_TYPE "hello"

cl::opt<unsigned> CallStringDepth("cs_depth",
                                  cl::desc("Specify the call-string depth."),
                                  cl::value_desc("cs-depth"));

#ifndef VSA_STATIC
static RegisterPass<VsaPass> Y("vsapass",
                               "VSA Pass (with getAnalysisUsage implemented)");
#endif // VSA_STATIC
