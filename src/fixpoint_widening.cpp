
#include <unordered_map>
#include <vector>

#include "llvm/IR/CFG.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"

#include "global.h"
#include "value_set.h"
#include "simple_interval.h"

namespace pcpo {

// Run the fixpoint algorithm using widening and narrowing. Note that a lot of code in here is
// duplicated from executeFixpointAlgorithm. If you just want to understand the basic fixpoint
// iteration, you should take a look at that instead.
//  The interface for AbstractState is the same as for the simple fixpoint (documented in
// AbstractStateDummy), except that is needs to support the merge operations WIDEN and NARROW, as
// you can probably guess.
//  Tip: Look at a diff of fixpoint.cpp and fixpoint_widening.cpp with a visual diff tool (I
// recommend Meld.)
template <typename AbstractState>
void executeFixpointAlgorithmWidening(llvm::Module& M) {
    constexpr int iterations_max = 1000;
    constexpr int widen_after = 2; // Number of iteration after which we switch to widening.

    // A node in the control flow graph, i.e. a basic block. Here, we need a bit of additional data
    // per node to execute the fixpoint algorithm.
    struct Node {
        int id;
        llvm::BasicBlock* bb;
        AbstractState state;
        bool update_scheduled = false; // Whether the node is already in the worklist

        // If this is set, the algorithm will add the initial values from the parameters of the
        // function to the incoming values, which is the correct thing to do for initial basic
        // blocks.
        llvm::Function* func_entry = nullptr;

        bool should_widen = false; // Whether we want to widen at this node
        int change_count = 0; // How often has node changed during iterations
    };

    std::vector<Node> nodes;
    std::unordered_map<llvm::BasicBlock*, int> nodeIdMap; // Maps basic blocks to the ids of their corresponding nodes
    std::vector<int> worklist; // Contains the ids of nodes that need to be processed
    bool phase_narrowing = false; // If this is set, we are in the narrowing phase of the fixpoint algorithm

    dbgs(1) << "Initialising fixpoint algorithm, collecting basic blocks\n";

    // Push dummy element indicating the end of the widening phase of the fixpoint algorithm. As the
    // worklist is processed in a LIFO order, this will be the last element coming out, indicating
    // that the worklist is empty. Once that happens, we have obtained a valid solution (using
    // widening) and can start to apply narrowing.
    worklist.push_back(-1);

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
        
        // Gather information about loops in the function. (We only want to widen a single node for
        // each loop, as that is enough to guarantee fast termination.)
        llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop> loopInfoBase;
        loopInfoBase.analyze(llvm::DominatorTree {f});
        for (llvm::Loop* loop: loopInfoBase) {
            // We want to widen only the conditions of the loops
            nodes[nodeIdMap.at(loop->getHeader())].should_widen = true;
            dbgs(1) << "  Enabling widening for basic block " << loop->getHeader()->getName() << '\n';
        }

        // Push the initial block into the worklist
        int entry_id = nodeIdMap.at(&f.getEntryBlock());
        worklist.push_back(entry_id);
        nodes[entry_id].update_scheduled = true;
        nodes[entry_id].func_entry = &f;
    }

    dbgs(1) << "\nWorklist initialised with " << worklist.size() << (worklist.size() != 1 ? " entries" : " entry")
            << ". Starting fixpoint iteration...\n";

    for (int iter = 0; !worklist.empty() and iter < iterations_max; ++iter) {
        
        // Check whether we have reached the end of the widening phase
        if (worklist.back() == -1) {
            worklist.pop_back();
            phase_narrowing = true;
            dbgs(1) << "\nStarting narrowing in iteration " << iter << "\n";

            // We need to consider all nodes once more.
            for (Node const& i: nodes) {
                worklist.push_back(i.id);
            }

            --iter;
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
            state_new.merge(Merge_op::UPPER_BOUND, state_entry);
        }

        dbgs(1) << "  Merge of " << llvm::pred_size(node.bb)
                << (llvm::pred_size(node.bb) != 1 ? " predecessors.\n" : " predecessor.\n");

        // Collect the predecessors
        std::vector<AbstractState> predecessors;
        for (llvm::BasicBlock* bb: llvm::predecessors(node.bb)) {
            dbgs(3) << "    Merging basic block " << bb->getName() << '\n';

            AbstractState state_branched {nodes[nodeIdMap[bb]].state};
            state_branched.branch(*bb, *node.bb);
            state_new.merge(Merge_op::UPPER_BOUND, state_branched);
            predecessors.push_back(state_branched);
        }

        dbgs(2) << "  Relevant incoming state\n"; state_new.printIncoming(*node.bb, dbgs(2), 4);

        // Apply the basic block
        dbgs(3) << "  Applying basic block\n";
        state_new.apply(*node.bb, predecessors);

        // Merge the state back into the node
        dbgs(3) << "  Merging with stored state\n";
        
        // We need to figure out what operation to apply.
        Merge_op::Type op;
        if (not phase_narrowing) {
            if (node.should_widen and node.change_count >= widen_after) {
                op = Merge_op::WIDEN;
            } else {
                op = Merge_op::UPPER_BOUND;
            }
        } else {
            op = Merge_op::NARROW;
        }

        // Now do the actual operation
        bool changed = node.state.merge(op, state_new);

        dbgs(2) << "  Outgoing state\n"; state_new.printOutgoing(*node.bb, dbgs(2), 4);

        // No changes, so no need to do anything else
        if (not changed) continue;

        ++node.change_count;
        
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

    if (!worklist.empty()) {
        dbgs(0) << "Iteration terminated due to exceeding loop count.\n";
    }
    
    // Output the final result
    dbgs(0) << "\nFinal result:\n";
    for (Node const& i: nodes) {
        dbgs(0) << i.bb->getName() << ":\n";
        i.state.printOutgoing(*i.bb, dbgs(0), 2);
    }
}

} /* end of namespace pcpo */
