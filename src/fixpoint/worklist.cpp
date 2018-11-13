#include "worklist.h"
#include "state.h"

using namespace llvm;
namespace pcpo {

void WorkList::push(Item item) {
  if (inWorklist.find(&item) == inWorklist.end()) {
    std::unique_ptr<Item> ptr(new Item(std::move(item)));

    inWorklist.insert(ptr.get());
    worklist.push(std::move(ptr));
  }
}

WorkList::Item WorkList::pop() {
  std::unique_ptr<Item> temp = std::move(worklist.front());
  inWorklist.erase(temp.get());
  worklist.pop();
  return std::move(*temp);
}

WorkList::Item const& WorkList::peek() {
  // 
  return *worklist.front();
}

bool WorkList::empty() { return worklist.empty(); }
} // namespace pcpo
