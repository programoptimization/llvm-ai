#include "fixpoint.h"

#include <unordered_map>
#include <vector>

#include "llvm/IR/CFG.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Dominators.h"
#include <llvm/Analysis/LoopInfo.h>


#include "global.h"
#include "value_set.h"
#include "simple_interval.h"

namespace pcpo {

// @Cleanup: getAnalysisUsage ??

static llvm::RegisterPass<AbstractInterpretationPass> Y("painpass", "AbstractInterpretation Pass");

char AbstractInterpretationPass::ID;

int debug_level = DEBUG_LEVEL; // from global.hpp

class AbstractStateDummy {
public:
    enum MergeOperation: int {
        INVALID,
        SMALL_UPPER_BOUND,
        WIDEN,
        NARROW
    };

    // This has to initialise the state to bottom.
    AbstractStateDummy() = default;

    // Creates a copy of the state. Using the default copy-constructor should be fine here, but if
    // some members do something weird you maybe want to implement this as
    //     AbstractState().merge(state)
    AbstractStateDummy(AbstractStateDummy const& state) = default;

    // Initialise the state to the incoming state of the function. This should do something like
    // assuming the parameters can be anything.
    explicit AbstractStateDummy(llvm::Function& f) {}

    // Applies the changes needed to reflect executing the instructions in the basic block. Before
    // this operation is called, the state is the one upon entering bb, afterwards it should be (an
    // upper bound of) the state leaving the basic block.
    //  predecessors contains the outgoing state for all the predecessors, in the same order as they
    // are listed in llvm::predecessors(bb).
    void apply(llvm::BasicBlock& bb, std::vector<AbstractStateDummy> const& predecessors) {};

    // After this operation the state should contain an upper bound of both its previous value and
    // other. In other words, this computes (an upper bound of) the union of *this and other in
    // place. (I would have called it union, if that were possible.) Returns whether the state
    // changed as a result.
    bool merge(AbstractStateDummy const& other, MergeOperation op) { return false; };

    // Restrict the set of values to the one that allows 'from' to branch towards
    // 'towards'. Starting with the state when exiting from, this should compute (an upper bound of)
    // the possible values that would reach the block towards. Doing nothing thus is a valid implementation.
    void branch(llvm::BasicBlock& from, llvm::BasicBlock& towards) {};

