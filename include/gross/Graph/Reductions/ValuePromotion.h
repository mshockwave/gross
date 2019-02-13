#ifndef GROSS_GRAPH_REDUCTIONS_VALUE_PROMOTION_H
#define GROSS_GRAPH_REDUCTIONS_VALUE_PROMOTION_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
class ValuePromotion {
  GraphReduction ReduceAssignment(Node* N);
  GraphReduction ReduceMemAssignment(Node* N);
  GraphReduction ReduceVarAccess(Node* N);
  GraphReduction ReduceMemAccess(Node* N);
  GraphReduction ReducePhiNode(Node* N);

  Graph& G;
  Node* DeadNode;

public:
  ValuePromotion(Graph& graph);

  static constexpr
  const char* name() { return "value-promotion"; }

  GraphReduction Reduce(Node* N);
};
} // end namespace gross
#endif
