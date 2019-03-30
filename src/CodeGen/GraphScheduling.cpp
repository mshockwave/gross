#include "BGL.h"
#include "gross/Graph/BGL.h"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/graphviz.hpp"
#include "gross/CodeGen/GraphScheduling.h"
#include "gross/Graph/Node.h"
#include "gross/Graph/NodeUtils.h"
#include "DLXNodeUtils.h"
#include <unordered_set>
#include <set>
#include <functional>
#include <queue>
#include <vector>
#include <iostream>

using namespace gross;

struct GraphSchedule::RPONodesVisitor
  : public boost::default_dfs_visitor {
  struct PostEntity {
    Node *StartNode, *EndNode;
    std::unordered_set<Node*> BranchStarts;

    PostEntity() : StartNode(nullptr), EndNode(nullptr) {}
  };

  void finish_vertex(Node* N, const SubGraph& G) {
    switch(N->getOp()) {
    case IrOpcode::Start:
      PE.StartNode = N;
      return;
    case IrOpcode::End:
      PE.EndNode = N;
      return;
#if 0
    case IrOpcode::If:
    case IrOpcode::Loop:
      return;
    case IrOpcode::IfTrue:
    case IrOpcode::IfFalse: {
      NodeProperties<IrOpcode::VirtIfBranches> BNP(N);
      auto* Br = BNP.BranchPoint();
      if(!VisitedBranchPoints.count(Br)) {
        VisitedBranchPoints.insert(Br);
        PE.BranchStarts.insert(N);
      }
      break;
    }
#endif
    default: break;
    }
    Trace.push_back(N);
  }

  RPONodesVisitor(std::vector<Node*>& trace,
                  PostEntity& entity)
    : Trace(trace), PE(entity) {}

private:
  std::vector<Node*>& Trace;
  PostEntity& PE;
  std::set<Node*> VisitedBranchPoints;
};

void GraphSchedule::SortRPONodes() {
  auto BackEdgePatcher = [](const Use& OrigEdge) -> Use {
    auto* Source = OrigEdge.Source;
    NodeProperties<IrOpcode::Phi> PNP(Source);
    if(PNP && PNP.CtrlPivot()->getOp() == IrOpcode::Loop) {
      // check if it is backedge
      Node* Backedge = nullptr;
      if(Source->getNumValueInput() &&
         !Source->getNumEffectInput()) {
        // normal value phi
        assert(Source->getNumValueInput() >= 2);
        // backedge is always the second input
        Backedge = Source->getValueInput(1);
      } else {
        // effect phi
        assert(Source->getNumEffectInput() >= 2);
        Backedge = Source->getEffectInput(1);
      }
      if(Backedge == OrigEdge.Dest) {
        // reverse!
        Use Copy(OrigEdge);
        Copy.Source = OrigEdge.Dest;
        Copy.Dest = OrigEdge.Source;
        return std::move(Copy);
      }
    } else if(Source->getOp() == IrOpcode::Loop) {
      assert(Source->getNumControlInput() >= 2);
      auto* Backedge = Source->getControlInput(1);
      if(Backedge == OrigEdge.Dest) {
        // reverse!
        Use Copy(OrigEdge);
        Copy.Source = OrigEdge.Dest;
        Copy.Dest = OrigEdge.Source;
        return std::move(Copy);
      }
    }
    return OrigEdge;
  };
  getSubGraph().SetEdgePatcher(BackEdgePatcher);

  RPONodesVisitor::PostEntity PE;
  RPONodesVisitor Vis(RPONodes, PE);
  std::unordered_map<Node*, boost::default_color_type> ColorStorage;
  StubColorMap<decltype(ColorStorage), Node> ColorMap(ColorStorage);
  boost::depth_first_search(getSubGraph(), Vis, std::move(ColorMap));
  assert(PE.StartNode && PE.EndNode);

  getSubGraph().ClearEdgePatcher();

  RPONodes.insert(RPONodes.begin(), PE.StartNode);
  RPONodes.push_back(PE.EndNode);
}

void GraphSchedule::dumpRPONodes() {
  size_t Idx = 0;
  for(auto* N : RPONodes) {
    IrOpcode::Print(G, std::cout << Idx++ << ": ", N) << "\n";
  }
}

// return # of words in current local allocation
size_t GraphSchedule::getWordAllocaSize() {
  size_t TotalSize = 0U;
  for(auto* BB : rpo_blocks()) {
    for(auto* N : BB->nodes()) {
      NodeProperties<IrOpcode::Alloca> ANP(N);
      if(!ANP) continue;
      auto* Size = ANP.Size();
      assert(Size->getOp() == IrOpcode::ConstantInt &&
             "dynamic size allocation?");
      TotalSize +=
        NodeProperties<IrOpcode::ConstantInt>(Size).as<size_t>(G);
    }
  }
  // ceil(TotalSize / 4)
  if(TotalSize % 4)
    return (TotalSize >> 2) + 1;
  else
    return (TotalSize >> 2);
}

