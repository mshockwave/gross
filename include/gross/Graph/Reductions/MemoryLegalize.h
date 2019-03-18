#ifndef GROSS_GRAPH_REDUCTIONS_MEMORY_LEGALIZE_H
#define GROSS_GRAPH_REDUCTIONS_MEMORY_LEGALIZE_H
#include "gross/Graph/GraphReducer.h"
#include <set>

namespace gross {
struct MemoryLegalize : public GraphEditor {
  explicit MemoryLegalize(GraphEditor::Interface* editor);

  static constexpr
  const char* name() { return "memory-normalize"; }

  GraphReduction Reduce(Node* N);

private:
  Graph& G;

  GraphReduction ReduceMemStore(Node* N);
};

// merge local and global allocas into single chunk
// for easier CodeGen.
// Since this require concept of function, can not be done
// with GraphReducer
struct MemAllocationLowering {
  MemAllocationLowering(Graph& graph) : G(graph) {}

  void Run();

private:
  Graph& G;

  Node* MergeAllocas(const std::set<Node*>&);

  void RunOnFunction(SubGraph& SG);
};
} // end namespace gross
#endif