    // @Cleanup Documentation
    void printIncoming(llvm::BasicBlock& bb, llvm::raw_ostream& out, int indentation = 0) const {};
    void printOutgoing(llvm::BasicBlock& bb, llvm::raw_ostream& out, int indentation = 0) const {};
};

template <typename AbstractState>
void executeFixpointAlgorithm(llvm::Module& M) {
    struct Node {
        int id;
        llvm::BasicBlock* bb;
        AbstractState state;
        bool update_scheduled = false; // Whether the node is already in the worklist

        // If this is set, the algorithm will add the initial values from the parameters of the
        // function to the incoming values, which is the correct thing to do for initial basic
        // blocks.
        llvm::Function* func_entry = nullptr;
    };

    std::vector<Node> nodes;
    std::unordered_map<llvm::BasicBlock*, int> nodeIdMap; // Maps basic blocks to the ids of their corresponding nodes
    std::vector<int> worklist; // Contains the ids of nodes that need to be processed

    // TODO: Check what this does for release clang, probably write out a warning
    dbgs(1) << "Initialising fixpoint algorithm, collecting basic blocks\n";

    for (llvm::Function& f: M.functions()) {
        // Check for external (i.e. declared but not defined) functions
        if (f.empty()) {
            dbgs(1) << "  Function " << f.getName() << " is external, skipping...";
            continue;
        }

        // Register basic blocks
        for (llvm::BasicBlock& bb: f) {
            dbgs(1) << "  Found basic block " << bb.getName() << '\n';

            Node node;
            node.id = nodes.size(); // Assign new id
            node.bb = &bb;
            // node.state is default initialised (to bottom)
            nodeIdMap[node.bb] = node.id;
            nodes.push_back(node);
        }

        // Push the initial block into the worklist
        int entry_id = nodeIdMap.at(&f.getEntryBlock());
        worklist.push_back(entry_id);
        nodes[entry_id].update_scheduled = true;
        nodes[entry_id].func_entry = &f;
    }

    dbgs(1) << "\nWorklist initialised with " << worklist.size() << (worklist.size() != 1 ? " entries" : " entry")
            << ". Starting fixpoint iteration...\n";

    for (int iter = 0; !worklist.empty(); ++iter) {
        Node& node = nodes[worklist.back()];
        worklist.pop_back();
        node.update_scheduled = false;

        dbgs(1) << "\nIteration " << iter << ", considering basic block " << node.bb->getName() << '\n';

        AbstractState state_new; // Set to bottom

        if (node.func_entry) {
            dbgs(1) << "  Merging function parameters, is entry block\n";

            AbstractState state_entry {*node.func_entry};
            state_new.merge(state_entry);
        }

        dbgs(1) << "  Merge of " << llvm::pred_size(node.bb)
                << (llvm::pred_size(node.bb) != 1 ? " predecessors.\n" : " predecessor.\n");

        // Collect the predecessors
        std::vector<AbstractState> predecessors;
        for (llvm::BasicBlock* bb: llvm::predecessors(node.bb)) {
            dbgs(3) << "    Merging basic block " << bb->getName() << '\n';

            AbstractState state_branched {nodes[nodeIdMap[bb]].state};
            state_branched.branch(*bb, *node.bb);
            state_new.merge(state_branched);
            predecessors.push_back(state_branched);
        }

        dbgs(2) << "  Relevant incoming state is:\n"; state_new.printIncoming(*node.bb, dbgs(2), 4);

        // Apply the basic block
        dbgs(3) << "  Applying basic block\n";
        state_new.apply(*node.bb, predecessors);

        // Merge the state back into the node
        dbgs(3) << "  Merging with stored state\n";

        bool changed = node.state.merge(state_new);

        dbgs(2) << "  Outgoing state is:\n"; state_new.printOutgoing(*node.bb, dbgs(2), 2);

        // No changes, so no need to do anything else
        if (not changed) continue;

        dbgs(2) << "  State changed, notifying " << llvm::succ_size(node.bb)
                << (llvm::succ_size(node.bb) != 1 ? " successors\n" : " successor\n");

        // Something changed and we will need to update the successors
        for (llvm::BasicBlock* succ_bb: llvm::successors(node.bb)) {
            Node& succ = nodes[nodeIdMap[succ_bb]];
            if (not succ.update_scheduled) {
                worklist.push_back(succ.id);
                succ.update_scheduled = true;

                dbgs(3) << "    Adding " << succ_bb->getName() << " to worklist\n";
            }
        }
    }

    // Output the final result
    dbgs(0) << "\nFinal result:\n";
    for (Node const& i: nodes) {
        dbgs(0) << i.bb->getName() << ":\n";
        i.state.printOutgoing(*i.bb, dbgs(0));
    }
}

template <typename AbstractState>
void executeFixpointAlgorithmWidening(llvm::Module& M) {
    constexpr int widen_after = 2;
    bool narrowing_enabled = false;

    struct Node {
        int id;
        int change_count = 0; // How often has node changed during iterations
        llvm::BasicBlock* bb;
        AbstractState state;
        bool should_widen = false; // Whether the node is qualifies for widening/narrowing etc.
        bool update_scheduled = false; // Whether the node is already in the worklist

        // If this is set, the algorithm will add the initial values from the parameters of the
        // function to the incoming values, which is the correct thing to do for initial basic
        // blocks.
        llvm::Function* func_entry = nullptr;
    };

    std::vector<Node> nodes;
    std::unordered_map<llvm::BasicBlock*, int> nodeIdMap; // Maps basic blocks to the ids of their corresponding nodes
    std::vector<int> worklist; // Contains the ids of nodes that need to be processed
    std::vector<llvm::BasicBlock*> basicBlocksInLoops; // Contains BasicBlocks that are part of loops

    // TODO: Check what this does for release clang, probably write out a warning
    dbgs(1) << "Initialising fixpoint algorithm, collecting basic blocks\n";

    // Push dummy element indicating the end of the fixpoint
    worklist.push_back(-1);

    for (llvm::Function& f: M.functions()) {
        // Check for external (i.e. declared but not defined) functions
        if (f.empty()) {
            dbgs(1) << "  Function " << f.getName() << " is external, skipping...";
            continue;
        }


        // Get the DominatorTree of the current function
        llvm::DominatorTree dominatorTree = llvm::DominatorTree();
        dominatorTree.recalculate(f);

        // Use the LoopInfoBase to gather information about loops in the function
        llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop> loopInfoBase;
        loopInfoBase.releaseMemory();
        loopInfoBase.analyze(dominatorTree);

        // Iterate over loops to find BasicBlocks that are part of them
        llvm::SmallVector<llvm::Loop*, 4> loops;
        loops = loopInfoBase.getLoopsInPreorder();

        // Save basic blocks for later
        for (size_t i = 0; i < loops.size(); ++i) {
            basicBlocksInLoops.push_back(loops[i]->getHeader());
        }

        // Register basic blocks
        for (llvm::BasicBlock& bb: f) {
            dbgs(1) << "  Found basic block " << bb.getName() << '\n';

            Node node;
            node.id = nodes.size(); // Assign new id
            node.bb = &bb;
            // node.state is default initialised (to bottom)

            if (std::find(basicBlocksInLoops.begin(), basicBlocksInLoops.end(), &bb) != basicBlocksInLoops.end()){
                dbgs(3) << "  Found basic block in Loop" << bb.getName() << '\n';
                node.should_widen = true;
            }

            nodeIdMap[node.bb] = node.id;
            nodes.push_back(node);
        }

        // Push the initial block into the worklist
        int entry_id = nodeIdMap.at(&f.getEntryBlock());
        worklist.push_back(entry_id);
        nodes[entry_id].update_scheduled = true;
        nodes[entry_id].func_entry = &f;
    }

    dbgs(1) << "\nWorklist initialised with " << worklist.size() << (worklist.size() != 1 ? " entries" : " entry")
            << ". Starting fixpoint iteration...\n";

    for (int iter = 0; !worklist.empty(); ++iter) {
        // if stop element is reached switch to narrowing
        if (worklist.back() == -1) {
            worklist.pop_back();
            narrowing_enabled = true;
            dbgs(1) << "\nStarting narrowing iteration at iteration: " << iter << "\n";

            for (Node const& i: nodes) {
                if (i.should_widen) {
                    worklist.push_back(i.id);
                }
            }

            continue;
        }

        Node& node = nodes[worklist.back()];
        worklist.pop_back();
        node.update_scheduled = false;

        dbgs(1) << "\nIteration " << iter << ", considering basic block " << node.bb->getName() << '\n';

        AbstractState state_new; // Set to bottom

        if (node.func_entry) {
            dbgs(1) << "  Merging function parameters, is entry block\n";

            AbstractState state_entry {*node.func_entry};
            state_new.merge(state_entry);
        }

        dbgs(1) << "  Merge of " << llvm::pred_size(node.bb)
                << (llvm::pred_size(node.bb) != 1 ? " predecessors.\n" : " predecessor.\n");

        // Collect the predecessors
        std::vector<AbstractState> predecessors;
        for (llvm::BasicBlock* bb: llvm::predecessors(node.bb)) {
            dbgs(3) << "    Merging basic block " << bb->getName() << '\n';

            AbstractState state_branched {nodes[nodeIdMap[bb]].state};
            state_branched.branch(*bb, *node.bb);
            state_new.merge(state_branched);
            predecessors.push_back(state_branched);
        }

        dbgs(2) << "  Relevant incoming state is:\n"; state_new.printIncoming(*node.bb, dbgs(2), 4);

        // Apply the basic block
        dbgs(3) << "  Applying basic block\n";
        state_new.apply(*node.bb, predecessors);

        // Merge the state back into the node
        dbgs(3) << "  Merging with stored state\n";

        //TODO: introduce widening/narrowing/warrowing
        bool changed;
        int op = 0;

        if (node.should_widen and node.change_count > widen_after and not narrowing_enabled) {
            op = 2;
        } else if (node.should_widen and node.change_count > widen_after and narrowing_enabled) {
            op = 3;
        } else op = 1;

        // calls widening/narrowing as needed
        changed = node.state.merge(state_new, op);

        dbgs(2) << "  Outgoing state is:\n"; state_new.printOutgoing(*node.bb, dbgs(2), 2);

        // No changes, so no need to do anything else
        if (not changed) continue;
        else node.change_count++;

        dbgs(2) << "  State changed, notifying " << llvm::succ_size(node.bb)
                << (llvm::succ_size(node.bb) != 1 ? " successors\n" : " successor\n");

        // Something changed and we will need to update the successors
        for (llvm::BasicBlock* succ_bb: llvm::successors(node.bb)) {
            Node& succ = nodes[nodeIdMap[succ_bb]];
            if (not succ.update_scheduled) {
                worklist.push_back(succ.id);
                succ.update_scheduled = true;

                dbgs(3) << "    Adding " << succ_bb->getName() << " to worklist\n";
            }
        }
    }

    // Output the final result
    dbgs(0) << "\nFinal result:\n";
    for (Node const& i: nodes) {
        dbgs(0) << i.bb->getName() << ":\n";
        i.state.printOutgoing(*i.bb, dbgs(0));
    }
}


bool AbstractInterpretationPass::runOnModule(llvm::Module& M) {
    using AbstractState = AbstractStateValueSet<SimpleInterval>;

    // use either the standard fixpoint algorithm or the version with widening
    executeFixpointAlgorithmWidening<AbstractState>(M);

    // We never change anything
    return false;
}

} /* end of namespace pcpo */