BasicBlock* GraphSchedule::NewBasicBlock() {
  BasicBlock* BB;
  if(Blocks.empty())
    BB = new BasicBlock();
  else {
    auto& PrevBB = *Blocks.back().get();
    BB = new BasicBlock(BasicBlock::Id::AdvanceFrom(PrevBB.getId()));
  }
  Blocks.emplace_back(BB);
  return Blocks.back().get();
}

typename GraphSchedule::edge_iterator
GraphSchedule::edge_begin() {
  return edge_iterator(Blocks.begin(), Blocks.end());
}
typename GraphSchedule::edge_iterator
GraphSchedule::edge_end() {
  return edge_iterator(Blocks.end(), Blocks.end());
}
llvm::iterator_range<typename GraphSchedule::edge_iterator>
GraphSchedule::edges() {
  return llvm::make_range(edge_begin(), edge_end());
}
size_t GraphSchedule::edge_size() {
  return std::distance(edge_begin(), edge_end());
}

// a simple BFS iterator
struct GraphSchedule::DominatorNode::subdom_iterator {
  using QueryCallbackTy
    = std::function<DominatorNode*(BasicBlock*)>;

  subdom_iterator(BasicBlock* Start, QueryCallbackTy CB)
    : Query(CB) {
    WorkQueue.push(Start);
  }

  BasicBlock* operator*() const {
    return WorkQueue.front();
  }

  subdom_iterator& operator++() {
    assert(!WorkQueue.empty());
    auto* BB = WorkQueue.front();
    WorkQueue.pop();
    auto* DomNode = Query(BB);
    if(DomNode) {
      for(auto* NewBB : DomNode->doms())
        WorkQueue.push(NewBB);
    }
    return *this;
  }
  subdom_iterator operator++(int) {
    subdom_iterator tmp(*this);
    operator++();
    return tmp;
  }

  bool isEnd() const {
    return WorkQueue.empty();
  }

private:
    QueryCallbackTy Query;
    std::queue<BasicBlock*> WorkQueue;
};

GraphSchedule::DominatorNode::subdom_iterator
GraphSchedule::subdom_begin(BasicBlock* RootBB) {
  using subdom_iterator = DominatorNode::subdom_iterator;
  return subdom_iterator(RootBB,
                         [this](BasicBlock* BB) -> DominatorNode* {
                           if(DomNodes.count(BB))
                             return DomNodes[BB].get();
                           else
                             return nullptr;
                         });
}

// basically a tree reachbility problem
bool GraphSchedule::Dominate(BasicBlock* FromBB, BasicBlock* ToBB) {
  if(!DomNodes.count(FromBB) ||
     !DomNodes.count(ToBB)) return false;
  if(FromBB == ToBB) return true;

  // BFS search
  std::queue<BasicBlock*> Queue;
  Queue.push(FromBB);
  while(!Queue.empty()) {
    auto* BB = Queue.front();
    Queue.pop();
    if(!DomNodes.count(BB)) continue;
    auto* DomNode = DomNodes[BB].get();
    for(auto* ChildBB : DomNode->doms()) {
      if(ChildBB == ToBB) return true;
      Queue.push(ChildBB);
    }
  }
  return false;
}

void GraphSchedule::ChangeDominator(BasicBlock* BB, BasicBlock* NewDomBB) {
  assert(DomNodes.count(BB));
  assert(DomNodes.count(NewDomBB));

  std::vector<LoopTreeNode*> StagingLoopNodes;
  // any loop header in (BB) subtree is invalid now
  for(auto SDI = subdom_begin(BB); !SDI.isEnd(); ++SDI) {
    auto* SubBB = *SDI;
    if(SubBB->getCtrlNode()->getOp() == IrOpcode::Loop)
      StagingLoopNodes.push_back(GetOrCreateLoopNode(SubBB));
  }

  auto* Node = DomNodes[BB].get();
  auto* OldDomBB = Node->getDominator();
  if(OldDomBB) {
    assert(DomNodes.count(OldDomBB));
    auto* OldDomNode = DomNodes[OldDomBB].get();
    OldDomNode->RemoveDomChild(BB);
  }
  Node->setDominator(NewDomBB);
  auto* NewDomNode = DomNodes[NewDomBB].get();
  NewDomNode->AddDomChild(BB);

  // update LoopTree
  for(auto* LoopNode : StagingLoopNodes) {
    auto* HeaderBB = LoopNode->getHeader();
    // search upward until hitting another loop header
    // or reach null
    assert(DomNodes.count(HeaderBB));
    auto* DN = DomNodes.at(HeaderBB).get();
    auto* PrevBB = HeaderBB;
    auto* NewBB = DN->getDominator();
    while(NewBB) {
      if(LoopTree.count(NewBB) &&
         PrevBB->getCtrlNode()->getOp() == IrOpcode::IfTrue)
        break;
      assert(DomNodes.count(NewBB));
      PrevBB = NewBB;
      DN = DomNodes.at(NewBB).get();
      NewBB = DN->getDominator();
    }
    if(NewBB != LoopNode->getParent()) {
      // remove the old link if any
      if(auto* ParentHeader = LoopNode->getParent()) {
        assert(LoopTree.count(ParentHeader));
        LoopTree[ParentHeader]->RemoveChildLoop(HeaderBB);
      }
      if(NewBB) {
        assert(LoopTree.count(NewBB));
        LoopTree[NewBB]->AddChildLoop(HeaderBB);
      }
      LoopNode->setParent(NewBB);
    }
  }
}

