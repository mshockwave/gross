#ifndef GROSS_CODEGEN_GRAPHSCHEDULING_H
#define GROSS_CODEGEN_GRAPHSCHEDULING_H
#include "boost/iterator/iterator_facade.hpp"
#include "boost/iterator/transform_iterator.hpp"
#include "gross/CodeGen/Function.h"
#include "gross/CodeGen/BasicBlock.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeMarker.h"
#include "gross/Support/iterator_range.h"
#include "gross/Support/STLExtras.h"
#include "gross/Support/Graph.h"
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>

namespace gross {
class GraphSchedule {
  Graph& G;
  SubGraph SG;

  // owner of basic blocks
  std::list<std::unique_ptr<BasicBlock>> Blocks;
  std::vector<BasicBlock*> RPOBlocks;
  std::unordered_map<Node*, BasicBlock*> Node2Block;

  struct RPOVisitor;

  class DominatorNode {
    BasicBlock* ThisBlock;
    std::vector<BasicBlock*> Dominants;
    BasicBlock* Dominator;

  public:
    explicit DominatorNode(BasicBlock* ThisBB)
      : ThisBlock(ThisBB), Dominator(nullptr) {}

    BasicBlock* getBlock() const { return ThisBlock; }

    void setDominator(BasicBlock* BB) { Dominator = BB; }
    BasicBlock* getDominator() { return Dominator; }

    void AddDomChild(BasicBlock* DN) { Dominants.push_back(DN); }
    bool RemoveDomChild(BasicBlock* DN) {
      auto It
        = gross::find_if(const_doms(),
                         [=](const BasicBlock* BB) -> bool {
                           return const_cast<BasicBlock*>(BB) == DN;
                         });
      if(It == dom_cend()) return false;
      Dominants.erase(It);
      return true;
    }

    using dom_iterator = typename decltype(Dominants)::iterator;
    using const_dom_iterator = typename decltype(Dominants)::const_iterator;
    dom_iterator dom_begin() { return Dominants.begin(); }
    const_dom_iterator dom_cbegin() { return Dominants.cbegin(); }
    dom_iterator dom_end() { return Dominants.end(); }
    const_dom_iterator dom_cend() { return Dominants.cend(); }
    llvm::iterator_range<dom_iterator> doms() {
      return llvm::make_range(dom_begin(), dom_end());
    }
    llvm::iterator_range<const_dom_iterator> const_doms() {
      return llvm::make_range(dom_cbegin(), dom_cend());
    }
    size_t dom_size() const { return Dominants.size(); }

    struct subdom_iterator;
  };
  using DomNodesTy
    = std::unordered_map<BasicBlock*, std::unique_ptr<DominatorNode>>;
  DomNodesTy DomNodes;

  DominatorNode::subdom_iterator subdom_begin(BasicBlock* BB);

  void ChangeDominator(BasicBlock* BB, BasicBlock* NewDomBB);

  class LoopTreeNode {
    BasicBlock* Header;
    BasicBlock* ParentLoop;
    std::vector<BasicBlock*> ChildLoops;

  public:
    explicit LoopTreeNode(BasicBlock* header)
      : Header(header),
        ParentLoop(nullptr) {}

    BasicBlock* getHeader() const { return Header; }

    BasicBlock* getParent() const { return ParentLoop; }
    void setParent(BasicBlock* ParentHeader) {
      ParentLoop = ParentHeader;
    }

    void AddChildLoop(BasicBlock* ChildHeader) {
      ChildLoops.push_back(ChildHeader);
    }
    void RemoveChildLoop(BasicBlock* ChildHeader) {
      std::remove_if(child_loop_begin(), child_loop_end(),
                     [=](BasicBlock* BB)->bool {
                       return ChildHeader == BB;
                     });
    }

    using loop_iterator = typename decltype(ChildLoops)::iterator;
    loop_iterator child_loop_begin() { return ChildLoops.begin(); }
    loop_iterator child_loop_end() { return ChildLoops.end(); }
    llvm::iterator_range<loop_iterator> child_loops() {
      return llvm::make_range(child_loop_begin(), child_loop_end());
    }
    size_t child_loop_size() const { return ChildLoops.size(); }
  };
  using LoopTreeTy
    = std::unordered_map<BasicBlock*, std::unique_ptr<LoopTreeNode>>;
  LoopTreeTy LoopTree;

  LoopTreeNode* GetOrCreateLoopNode(BasicBlock* Header) {
    if(!LoopTree.count(Header)) {
      LoopTree.insert({
        Header,
        gross::make_unique<LoopTreeNode>(Header)
      });
    }
    return LoopTree[Header].get();
  }

