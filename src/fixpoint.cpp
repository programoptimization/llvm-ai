#include "fixpoint.h"

#include <unordered_map>
#include <vector>

#include "llvm/IR/CFG.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "value_set.h"
#include "simple_interval.h"

#define DEBUG_LEVEL 4

namespace pcpo {

// @Cleanup: Prune headers
// @Cleanup: getAnalysisUsage ??

static llvm::RegisterPass<AbstractInterpretationPass> Y("painpass", "AbstractInterpretation Pass");

char AbstractInterpretationPass::ID;

llvm::raw_ostream& dbg(int level) {
    if (level <= DEBUG_LEVEL) {
        return errs();
    } else {
        return nulls();
    }
}

class AbstractStateDummy /*AbstractState*/ {
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
    void print(llvm::BasicBlock& bb, llvm::raw_ostream& out) const {};
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
    
    for (llvm::Function& f: M.functions()) {
        // Register basic blocks
        for (llvm::BasicBlock& bb: f) {
            Node node;
            node.id = nodes.size(); // Assign new id
            node.bb = &bb;
            // node.state is default initialised (to bottom)
        
            nodes.push_back(node);
            nodeIdMap[node.bb] = node.id;
        }

        // Push the initial block into the worklist
        int entry_id = nodeIdMap.at(&f.getEntryBlock());
        worklist.push_back(entry_id);
        nodes[entry_id].update_scheduled = true;
    }

    while (!worklist.empty()) {
        Node& node = nodes[worklist.back()];
        worklist.pop_back();
        node.update_scheduled = false;

        AbstractState state_new; // Set to bottom
        
        // Collect the predecessors
        for (llvm::BasicBlock* bb: llvm::predecessors(node.bb)) {
            AbstractState state_branched {nodes[nodeIdMap[bb]].state};
            state_branched.branch(*bb, *node.bb);
            state_new.merge(state_branched);
        }

        // Apply the basic block
        state_new.apply(*node.bb);

        // Merge the state back into the node
        bool changed = node.state.merge(state_new);

        if (not changed) continue;
        
        // If something changed we will need to update the successors
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
        i.state.print(*i.bb, dbg(0));
    }
}


bool AbstractInterpretationPass::runOnModule(llvm::Module& M) {
    using AbstractState = AbstractStateValueSet<SimpleInterval>;
    
    executeFixpointAlgorithm<AbstractState>(M);
    
    // We never change anything
    return false;
}

} /* end of namespace pcpo */
