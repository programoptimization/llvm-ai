#include "fixpoint.h"

#include <unordered_map>
#include <vector>

#include "llvm/IR/CFG.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"

#include "global.h"
#include "fixpoint_widening.cpp"
#include "value_set.h"
#include "simple_interval.h"

namespace pcpo {

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
    explicit AbstractStateDummy(llvm::Function const& f) {}

    // Applies the changes needed to reflect executing the instructions in the basic block. Before
    // this operation is called, the state is the one upon entering bb, afterwards it should be (an
    // upper bound of) the state leaving the basic block.
    //  predecessors contains the outgoing state for all the predecessors, in the same order as they
    // are listed in llvm::predecessors(bb).
    void apply(llvm::BasicBlock const& bb, std::vector<AbstractStateDummy> const& predecessors) {};

    // This 'merges' two states, which is the operation we do fixpoint iteration over. Currently,
    // there are three possibilities for op:
    //   1. UPPER_BOUND: This has to return some upper bound of itself and other, with more precise
    //      bounds being preferred.
    //   2. WIDEN: Same as UPPER_BOUND, but this operation should sacrifice precision to converge
    //      quickly. Returning T would be fine, though maybe not optimal. For example for intervals,
    //      an implementation could ensure to double the size of the interval.
    //   3. NARROW: Return a value between the intersection of the state and other, and the
    //      state. In pseudocode:
    //          intersect(state, other) <= narrow(state, other) <= state
    // For all of the above, this operation returns whether the state changed as a result.
    // IMPORTANT: The simple fixpoint algorithm only performs UPPER_BOUND, so you do not need to
    // implement the others if you just use that one. (The more advanced algorithm in
    // fixpoint_widening.cpp uses all three operations.)
    bool merge(Merge_op::Type op, AbstractStateDummy const& other) { return false; };

    // Restrict the set of values to the one that allows 'from' to branch towards
    // 'towards'. Starting with the state when exiting from, this should compute (an upper bound of)
    // the possible values that would reach the block towards. Doing nothing thus is a valid
    // implementation.
    void branch(llvm::BasicBlock const& from, llvm::BasicBlock const& towards) {};

    // Functions to generate the debug output. printIncoming should output the state as of entering
    // the basic block, printOutcoming the state when leaving it.
    void printIncoming(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation = 0) const {};
    void printOutgoing(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation = 0) const {};
};

// Run the simple fixpoint algorithm. AbstractState should implement the interface documented in
// AbstractStateDummy (no need to subclass or any of that, just implement the methods with the right
// signatures and take care to fulfil the contracts outlines above).
// Note that a lot of this code is duplicated in executeFixpointAlgorithmWidening in
// fixpoint_widening.cpp, so if you fix any bugs in here, they probably should be fixed there as
// well.
//  Tip: Look at a diff of fixpoint.cpp and fixpoint_widening.cpp with a visual diff tool (I
// recommend Meld.)
template <typename AbstractState>
void executeFixpointAlgorithm(llvm::Module const& M) {
    constexpr int iterations_max = 1000;

    // A node in the control flow graph, i.e. a basic block. Here, we need a bit of additional data
    // per node to execute the fixpoint algorithm.
    struct Node {
        int id;
        llvm::BasicBlock const* bb;
        AbstractState state;
        bool update_scheduled = false; // Whether the node is already in the worklist

        // If this is set, the algorithm will add the initial values from the parameters of the
        // function to the incoming values, which is the correct thing to do for initial basic
        // blocks.
        llvm::Function const* func_entry = nullptr;
    };

    std::vector<Node> nodes;
    std::unordered_map<llvm::BasicBlock const*, int> nodeIdMap; // Maps basic blocks to the ids of their corresponding nodes
    std::vector<int> worklist; // Contains the ids of nodes that need to be processed

    // TODO: Check what this does for release clang, probably write out a warning
    dbgs(1) << "Initialising fixpoint algorithm, collecting basic blocks\n";

    for (llvm::Function const& f: M.functions()) {
        // Check for external (i.e. declared but not defined) functions
        if (f.empty()) {
            dbgs(1) << "  Function " << f.getName() << " is external, skipping...";
            continue;
        }

        // Register basic blocks
        for (llvm::BasicBlock const& bb: f) {
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
            state_new.merge(Merge_op::UPPER_BOUND, state_entry);
        }

        dbgs(1) << "  Merge of " << llvm::pred_size(node.bb)
                << (llvm::pred_size(node.bb) != 1 ? " predecessors.\n" : " predecessor.\n");

        // Collect the predecessors
        std::vector<AbstractState> predecessors;
        for (llvm::BasicBlock const* bb: llvm::predecessors(node.bb)) {
            dbgs(3) << "    Merging basic block " << bb->getName() << '\n';

            AbstractState state_branched {nodes[nodeIdMap[bb]].state};
            state_branched.branch(*bb, *node.bb);
            state_new.merge(Merge_op::UPPER_BOUND, state_branched);
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
        for (llvm::BasicBlock const* succ_bb: llvm::successors(node.bb)) {
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

    // Use either the standard fixpoint algorithm or the version with widening
    //executeFixpointAlgorithm        <AbstractState>(M);
      executeFixpointAlgorithmWidening<AbstractState>(M);

    // We never change anything
    return false;
}


void AbstractInterpretationPass::getAnalysisUsage(llvm::AnalysisUsage& info) const {
    info.setPreservesAll();
}

} /* end of namespace pcpo */