void GraphSchedule::OnConnectBlock(BasicBlock* Pred, BasicBlock* Succ) {
  if(!DomNodes.count(Pred))
    DomNodes[Pred] = gross::make_unique<DominatorNode>(Pred);
  if(!DomNodes.count(Succ))
    DomNodes[Succ] = gross::make_unique<DominatorNode>(Succ);

  // treat Pred as Succ's immediate Dominator
  // if there is only one predecessor on Succ
  if(Succ->pred_size() <= 1) {
    ChangeDominator(Succ, Pred);
  } else {
    // search upward until BB dominate Pred and previous
    // immediate dominator of Succ
    auto* SuccNode = DomNodes[Succ].get();
    auto* Dom = SuccNode->getDominator();
    auto* NewDom = Dom;
    while(NewDom &&
          !(Dominate(NewDom, Dom) &&
            Dominate(NewDom, Pred))) {
      assert(DomNodes.count(NewDom));
      auto* NewDomNode = DomNodes[NewDom].get();
      NewDom = NewDomNode->getDominator();
    }
    assert(NewDom);
    ChangeDominator(Succ, NewDom);
  }
}

namespace gross {
namespace _internal {
class CFGBuilder {
  // SubGraph will travel nodes in reverse BFS order
  SubGraph SG;
  GraphSchedule& Schedule;
  Graph& G;

  std::vector<Node*> ControlNodes;

  BasicBlock* MapBlock(Node* N) {
    return Schedule.MapBlock(N);
  }

  void AddNodeToBlock(Node* N, BasicBlock* BB) {
    Schedule.AddNode(BB, N);
  }
  void AddNodeToBlock(Node* N,
                      BasicBlock* BB,
                      typename BasicBlock::node_iterator Pos) {
    Schedule.AddNode(BB, Pos, N);
  }

  void BlockPlacement();

  void connectBlocks(BasicBlock* PredBB, BasicBlock* SuccBB) {
    PredBB->AddSuccBlock(SuccBB);
    SuccBB->AddPredBlock(PredBB);
    Schedule.OnConnectBlock(PredBB, SuccBB);
  }
  void ConnectBlock(Node* CtrlNode);

public:
  CFGBuilder(GraphSchedule& schedule)
    : SG(schedule.getSubGraph()),
      Schedule(schedule),
      G(Schedule.getGraph()) {}

