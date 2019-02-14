#include "GraphScheduling.h"
#include <vector>

using namespace gross;

BasicBlock* GraphSchedule::NewBasicBlock() {
  auto* BB = new BasicBlock();
  Blocks.emplace_back(BB);
  return Blocks.back().get();
}

namespace gross {
namespace _internal {
class CFGBuilder {
  // SubGraph will travel nodes in reverse BFS order
  SubGraph SG;
  GraphSchedule& Schedule;

  std::vector<Node*> ControlNodes;

  void BlockPlacement();

  void ConnectBlocks();

public:
  CFGBuilder(const SubGraph& subgraph, GraphSchedule& schedule)
    : SG(subgraph),
      Schedule(schedule) {}

  void Run() {
  }
};

void CFGBuilder::BlockPlacement() {
  for(auto* N : SG.nodes()) {
    switch(N->getOp()) {
    case IrOpcode::Start:
      break;
    case IrOpcode::End:
      break;
    case IrOpcode::If:
      break;
    default:
      continue;
    }
  }
}

void CFGBuilder::ConnectBlocks() {
}
} // end namespace _internal
} // end namespace gross

void GraphSchedule::AddNode(BasicBlock* BB, Node* N) {
}

GraphScheduler::GraphScheduler(Graph& graph) : G(graph) {}

void GraphScheduler::ComputeScheduledGraph() {
  // Phases:
  // 1. Build CFG. Insert fix nodes
  // 2. Insert rest of the nodes according to
  //    control dependencies.
}