  enum ScheduleState : uint8_t {
    NotScheduled = 0,
    Fixed,
    Scheduled
  };
  NodeMarker<ScheduleState> StateMarker;

  // for Graphviz printing
  struct block_prop_writer;

public:
  GraphSchedule(Graph& graph, const SubGraph& subgraph)
    : G(graph), SG(subgraph),
      StateMarker(G, 3) {}

  const SubGraph& getSubGraph() const { return SG; }
  SubGraph& getSubGraph() { return SG; }

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
  using rpo_reverse_iterator
    = typename decltype(RPOBlocks)::reverse_iterator;
  rpo_reverse_iterator rpo_rbegin() { return RPOBlocks.rbegin(); }
  rpo_reverse_iterator rpo_rend() { return RPOBlocks.rend(); }
  llvm::iterator_range<rpo_reverse_iterator> po_blocks() {
    return llvm::make_range(rpo_rbegin(), rpo_rend());
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
  void AddNodeBefore(BasicBlock* BB, Node* Pos, Node* N) {
    BB->AddNodeBefore(Pos, N);
    Node2Block[N] = BB;
  }
  void AddNode(BasicBlock* BB, typename BasicBlock::const_node_iterator Pos,
               Node* N) {
    BB->AddNode(Pos, N);
    Node2Block[N] = BB;
  }

  BasicBlock* MapBlock(Node* N) {
    if(!Node2Block.count(N)) return nullptr;
    return Node2Block.at(N);
  }

  // just a wrapper for printing DomTree
  class DomTreeHandle;
  bool Dominate(BasicBlock* FromBB, BasicBlock* ToBB);
  BasicBlock* getDominator(BasicBlock* BB) {
    if(DomNodes.count(BB))
      return DomNodes[BB]->getDominator();
    else
      return nullptr;
  }
  // notify the DomTree to update
  void OnConnectBlock(BasicBlock* Pred, BasicBlock* Succ);

  bool IsLoopHeader(BasicBlock* BB) { return LoopTree.count(BB); }
  BasicBlock* getParentLoop(BasicBlock* HeaderBB) {
    if(IsLoopHeader(HeaderBB))
      return LoopTree[HeaderBB]->getParent();
    else
      return nullptr;
  }

  void SetFixed(Node* N) { StateMarker.Set(N, Fixed); }
  void SetScheduled(Node* N) { StateMarker.Set(N, Scheduled); }

  bool IsNodeFixed(Node* N) { return StateMarker.Get(N) == Fixed; }
  bool IsNodeScheduled(Node* N) {
    return StateMarker.Get(N) == Scheduled ||
           IsNodeFixed(N);
  }

  std::ostream& printBlock(std::ostream&, BasicBlock* BB);

  void dumpGraphviz(std::ostream&);

  void dumpDomTreeGraphviz(std::ostream&);
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
  size_t SuccIt;

  void nextValidPos() {
    while(BlockIt != BlockEnd &&
          SuccIt >= BlockIt->get()->succ_size()) {
      // step to next block
      ++BlockIt;
      SuccIt = 0;
    }
  }

  void increment() {
    ++SuccIt;
    nextValidPos();
  }

  typename GraphSchedule::edge_type
  dereference() const {
    auto* BB = BlockIt->get();
    return std::make_pair(BB,
                          *std::next(BB->succ_begin(), SuccIt));
  }

  bool equal(const edge_iterator& Other) const {
    return BlockIt == Other.BlockIt &&
           (SuccIt == Other.SuccIt ||
            BlockIt == BlockEnd);
  }

public:
  edge_iterator() = default;
  edge_iterator(block_iterator Start, block_iterator End)
    : BlockIt(Start), BlockEnd(End),
      SuccIt(0U) {
    nextValidPos();
  }
};

/// Linearlize the Nodes into instruction sequence
class GraphScheduler {
  Graph& G;
  // owner of all the schedules
  std::vector<std::unique_ptr<GraphSchedule>> Schedules;

public:
  GraphScheduler(Graph& graph);

  using schedule_iterator
    = boost::transform_iterator<gross::unique_ptr_unwrapper<GraphSchedule>,
                                typename decltype(Schedules)::iterator,
                                GraphSchedule*, // Reference type
                                GraphSchedule*  // Value type
                                >;
  schedule_iterator schedule_begin();
  schedule_iterator schedule_end();
  llvm::iterator_range<schedule_iterator> schedules() {
    return llvm::make_range(schedule_begin(), schedule_end());
  }
  size_t schedule_size() const { return Schedules.size(); }

  void ComputeScheduledGraph();
};
} // end namespace gross
#endif