  void Run() {
    BlockPlacement();

    for(auto* N : ControlNodes) {
      ConnectBlock(N);
    }
  }
};

void CFGBuilder::BlockPlacement() {
  auto* StartNode = Schedule.getStartNode();
  auto* EndNode = Schedule.getEndNode();

  for(auto* N : Schedule.rpo_nodes()) {
    if(Schedule.IsNodeScheduled(N)) continue;

    switch(N->getOp()) {
    case IrOpcode::Start:
    case IrOpcode::End:
    case IrOpcode::Loop: // loop header
    case IrOpcode::IfTrue:
    case IrOpcode::IfFalse:
    case IrOpcode::Merge: {
      auto* BB = Schedule.NewBasicBlock();
      AddNodeToBlock(N, BB);
      Schedule.SetFixed(N);
      break;
    }
    case IrOpcode::If:
    case IrOpcode::Return: {
      assert(N->getNumControlInput() > 0);
      auto* PrevCtrl = N->getControlInput(0);
      auto* BB = MapBlock(PrevCtrl);
      assert(BB && "previous control not visited yet?");
      AddNodeToBlock(N, BB);
      Schedule.SetScheduled(N);
      continue;
    }
    case IrOpcode::Phi: {
      assert(N->getNumControlInput() > 0);
      auto* PrevCtrl = N->getControlInput(0);
      auto* BB = MapBlock(PrevCtrl);
      assert(BB && "previous control not visited yet?");
      // add after the first(control) node
      auto Pos = BB->node_begin();
      ++Pos;
      AddNodeToBlock(N, BB, Pos);
      Schedule.SetScheduled(N);
      continue;
    }
    case IrOpcode::Call: {
      if(N->getNumControlInput() > 0) {
        auto* PrevCtrl = N->getControlInput(0);
        auto* BB = MapBlock(PrevCtrl);
        assert(BB && "previous control not visited yet?");
        // add after the first(control) node
        auto Pos = BB->node_begin();
        ++Pos;
        AddNodeToBlock(N, BB, Pos);
        Schedule.SetScheduled(N);
      }
      continue;
    }
    case IrOpcode::Alloca: {
      // needs to be placed in the entry block
      auto* EntryBlock = MapBlock(StartNode);
      assert(EntryBlock);
      if(G.IsGlobalVar(N)) {
        Schedule.SetScheduled(N);
        continue;
      }
      auto Pos = EntryBlock->node_begin();
      ++Pos;
      AddNodeToBlock(N, EntryBlock, Pos);
      Schedule.SetScheduled(N);
      continue;
    }
    case IrOpcode::ConstantInt:
    case IrOpcode::ConstantStr:
    case IrOpcode::FunctionStub:
    case IrOpcode::Argument: {
      // skip these Nodes
      Schedule.SetScheduled(N);
      continue;
    }
    default:
      continue;
    }
    ControlNodes.push_back(N);
  }
}

void CFGBuilder::ConnectBlock(Node* CtrlNode) {
  auto* EncloseBB = MapBlock(CtrlNode);
  assert(EncloseBB && "not enclosed in any BB?");

  switch(CtrlNode->getOp()) {
  default:
    return;
  case IrOpcode::Start:
  case IrOpcode::Phi:
  case IrOpcode::Return:
  case IrOpcode::If: {
    // there are chances that we need to connect
    // to these node from a backedge
    break;
  }
  case IrOpcode::Merge: {
    // merge from two branches
    for(auto* Br : CtrlNode->control_inputs()) {
      auto* BB = MapBlock(Br);
      assert(BB && "branch not enclosed in any BB?");
      connectBlocks(BB, EncloseBB);
    }
    break;
  }
  case IrOpcode::IfTrue:
  case IrOpcode::IfFalse: {
    NodeProperties<IrOpcode::VirtIfBranches> NP(CtrlNode);
    auto* PrevBB = MapBlock(NP.BranchPoint());
    assert(PrevBB && "If node not enclosed in any BB?");
    connectBlocks(PrevBB, EncloseBB);
    break;
  }
  case IrOpcode::Loop: {
    // connect except the back edge dep
    NodeProperties<IrOpcode::Loop> LNP(CtrlNode);
    auto* Backedge = LNP.Backedge();
    for(auto* CI : CtrlNode->control_inputs()) {
      if(CI == Backedge) continue;
      auto* PrevBB = MapBlock(CI);
      assert(PrevBB);
      connectBlocks(PrevBB, EncloseBB);
    }
    break;
  }
  case IrOpcode::End: {
    std::set<BasicBlock*> PrevBBs;
    BasicBlock* EntryBlock = nullptr;
    for(auto* CI : CtrlNode->control_inputs()) {
      auto* PrevBB = MapBlock(CI);
      if(!PrevBB) continue;
      if(CI->getOp() == IrOpcode::Start) {
        EntryBlock = PrevBB;
        continue;
      }
      // eliminate duplicate
      PrevBBs.insert(PrevBB);
    }
    if(PrevBBs.empty() && EntryBlock) PrevBBs.insert(EntryBlock);
    for(auto* BB : PrevBBs) {
      connectBlocks(BB, EncloseBB);
    }
    break;
  }
  }
  // if this is the back edge of a loop
  // connect now
  for(auto* CU : CtrlNode->control_users()) {
    if(CU->getOp() == IrOpcode::Loop &&
       NodeProperties<IrOpcode::Loop>(CU).Backedge() == CtrlNode) {
      auto* LoopHeader = MapBlock(CU);
      assert(LoopHeader);
      connectBlocks(EncloseBB, LoopHeader);
    }
  }
}

class PostOrderNodePlacement {
  GraphSchedule& Schedule;

  template<class SetT>
  BasicBlock* GetCommonDominator(const SetT& BBs);

  BasicBlock* FindLoopInvariantPos(Node* N, BasicBlock* BB);

public:
  PostOrderNodePlacement(GraphSchedule& schedule)
    : Schedule(schedule) {}

