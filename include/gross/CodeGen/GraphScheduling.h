#ifndef GROSS_CODEGEN_GRAPHSCHEDULING_H
#define GROSS_CODEGEN_GRAPHSCHEDULING_H
#include "boost/iterator/iterator_facade.hpp"
#include "gross/CodeGen/Function.h"
#include "gross/CodeGen/BasicBlock.h"
#include "gross/Graph/Graph.h"
#include "gross/Support/iterator_range.h"
#include "gross/Support/Graph.h"
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>

namespace gross {
class GraphSchedule {
  Graph& G;

  // owner of basic blocks
  std::list<std::unique_ptr<BasicBlock>> Blocks;
  std::vector<BasicBlock*> RPOBlocks;
  std::unordered_map<Node*, BasicBlock*> Node2Block;

  struct RPOVisitor;

public:
  GraphSchedule(Graph& graph) : G(graph) {}

  using block_iterator = typename decltype(Blocks)::iterator;
  using const_block_iterator = typename decltype(Blocks)::const_iterator;
  block_iterator block_begin() { return Blocks.begin(); }
  const_block_iterator block_cbegin() const { return Blocks.cbegin(); }
  block_iterator block_end() { return Blocks.end(); }
  const_block_iterator block_cend() const { return Blocks.cend(); }
  llvm::iterator_range<block_iterator> blocks() {
    return llvm::make_range(block_begin(), block_end());
  }
  size_t block_size() const { return Blocks.size(); }

  void SortRPO();
  using rpo_iterator = typename decltype(RPOBlocks)::iterator;
  rpo_iterator rpo_begin() { return RPOBlocks.begin(); }
  rpo_iterator rpo_end() { return RPOBlocks.end(); }
  llvm::iterator_range<rpo_iterator> rpo_blocks() {
    return llvm::make_range(rpo_begin(), rpo_end());
  }

  // { FromBB, ToBB }
  using edge_type = std::pair<BasicBlock*, BasicBlock*>;
  class edge_iterator;
  edge_iterator edge_begin();
  edge_iterator edge_end();
  llvm::iterator_range<edge_iterator> edges();
  size_t edge_size();
  size_t edge_size() const {
    return const_cast<GraphSchedule*>(this)->edge_size();
  }

  BasicBlock* NewBasicBlock();

  void AddNode(BasicBlock* BB, Node* N) {
    BB->AddNode(N);
    Node2Block[N] = BB;
  }

  BasicBlock* MapBlock(Node* N) {
    if(!Node2Block.count(N)) return nullptr;
    return Node2Block.at(N);
  }

  std::ostream& printBlock(std::ostream&, BasicBlock* BB);
};

class GraphSchedule::edge_iterator
  : public boost::iterator_facade<
      GraphSchedule::edge_iterator,
      typename GraphSchedule::edge_type, // Value type
      boost::forward_traversal_tag,      // Traversal tag
      typename GraphSchedule::edge_type  // Reference type
      > {
  friend class boost::iterator_core_access;
  using block_iterator = typename GraphSchedule::block_iterator;
  block_iterator BlockIt, BlockEnd;
  typename BasicBlock::succ_iterator SuccIt, SuccEnd;

  void nextValidPos() {
    while(SuccIt == SuccEnd &&
          BlockIt != BlockEnd) {
      // step to next block
      ++BlockIt;
      auto SuccRange = BlockIt->get()->succs();
      SuccIt = SuccRange.begin();
      SuccEnd = SuccRange.end();
    }
  }

  void increment() {
    ++SuccIt;
    nextValidPos();
  }

  typename GraphSchedule::edge_type
  dereference() const {
    return std::make_pair(BlockIt->get(), *SuccIt);
  }

  bool equal(const edge_iterator& Other) const {
    return BlockIt == Other.BlockIt &&
           SuccIt == Other.SuccIt;
  }

public:
  edge_iterator() = default;
  edge_iterator(block_iterator Start, block_iterator End)
    : BlockIt(Start), BlockEnd(End),
      SuccIt(), SuccEnd() {
    nextValidPos();
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
