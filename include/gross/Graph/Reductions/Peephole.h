#ifndef GROSS_GRAPH_REDUCTIONS_PEEPHOLE_H
#define GROSS_GRAPH_REDUCTIONS_PEEPHOLE_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
class PeepholeReducer : public GraphEditor {
  GraphReduction ReduceArithmetic(Node* N);
  GraphReduction ReduceRelation(Node* N);

  GraphReduction DeadPHIElimination(Node* N);

  Graph& G;
  Node* DeadNode;

public:
  PeepholeReducer(GraphEditor::Interface* editor);

  static constexpr
  const char* name() { return "peephole"; }

  GraphReduction Reduce(Node* N);
};
} // end namespace gross
#endif