  void Compute();
};

template<class SetT> BasicBlock*
PostOrderNodePlacement::GetCommonDominator(const SetT& BBs) {
  // find the BB with smallest RPO index
  auto BI = Schedule.po_begin(),
       BIE = Schedule.po_end();
  size_t Counter = 0U, BBSize = BBs.size();
  for(; BI != BIE; ++BI) {
    if(BBs.count(*BI)) ++Counter;
    // we want to break on block that has the least RPO index
    if(Counter >= BBSize) break;
  }
  for(; BI != BIE; ++BI) {
    auto* DomBB = *BI;
    bool Found = true;
    for(auto* BB : BBs) {
      if(!Schedule.Dominate(DomBB, BB)) {
        Found = false;
        break;
      }
    }
    if(Found) return DomBB;
  }
  return nullptr;
}

BasicBlock* PostOrderNodePlacement::FindLoopInvariantPos(Node* CurNode,
                                                         BasicBlock* OrigBB) {
  auto* SafeBB = OrigBB;
  auto* NewBB = OrigBB;
  BasicBlock* LastLoopHeader = nullptr;
  // find the closest loop
  while(NewBB) {
    if(Schedule.IsLoopHeader(NewBB)) {
      LastLoopHeader = NewBB;
    }
    NewBB = Schedule.getDominator(NewBB);
    if(LastLoopHeader) break;
  }
  if(!NewBB) return SafeBB;

  // still need to be after all input nodes if scheduled
  std::set<BasicBlock*> InputBBs;
  for(auto* IN : CurNode->value_inputs()) {
    if(Schedule.IsNodeScheduled(IN)) {
      if(auto* InputBB = Schedule.MapBlock(IN)) {
        InputBBs.insert(InputBB);
      }
    }
  }
  for(auto* IN : CurNode->effect_inputs()) {
    if(Schedule.IsNodeScheduled(IN)) {
      if(auto* InputBB = Schedule.MapBlock(IN)) {
        InputBBs.insert(InputBB);
      }
    }
  }

  auto isSafe = [this,&InputBBs](BasicBlock* BB) -> bool {
    for(auto* InputBB : InputBBs) {
      if(!Schedule.Dominate(InputBB, BB)) return false;
    }
    return true;
  };

  while(true) {
    if(!isSafe(NewBB)) break;
    SafeBB = NewBB;

    LastLoopHeader = Schedule.getParentLoop(LastLoopHeader);
    if(!LastLoopHeader) break; // reach the root
    NewBB = Schedule.getDominator(LastLoopHeader);
  }
  return SafeBB;
}

void PostOrderNodePlacement::Compute() {
  for(auto* CurNode : Schedule.po_nodes()) {
    if(Schedule.IsNodeScheduled(CurNode)) continue;
    if(NodeProperties<IrOpcode::VirtConstantValues>(CurNode))
      continue;

    // consider all the value users first
    std::unordered_set<BasicBlock*> UserBBs;
    std::unordered_set<Node*> UserNodes;
    auto collectUsages = [&,this](Node* UN, Use::Kind UseKind) {
      assert(Schedule.IsNodeScheduled(UN) &&
             "User not scheduled?");
      UserNodes.insert(UN);

      BasicBlock* BB = nullptr;
      if(UN->getOp() == IrOpcode::Phi) {
        // don't need to dominate Phi block,
        // use the branch block instead
        NodeProperties<IrOpcode::Phi> PNP(UN);
        auto* CN = PNP.MapCtrlNode(CurNode, UseKind);
        assert(CN && "failed to map input to control node");
        BB = Schedule.MapBlock(CN);
      } else {
        BB = Schedule.MapBlock(UN);
      }
      assert(BB);
      UserBBs.insert(BB);
    };

    for(auto* VU : CurNode->value_users()) {
      collectUsages(VU, Use::K_VALUE);
    }
    for(auto* EU : CurNode->effect_users()) {
      collectUsages(EU, Use::K_EFFECT);
    }

    if(!UserBBs.empty()) {
      auto* DomBB = GetCommonDominator(UserBBs);
      assert(DomBB);
      // Don't put node in a loop
      // FIXME: post-order placement is not suitable to do
      // this kind of optimization as there is no input nodes
      // scheduling information.
      //DomBB = FindLoopInvariantPos(CurNode, DomBB);

      // search from top to bottom within the block,
      // insert right before a value user if any. Otherwise
      // insert at the end
      auto NI = DomBB->node_begin();
      for(auto NE = DomBB->node_end(); NI != NE; ++NI) {
        auto* N = *NI;
        if(UserNodes.count(N)) break;
        // insert before terminate instructions
        // FIXME: Is there any other way to generalize this
        // concept
        else if(NodeProperties<IrOpcode::VirtTerminate>(N))
          break;
      }
      Schedule.AddNode(DomBB, NI, CurNode);
      Schedule.SetScheduled(CurNode);
      continue;
    }
  }
}
} // end namespace _internal
} // end namespace gross

