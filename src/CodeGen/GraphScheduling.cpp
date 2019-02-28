#include "BGL.h"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/graphviz.hpp"
#include "gross/CodeGen/GraphScheduling.h"
#include "gross/Graph/Node.h"
#include "gross/Graph/NodeUtils.h"
#include <queue>
#include <vector>
#include <iostream>

using namespace gross;

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

  std::vector<Node*> ControlNodes;

  BasicBlock* MapBlock(Node* N) {
    return Schedule.MapBlock(N);
  }

  void AddNodeToBlock(Node* N, BasicBlock* BB) {
    Schedule.AddNode(BB, N);
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
      Schedule(schedule) {}

  void Run() {
    BlockPlacement();

    for(auto* N : ControlNodes) {
      ConnectBlock(N);
    }

    Schedule.SortRPO();
  }
};

void CFGBuilder::BlockPlacement() {
  // we want normal BFS traveling order here
  std::vector<Node*> RPONodes;
  // force Start/End nodes be the first/last nodes
  Node *StartNode = nullptr, *EndNode = nullptr;
  for(auto* N : SG.nodes()) {
    switch(N->getOp()) {
    case IrOpcode::Start: {
      StartNode = N;
      break;
    }
    case IrOpcode::End: {
      EndNode = N;
      break;
    }
    case IrOpcode::Loop: {
      // put off Loop node to be inserted after
      // backedge(i.e. IfTrue node) is inserted
      break;
    }
    case IrOpcode::IfTrue: {
      RPONodes.insert(RPONodes.cbegin(), N);
      for(auto* CU : N->control_users()) {
        if(CU->getOp() == IrOpcode::Loop) {
          // backedge
          RPONodes.insert(RPONodes.cbegin(), CU);
        }
      }
      break;
    }
    default:
      RPONodes.insert(RPONodes.cbegin(), N);
    }
  }
  assert(StartNode && EndNode);
  RPONodes.insert(RPONodes.cbegin(), StartNode);
  RPONodes.insert(RPONodes.cend(), EndNode);

  for(auto* N : RPONodes) {
    switch(N->getOp()) {
    case IrOpcode::Start:
    case IrOpcode::End:
    case IrOpcode::Loop: // loop header
    case IrOpcode::IfTrue:
    case IrOpcode::IfFalse:
    case IrOpcode::Merge: {
      auto* BB = Schedule.NewBasicBlock();
      AddNodeToBlock(N, BB);
      break;
    }
    case IrOpcode::Phi:
    case IrOpcode::If: {
      assert(N->getNumControlInput() > 0);
      auto* PrevCtrl = N->getControlInput(0);
      auto* BB = MapBlock(PrevCtrl);
      assert(BB && "previous control not visited yet?");
      AddNodeToBlock(N, BB);
      continue;
    }
    case IrOpcode::Alloca: {
      // needs to be placed in the entry block
      // TODO: tell local alloca from global variable
      auto* EntryBlock = MapBlock(StartNode);
      assert(EntryBlock);
      AddNodeToBlock(N, EntryBlock);
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
  case IrOpcode::Merge: {
    // merge from two branches
    for(auto* Br : CtrlNode->control_inputs()) {
      auto* BB = MapBlock(Br);
      assert(BB && "branch not enclosed in any BB?");
      EncloseBB->AddPredBlock(BB);
      BB->AddSuccBlock(EncloseBB);
    }
    break;
  }
  case IrOpcode::IfTrue:
  case IrOpcode::IfFalse: {
    NodeProperties<IrOpcode::VirtIfBranches> NP(CtrlNode);
    auto* PrevBB = MapBlock(NP.BranchPoint());
    assert(PrevBB && "If node not enclosed in any BB?");
    connectBlocks(PrevBB, EncloseBB);
    // if IfTrue is the back edge of a loop
    // connect now
    if(CtrlNode->getOp() == IrOpcode::IfTrue) {
      for(auto* CU : CtrlNode->control_users()) {
        if(CU->getOp() == IrOpcode::Loop) {
          auto* LoopHeader = MapBlock(CU);
          assert(LoopHeader);
          connectBlocks(EncloseBB, LoopHeader);
        }
      }
    }
    break;
  }
  case IrOpcode::Loop: {
    // connect except the back edge dep
    NodeProperties<IrOpcode::Loop> LNP(CtrlNode);
    auto* TrueBr = NodeProperties<IrOpcode::If>(LNP.Branch())
                   .TrueBranch();
    for(auto* CI : CtrlNode->control_inputs()) {
      if(CI == TrueBr) continue;
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
      //assert(PrevBB);
      // FIXME: should Return node be the side effect
      // dependency for End node?
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
}

class PostOrderNodePlacement {
  GraphSchedule& Schedule;

  std::vector<Node*> WorkQueue;

  void Push(Node* N) { WorkQueue.push_back(N); }
  void Pop() { WorkQueue.erase(WorkQueue.cbegin()); }

public:
  PostOrderNodePlacement(GraphSchedule& schedule)
    : Schedule(schedule) {}

  void Compute();
};

void PostOrderNodePlacement::Compute() {
  // Although SubGraph will walk nodes in PO order,
  // we want more control on Node queuing
  while(!WorkQueue.empty()) {
    auto* NextNode = WorkQueue.front();
    // TODO: Put in the block that dominate all value users
  }
}
} // end namespace _internal
} // end namespace gross

struct GraphSchedule::RPOVisitor
  : public boost::default_dfs_visitor {
  void finish_vertex(BasicBlock* BB, const GraphSchedule& G) {
    Trace.insert(Trace.cbegin(), BB);
  }

  RPOVisitor() = delete;
  RPOVisitor(std::vector<BasicBlock*>& trace) : Trace(trace) {}

private:
  std::vector<BasicBlock*>& Trace;
};

void GraphSchedule::SortRPO() {
  RPOBlocks.clear();
  RPOVisitor Vis(RPOBlocks);
  std::unordered_map<BasicBlock*,boost::default_color_type> ColorStorage;
  StubColorMap<decltype(ColorStorage), BasicBlock> ColorMap(ColorStorage);
  boost::depth_first_search(*this, Vis, std::move(ColorMap));
}

std::ostream& GraphSchedule::printBlock(std::ostream& OS, BasicBlock* BB) {
  for(auto* N : BB->nodes()) {
    assert(BB->getNodeId(N));
    BB->getNodeId(N)->print(OS);
    OS << " = ";
    IrOpcode::Print(G, OS, N);
    for(auto* VI : N->value_inputs()) {
      // ignore non-value inputs here
      OS << " ";
      if(VI->getOp() == IrOpcode::ConstantInt) {
        OS << "#"
           << NodeProperties<IrOpcode::ConstantInt>(VI)
              .as<int32_t>(G);
      } else if(auto* DestBB = MapBlock(VI)) {
        if(DestBB != BB) {
          DestBB->getId().print(OS);
          OS << "/";
        }
        assert(DestBB->getNodeId(VI));
        DestBB->getNodeId(VI)->print(OS);
      } else {
        continue;
      }
    }
    OS << "\n";
  }
  return OS;
}

void GraphSchedule::dumpGraphviz(std::ostream& OS) {
  struct block_prop_writer {
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
          OutStr.insert(OutStr.cbegin() + i + 1, 'l');
        }
      }
      OS << OutStr << "\"]";
    }

  private:
    GraphSchedule& Schedule;
  };

  boost::write_graphviz(OS, *this,
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
