#ifndef GROSS_CODEGEN_GRAPHSCHEDULING_H
#define GROSS_CODEGEN_GRAPHSCHEDULING_H
#include "gross/CodeGen/Function.h"
#include "gross/CodeGen/BasicBlock.h"
#include "gross/Graph/Graph.h"
#include "gross/Support/iterator_range.h"
#include "gross/Support/Graph.h"
#include <list>
#include <memory>

namespace gross {
// owner of basic blocks
class GraphSchedule {
  std::list<std::unique_ptr<BasicBlock>> Blocks;

public:
  using block_iterator = typename decltype(Blocks)::iterator;
  block_iterator block_begin() { return Blocks.begin(); }
  block_iterator block_end() { return Blocks.end(); }
  llvm::iterator_range<block_iterator> blocks() {
    return llvm::make_range(block_begin(), block_end());
  }
  size_t block_size() const { return Blocks.size(); }

  BasicBlock* NewBasicBlock();

  void AddNode(BasicBlock* BB, Node* N) {
    BB->AddNode(N);
  }
};

/// Linearlize the Nodes into instruction sequence
class GraphScheduler {
  Graph& G;

public:
  GraphScheduler(Graph& graph);

  void ComputeScheduledGraph();
};
} // end namespace gross
#endif
