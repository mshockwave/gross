#ifndef GROSS_GRAPH_REDUCTIONS_DCE_H
#define GROSS_GRAPH_REDUCTIONS_DCE_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
struct DCEReducer {
  DCEReducer() : DeadNode(nullptr) {}

  static constexpr
  const char* name() { return "dce"; }

  void Reduce(Graph& G);

private:
  Node* DeadNode;

  void EliminateUnreachable(Graph& G);
  // remove always true/false control structures
  void DeadControlElimination(Graph& G);
};
} // end namespace gross
#endif
