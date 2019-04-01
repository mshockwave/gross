#ifndef GROSS_CODEGEN_GRAPHSCHEDULING_H
#define GROSS_CODEGEN_GRAPHSCHEDULING_H
#include "boost/iterator/iterator_facade.hpp"
#include "boost/iterator/transform_iterator.hpp"
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
  // must be in RPO order
  std::vector<std::unique_ptr<BasicBlock>> Blocks;
  std::unordered_map<Node*, BasicBlock*> Node2Block;
  std::vector<Node*> RPONodes;

  struct RPONodesVisitor;
  void SortRPONodes();

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
        = gross::find_if(doms(),
                         [=](BasicBlock* BB) -> bool {
                           return BB == DN;
                         });
      if(It == dom_end()) return false;
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
      LoopTree.insert(std::make_pair(
        Header,
        gross::make_unique<LoopTreeNode>(Header)
      ));
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

  // DLXOffset node -> BasicBlock
  NodeBiMap<BasicBlock*> BlockOffsets;

public:
  GraphSchedule(Graph& graph, const SubGraph& subgraph)
    : G(graph), SG(subgraph),
      StateMarker(G, 3) {
    SortRPONodes();
  }

  const Graph& getGraph() const { return G; }
  Graph& getGraph() { return G; }

  const SubGraph& getSubGraph() const { return SG; }
  SubGraph& getSubGraph() { return SG; }

  Node* getStartNode() const { return RPONodes.front(); }
  Node* getEndNode() const { return RPONodes.back(); }

  size_t getWordAllocaSize();

  using rpo_node_iterator = typename decltype(RPONodes)::iterator;
  rpo_node_iterator rpo_node_begin() { return RPONodes.begin(); }
  rpo_node_iterator rpo_node_end() { return RPONodes.end(); }
  llvm::iterator_range<rpo_node_iterator> rpo_nodes() {
    return llvm::make_range(rpo_node_begin(), rpo_node_end());
  }
  using po_node_iterator
    = typename decltype(RPONodes)::reverse_iterator;
  po_node_iterator po_node_begin() { return RPONodes.rbegin(); }
  po_node_iterator po_node_end() { return RPONodes.rend(); }
  llvm::iterator_range<po_node_iterator> po_nodes() {
    return llvm::make_range(po_node_begin(), po_node_end());
  }

  void dumpRPONodes();

  using block_iterator = typename decltype(Blocks)::iterator;
  using const_block_iterator = typename decltype(Blocks)::const_iterator;
  using reverse_block_iterator
    = typename decltype(Blocks)::reverse_iterator;
  block_iterator block_begin() { return Blocks.begin(); }
  const_block_iterator block_cbegin() const { return Blocks.cbegin(); }
  reverse_block_iterator block_rbegin() { return Blocks.rbegin(); }
  block_iterator block_end() { return Blocks.end(); }
  const_block_iterator block_cend() const { return Blocks.cend(); }
  reverse_block_iterator block_rend() { return Blocks.rend(); }
  llvm::iterator_range<block_iterator> blocks() {
    return llvm::make_range(block_begin(), block_end());
  }
  size_t block_size() const { return Blocks.size(); }

  using rpo_iterator
    = boost::transform_iterator<gross::unique_ptr_unwrapper<BasicBlock>,
                                block_iterator,
                                BasicBlock*, // Refrence type
                                BasicBlock* // Value type
                                >;
  rpo_iterator rpo_begin() {
    gross::unique_ptr_unwrapper<BasicBlock> functor;
    return rpo_iterator(block_begin(), functor);
  }
  rpo_iterator rpo_end() {
    gross::unique_ptr_unwrapper<BasicBlock> functor;
    return rpo_iterator(block_end(), functor);
  }
  llvm::iterator_range<rpo_iterator> rpo_blocks() {
    return llvm::make_range(rpo_begin(), rpo_end());
  }
  using po_iterator
    = boost::transform_iterator<gross::unique_ptr_unwrapper<BasicBlock>,
                                reverse_block_iterator,
                                BasicBlock*, // Refrence type
                                BasicBlock* // Value type
                                >;
  po_iterator po_begin() {
    gross::unique_ptr_unwrapper<BasicBlock> functor;
    return po_iterator(block_rbegin(), functor);
  }
  po_iterator po_end() {
    gross::unique_ptr_unwrapper<BasicBlock> functor;
    return po_iterator(block_rend(), functor);
  }
  llvm::iterator_range<po_iterator> po_blocks() {
    return llvm::make_range(po_begin(), po_end());
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
    if(BB->AddNodeBefore(Pos, N))
      Node2Block[N] = BB;
  }
  void AddNodeAfter(BasicBlock* BB, Node* Pos, Node* N) {
    if(BB->AddNodeAfter(Pos, N))
      Node2Block[N] = BB;
  }
  void AddNode(BasicBlock* BB, typename BasicBlock::node_iterator Pos,
               Node* N) {
    BB->AddNode(Pos, N);
    Node2Block[N] = BB;
  }

  std::pair<bool, typename BasicBlock::node_iterator>
  RemoveNode(BasicBlock* BB, Node* N) {
    auto Ret = BB->RemoveNode(N);
    if(Ret.first) {
      Node2Block.erase(N);
    }
    return Ret;
  }

  void ReplaceNode(BasicBlock* BB, Node* Old, Node* New) {
    BB->ReplaceNode(Old, New);
    if(Node2Block.count(Old))
      Node2Block.erase(Old);
    Node2Block[New] = BB;
  }

  BasicBlock* MapBlock(Node* N) {
    if(!Node2Block.count(N)) return nullptr;
    return Node2Block.at(N);
  }

  Node* MapBlockOffset(BasicBlock* BB) {
    if(!BlockOffsets.find_node(BB)) {
      auto* N = new Node(IrOpcode::DLXOffset, {});
      G.InsertNode(N);
      BlockOffsets.insert({N, BB});
    }
    return BlockOffsets.find_node(BB);
  }

  BasicBlock* getEntryBlock() {
    return MapBlock(getStartNode());
  }

  // wrapper for printing DomTree and LoopTree
  template<class ProxyT>
  class TreeHandle;

  struct DomTreeProxy;
  bool Dominate(BasicBlock* FromBB, BasicBlock* ToBB);
  BasicBlock* getDominator(BasicBlock* BB) {
    if(DomNodes.count(BB))
      return DomNodes[BB]->getDominator();
    else
      return nullptr;
  }
  // notify the DomTree to update
  void OnConnectBlock(BasicBlock* Pred, BasicBlock* Succ);

  struct LoopTreeProxy;
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
  void dumpLoopTreeGraphviz(std::ostream&);
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