struct GraphSchedule::DomTreeProxy {
  using map_type = GraphSchedule::DomNodesTy;
  using map_iterator = typename map_type::iterator;
  using block_iterator
    = boost::transform_iterator<gross::unique_ptr_unwrapper<BasicBlock>,
                                typename GraphSchedule::block_iterator,
                                BasicBlock*, // Refrence type
                                BasicBlock* // Value type
                                >;

  using value_type = DominatorNode*;
  using child_iterator = typename DominatorNode::dom_iterator;

  static map_type& GetMap(GraphSchedule& Schedule) {
    return Schedule.DomNodes;
  }

  static value_type GetValue(map_iterator MapIt) {
    return MapIt->second.get();
  }

  static block_iterator block_begin(GraphSchedule& Schedule) {
    gross::unique_ptr_unwrapper<BasicBlock> functor;
    return block_iterator(Schedule.block_begin(), std::move(functor));
  }
  static block_iterator block_end(GraphSchedule& Schedule) {
    gross::unique_ptr_unwrapper<BasicBlock> functor;
    return block_iterator(Schedule.block_end(), std::move(functor));
  }
  static size_t block_size(GraphSchedule& Schedule) {
    return Schedule.block_size();
  }

  static child_iterator child_begin(value_type Value) {
    return Value->dom_begin();
  }
  static child_iterator child_end(value_type Value) {
    return Value->dom_end();
  }
  static size_t child_size(value_type Value) {
    return Value->dom_size();
  }
};

struct GraphSchedule::LoopTreeProxy {
  using map_type = GraphSchedule::LoopTreeTy;
  using map_iterator = typename map_type::iterator;
  struct key_extractor {
    BasicBlock*
    operator()(typename map_type::value_type& Pair) const noexcept {
      return Pair.first;
    }
  };
  using block_iterator
    = boost::transform_iterator<key_extractor, map_iterator,
                                BasicBlock*, // Refrence type
                                BasicBlock* // Value type
                                >;

  using value_type = LoopTreeNode*;
  using child_iterator = typename LoopTreeNode::loop_iterator;

  static map_type& GetMap(GraphSchedule& Schedule) {
    return Schedule.LoopTree;
  }

  static value_type GetValue(map_iterator MapIt) {
    return MapIt->second.get();
  }

  static block_iterator block_begin(GraphSchedule& Schedule) {
    key_extractor functor;
    return block_iterator(Schedule.LoopTree.begin(),
                          std::move(functor));
  }
  static block_iterator block_end(GraphSchedule& Schedule) {
    key_extractor functor;
    return block_iterator(Schedule.LoopTree.end(),
                          std::move(functor));
  }
  static size_t block_size(GraphSchedule& Schedule) {
    return Schedule.LoopTree.size();
  }

  static child_iterator child_begin(value_type Value) {
    return Value->child_loop_begin();
  }
  static child_iterator child_end(value_type Value) {
    return Value->child_loop_end();
  }
  static size_t child_size(value_type Value) {
    return Value->child_loop_size();
  }
};

template<class ProxyT>
struct GraphSchedule::TreeHandle {
  /// GraphConcept
  using vertex_descriptor = BasicBlock*;
  // { source, dest }
  using edge_descriptor = std::pair<vertex_descriptor, vertex_descriptor>;
  using directed_category = boost::directed_tag;
  using edge_parallel_category = boost::allow_parallel_edge_tag;

  static vertex_descriptor null_vertex() {
    return nullptr;
  }

  /// VertexListGraphConcept
  using vertex_iterator = typename ProxyT::block_iterator;
  using vertices_size_type = size_t;

