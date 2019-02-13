#ifndef GROSS_GRAPH_REDUCTIONS_PEEPHOLE_H
#define GROSS_GRAPH_REDUCTIONS_PEEPHOLE_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
class PeepholeReducer {
  GraphReduction ReduceArithmetic(Node* N);
  GraphReduction ReduceRelation(Node* N);

  Graph& G;
  Node* DeadNode;

public:
  PeepholeReducer(Graph& graph);

  static constexpr
  const char* name() { return "peephole"; }

  GraphReduction Reduce(Node* N);
};
} // end namespace gross
#endif
