#ifndef GROSS_CODEGEN_GRAPHSCHEDULING_H
#define GROSS_CODEGEN_GRAPHSCHEDULING_H
#include "gross/CodeGen/Function.h"
#include "gross/CodeGen/BasicBlock.h"
#include "gross/Graph/Graph.h"
#include <list>
#include <memory>

namespace gross {
// owner of basic blocks
class GraphSchedule {
  std::list<std::unique_ptr<BasicBlock>> Blocks;

public:
  BasicBlock* NewBasicBlock();

  void AddNode(BasicBlock* BB, Node* N);
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