  /// EdgeListGraphConcept
  using edges_size_type = size_t;
  class edge_iterator : public boost::iterator_facade<
        edge_iterator,
        typename TreeHandle<ProxyT>::edge_descriptor,
        boost::forward_traversal_tag,
        typename TreeHandle<ProxyT>::edge_descriptor> {
    friend class boost::iterator_core_access;
    GraphSchedule* Schedule;

    using edge_type = typename TreeHandle<ProxyT>::edge_descriptor;

    using MapTy = typename ProxyT::map_type;
    using ValueTy = typename ProxyT::value_type;
    using iterator = typename MapTy::iterator;
    // iterator over the map
    iterator MapIt;

    typename ProxyT::child_iterator ChildIt;

    bool IsTombstone;

    MapTy& Map() const {
      assert(Schedule);
      return ProxyT::GetMap(*Schedule);
    }
    ValueTy CurValue() const {
      assert(MapIt != Map().end());
      return ProxyT::GetValue(MapIt);
    }

    bool equal(const edge_iterator& Other) const {
      // iterator for unordered_map is unstable,
      // so it's nearly impossible to compare two arbitrary iterator.
      // Thus we only care about the case comparing with end iterator
      if(Other.IsTombstone) {
        return MapIt == Map().end();
      }
      return false;
    }

    void nextValidPos() {
      while(MapIt != Map().end() &&
            ChildIt == ProxyT::child_end(CurValue()) ) {
        ++MapIt;
        if(MapIt != Map().end())
          ChildIt = ProxyT::child_begin(CurValue());
      }
    }

    void increment() {
      ++ChildIt;
      nextValidPos();
    }

    edge_type dereference() const {
      return std::make_pair(MapIt->first, *ChildIt);
    }

  public:
    edge_iterator()
      : Schedule(nullptr),
        IsTombstone(false) {}
    edge_iterator(GraphSchedule* schedule, bool IsEnd = false)
      : Schedule(schedule),
        IsTombstone(IsEnd) {
      if(!IsEnd) {
        MapIt = Map().begin();
        if(MapIt != Map().end()) {
          ChildIt = ProxyT::child_begin(CurValue());
          nextValidPos();
        }
      }
    }
  };
  edge_iterator edge_begin() const {
    return edge_iterator(&pImpl);
  }
  edge_iterator edge_end() const {
    return edge_iterator(&pImpl, true);
  }
  size_t edge_size() const {
    auto& Map = ProxyT::GetMap(pImpl);
    size_t Sum = 0U;
    for(auto I = Map.begin(), E = Map.end(); I != E; ++I) {
      auto* Val = ProxyT::GetValue(I);
      Sum += ProxyT::child_size(Val);
    }
    return Sum;
  }

  struct traversal_category :
    public boost::vertex_list_graph_tag,
    public boost::edge_list_graph_tag {};

  TreeHandle(GraphSchedule& Impl) : pImpl(Impl) {}
  GraphSchedule& getImpl() const { return pImpl; }

private:
  GraphSchedule& pImpl;
};

std::ostream& GraphSchedule::printBlock(std::ostream& OS, BasicBlock* BB) {
  auto printInputNode = [this,&OS,BB](Node* Input) {
    if(auto* DestBB = MapBlock(Input)) {
      if(DestBB != BB) {
        DestBB->getId().print(OS);
        OS << "/";
      }
      assert(DestBB->getNodeId(Input));
      DestBB->getNodeId(Input)->print(OS);
    }
  };

  BB->getId().print(OS << "BB");
  OS << " :\n";
  for(auto* N : BB->nodes()) {
    assert(BB->getNodeId(N));
    BB->getNodeId(N)->print(OS);
    OS << ": ";

    // special handle for EffectPhi
    if(N->getOp() == IrOpcode::Phi &&
       !N->getNumValueInput() && N->getNumEffectInput()) {
      OS << "EffectPhi ";
      for(auto* EI : N->effect_inputs()) {
        OS << " ";
        printInputNode(EI);
      }
      OS << "\n";
      continue;
    }

    IrOpcode::Print(G, OS, N);
    for(auto* VI : N->value_inputs()) {
      OS << " ";
      if(VI->getOp() == IrOpcode::ConstantInt) {
        OS << "#"
           << NodeProperties<IrOpcode::ConstantInt>(VI)
              .as<int32_t>(G);
      } else if(NodeProperties<IrOpcode::VirtDLXRegisters>(VI)) {
        //IrOpcode::Print(G, OS, VI);
        auto Offset = static_cast<unsigned>(VI->getOp() - IrOpcode::DLXr0);
        OS << "r" << Offset;
      } else if(VI->getOp() == IrOpcode::DLXOffset) {
        if(BasicBlock** OffsetBB = BlockOffsets.find_value(VI)) {
          OS << "<";
          (*OffsetBB)->getId().print(OS);
          OS << ">";
        }
      } else if(VI->getOp() == IrOpcode::Argument) {
        IrOpcode::Print(G, OS, VI);
      } else {
        printInputNode(VI);
      }
    }
    OS << "\n";
  }
  return OS;
}

struct GraphSchedule::block_prop_writer {
  block_prop_writer(GraphSchedule& g)
    : Schedule(g) {}

  void operator()(std::ostream& OS, const BasicBlock* v) const {
    auto* BB = const_cast<BasicBlock*>(v);
    OS << "[shape=record,";
    OS << "label=\"";
    std::stringstream SS;
    Schedule.printBlock(SS, BB);
    // adapt the string
    auto OutStr = SS.str();
    for(auto i = 0U; i < OutStr.size(); ++i) {
      if(OutStr[i] == '\n') {
        // replace with left-justified '\l'
        OutStr[i] = '\\';
        OutStr.insert(OutStr.begin() + i + 1, 'l');
      } else if(OutStr[i] == '<') {
        // replace with '&lt;'
        OutStr[i] = '&';
        OutStr.insert(OutStr.begin() + i + 1, ';');
        OutStr.insert(OutStr.begin() + i + 1, 't');
        OutStr.insert(OutStr.begin() + i + 1, 'l');
      } else if(OutStr[i] == '>') {
        // replace with '&gt;'
        OutStr[i] = '&';
        OutStr.insert(OutStr.begin() + i + 1, ';');
        OutStr.insert(OutStr.begin() + i + 1, 't');
        OutStr.insert(OutStr.begin() + i + 1, 'g');
      }
    }
    OS << OutStr << "\"]";
  }

private:
  GraphSchedule& Schedule;
};

