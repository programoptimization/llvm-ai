#include "fixpoint.h"

#include <unordered_map>
#include <vector>

#include "llvm/IR/CFG.h"
#include "llvm/IR/Module.h"

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
    bool merge(Merge_op::Type op, AbstractStateDummy const& other) { return false; };

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
    constexpr int iterations_max = 1000;
    
    struct Node {
        int id;
        llvm::BasicBlock* bb;
        AbstractState state;
        bool update_scheduled = false; // Whether the node is already in the worklist

        // If this is set, the algorithm will add the initial values from the parameters of the
        // function to the incoming values, which is the correct thing to do for inital basic
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

    for (int iter = 0; !worklist.empty() and iter < iterations_max; ++iter) {
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
        bool changed = node.state.merge(Merge_op::UPPER_BOUND, state_new);

        dbgs(2) << "  Outgoing state is:\n"; state_new.printOutgoing(*node.bb, dbgs(2), 4);

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


bool AbstractInterpretationPass::runOnModule(llvm::Module& M) {
    using AbstractState = AbstractStateValueSet<SimpleInterval>;
    
    executeFixpointAlgorithm<AbstractState>(M);
    
    // We never change anything
    return false;
}

} /* end of namespace pcpo */
