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

    // Applies the changes needed to reflect executing the instructions in the basic block. Before
    // this operation is called, the state is the one upon entering bb, afterwards it should be (an
    // upper bound of) the state leaving the basic block.
    void apply(llvm::BasicBlock& bb) {};

    // After this operation the state should contain an upper bound of both its previous value and
    // other. In other words, this computes (an upper bound of) the union of *this and other in
    // place. (I would have called it union, if that were possible.) Returns whether the state
    // changed as a result.
    bool merge(AbstractStateDummy const& other) { return false; };

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
    };

    std::vector<Node> nodes;
    std::unordered_map<llvm::BasicBlock*, int> nodeIdMap; // Maps basic blocks to the ids of their corresponding nodes
    std::vector<int> worklist; // Contains the ids of nodes that need to be processed

    // TODO: Check what this does for release clang, probably write out a warning
    dbgs(1) << "Initialising fixpoint algorithm, collecting basic blocks\n";
    
    for (llvm::Function& f: M.functions()) {
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
    }
    
    dbgs(1) << "\nWorklist initialised with " << worklist.size() << (worklist.size() != 1 ? " entries" : " entry")
            << ". Starting fixpoint iteration...\n\n";

    for (int iter = 0; !worklist.empty(); ++iter) {
        Node& node = nodes[worklist.back()];
        worklist.pop_back();
        node.update_scheduled = false;

        dbgs(1) << "Iteration " << iter << ", considering basic block " << node.bb->getName() << '\n';

        AbstractState state_new; // Set to bottom
        
        dbgs(1) << "  Merge of " << llvm::pred_size(node.bb)
                << (llvm::pred_size(node.bb) != 1 ? " predecessors.\n" : " predecessor.\n");

        // Collect the predecessors
        for (llvm::BasicBlock* bb: llvm::predecessors(node.bb)) {
            dbgs(3) << "    Merging basic block " << bb->getName() << '\n';
            
            AbstractState state_branched {nodes[nodeIdMap[bb]].state};
            state_branched.branch(*bb, *node.bb);
            state_new.merge(state_branched);
        }

        dbgs(2) << "  Incoming state is:\n"; state_new.printIncoming(*node.bb, dbgs(2), 4);
        
        // Apply the basic block
        dbgs(3) << "  Applying basic block\n";
        state_new.apply(*node.bb);

        // Merge the state back into the node
        dbgs(3) << "  Merging with stored state\n";
        bool changed = node.state.merge(state_new);

        dbgs(2) << "  Outgoing state is:\n"; state_new.printOutgoing(*node.bb, dbgs(2), 2); dbgs(2) << '\n';

        // No changes, so no need to do anything else
        if (not changed) continue;
        
        // Something changed and we will need to update the successors
        for (llvm::BasicBlock* succ_bb: llvm::successors(node.bb)) {
            Node& succ = nodes[nodeIdMap[succ_bb]];
            if (not succ.update_scheduled) {
                worklist.push_back(succ.id);
                succ.update_scheduled = true;
            }
        }
    }

    // Output the final result
    for (Node const& i: nodes) {
        i.state.printOutgoing(*i.bb, dbgs(0));
    }
}


bool AbstractInterpretationPass::runOnModule(llvm::Module& M) {
    using AbstractState = AbstractStateValueSet<SimpleInterval>;
    
    executeFixpointAlgorithm<AbstractState>(M);
    
    // We never change anything
    return false;
}

} /* end of namespace pcpo */