void GraphSchedule::dumpGraphviz(std::ostream& OS) {
  boost::write_graphviz(OS, *this,
                        block_prop_writer(*this));
}

// graph_traits for DomTree. So far we only need the trait
// for Graphviz, so put it here
namespace gross {
template<class ProxyT>
std::pair<typename GraphSchedule::TreeHandle<ProxyT>::vertex_iterator,
          typename GraphSchedule::TreeHandle<ProxyT>::vertex_iterator>
vertices(const GraphSchedule::TreeHandle<ProxyT>& g) {
  auto& Impl = g.getImpl();
  return std::make_pair(ProxyT::block_begin(Impl),
                        ProxyT::block_end(Impl));
}
template<class ProxyT>
size_t num_vertices(const GraphSchedule::TreeHandle<ProxyT>& g) {
  return ProxyT::block_size(g.getImpl());
}

template<class ProxyT>
std::pair<typename GraphSchedule::TreeHandle<ProxyT>::edge_iterator,
          typename GraphSchedule::TreeHandle<ProxyT>::edge_iterator>
edges(const GraphSchedule::TreeHandle<ProxyT>& g) {
  return std::make_pair(g.edge_begin(), g.edge_end());
}
template<class ProxyT>
size_t num_edges(const GraphSchedule::TreeHandle<ProxyT>& g) {
  return g.edge_size();
}

/// Well...there is a default implementation of source/target for std::pair
/// in boost
//template<class ProxyT>
//typename GraphSchedule::TreeHandle<ProxyT>::vertex_descriptor
//source(const typename GraphSchedule::TreeHandle<ProxyT>::edge_descriptor& e,
//       const GraphSchedule::TreeHandle<ProxyT>& g) {
//  return const_cast<BasicBlock*>(e.first);
//}
//template<class ProxyT>
//typename GraphSchedule::TreeHandle<ProxyT>::vertex_descriptor
//target(const typename GraphSchedule::TreeHandle<ProxyT>::edge_descriptor& e,
//       const GraphSchedule::TreeHandle<ProxyT>& g) {
//  return const_cast<BasicBlock*>(e.second);
//}

// get() for getting vertex id property map from graph
template<class ProxyT>
inline gross::GraphScheduleVertexIdMap
get(boost::vertex_index_t tag, const GraphSchedule::TreeHandle<ProxyT>& g) {
  return gross::GraphScheduleVertexIdMap(g.getImpl());
}
} // end namespace gross

void GraphSchedule::dumpDomTreeGraphviz(std::ostream& OS) {
  TreeHandle<DomTreeProxy> DomTree(*this);
  boost::write_graphviz(OS, DomTree,
                        block_prop_writer(*this));
}
void GraphSchedule::dumpLoopTreeGraphviz(std::ostream& OS) {
  TreeHandle<LoopTreeProxy> LoopTree(*this);
  boost::write_graphviz(OS, LoopTree,
                        block_prop_writer(*this));
}

/// ======= GraphScheduler ========
GraphScheduler::GraphScheduler(Graph& graph) : G(graph) {
  // create new schedule for each SubGraph
  for(auto& SG : G.subregions()) {
    Schedules.emplace_back(new GraphSchedule(G, SG));
  }
}

typename GraphScheduler::schedule_iterator
GraphScheduler::schedule_begin() {
  gross::unique_ptr_unwrapper<GraphSchedule> Functor;
  return schedule_iterator(Schedules.begin(), Functor);
}
typename GraphScheduler::schedule_iterator
GraphScheduler::schedule_end() {
  gross::unique_ptr_unwrapper<GraphSchedule> Functor;
  return schedule_iterator(Schedules.end(), Functor);
}

void GraphScheduler::ComputeScheduledGraph() {
  for(auto& SchedulePtr : Schedules) {
    auto& Schedule = *SchedulePtr.get();

    // Phase 1. Build CFG and insert fixed nodes
    _internal::CFGBuilder CFB(Schedule);
    CFB.Run();

    // Phase 2. Place rest of the nodes.
    _internal::PostOrderNodePlacement POP(Schedule);
    POP.Compute();
  }
}
