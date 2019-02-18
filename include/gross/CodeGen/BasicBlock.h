#ifndef GROSS_CODEGEN_BASICBLOCK_H
#define GROSS_CODEGEN_BASICBLOCK_H
#include "gross/Graph/Node.h"
#include "gross/Support/iterator_range.h"
#include <list>
#include <utility>

namespace gross {
class BasicBlock {
  std::list<Node*> NodeSequence;

  std::vector<BasicBlock*> Predecessors;
  std::vector<BasicBlock*> Successors;

public:
  BasicBlock() = default;

  void AddNode(Node* N) { NodeSequence.push_back(N); }

  // iterators
  using pred_iterator = typename decltype(Predecessors)::iterator;
  using succ_iterator = typename decltype(Successors)::iterator;
  pred_iterator pred_begin() { return Predecessors.begin(); }
  pred_iterator pred_end() { return Predecessors.end(); }
  llvm::iterator_range<pred_iterator> preds() {
    return llvm::make_range(pred_begin(), pred_end());
  }
  size_t pred_size() const { return Predecessors.size(); }
  succ_iterator succ_begin() { return Successors.begin(); }
  succ_iterator succ_end() { return Successors.end(); }
  llvm::iterator_range<succ_iterator> succs() {
    return llvm::make_range(succ_begin(), succ_end());
  }
  size_t succ_size() const { return Successors.size(); }

  void AddPredBlock(BasicBlock* BB) { Predecessors.push_back(BB); }
  void AddSuccBlock(BasicBlock* BB) { Successors.push_back(BB); }

  // build a std::pair<BasicBlock*,BasicBlock*> that represents
  // { source BB, dest BB }
  struct EdgeBuilder;
};

struct BasicBlock::EdgeBuilder {
  using value_type = std::pair<BasicBlock*,BasicBlock*>;
  BasicBlock* FromBB;

  EdgeBuilder() : FromBB(nullptr) {}
  explicit EdgeBuilder(BasicBlock* From) : FromBB(From) {}

  value_type operator()(BasicBlock* To) const {
    return std::make_pair(FromBB, To);
  }
};
} // end namespace gross
#endif
