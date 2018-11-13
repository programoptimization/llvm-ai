#ifndef WORKLIST
#define WORKLIST

#include "../interprocedural/CallHierarchy.h"
#include "llvm/IR/BasicBlock.h"
#include <memory>
#include <queue>
#include <unordered_set>
#include <utility>

using namespace llvm;

namespace pcpo {
class WorkList {
public:
  struct Item {
    CallHierarchy hierarchy;
    BasicBlock *block;
  };

  void push(Item item);

  Item const& peek();

  Item pop();

  bool empty();

private:
  std::queue<std::unique_ptr<Item>> worklist;

  struct ItemHasher {
    std::size_t operator()(Item const *item) const {
      // TODO Implement the hash for this
      return 0;
    }
  };
  struct ItemComparator {
    bool operator()(Item const *left, Item const *right) const {
      //
      return (left->hierarchy == right->hierarchy) &&
             (left->block == right->block);
    }
  };

  std::unordered_set<Item *, ItemHasher, ItemComparator> inWorklist;
};
} // namespace pcpo

#endif
