#ifndef GROSS_GRAPH_REDUCTIONS_DCE_H
#define GROSS_GRAPH_REDUCTIONS_DCE_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
struct DCEReducer {
  DCEReducer(Graph& graph) : G(graph) {}

  static constexpr
  const char* name() { return "dce"; }

  GraphReduction Reduce(Node* N);

private:
  Graph& G;
  GraphReduction EliminateDead(Node* N);
};
} // end namespace gross
#endif
