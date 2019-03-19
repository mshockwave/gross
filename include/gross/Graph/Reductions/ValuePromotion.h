#ifndef GROSS_GRAPH_REDUCTIONS_VALUE_PROMOTION_H
#define GROSS_GRAPH_REDUCTIONS_VALUE_PROMOTION_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
class ValuePromotion : public GraphEditor {
  GraphReduction ReduceAssignment(Node* N);
  GraphReduction ReduceMemAssignment(Node* N);
  GraphReduction ReduceVarAccess(Node* N);
  GraphReduction ReduceArrayDecl(Node* N);
  GraphReduction ReduceMemAccess(Node* N);
  GraphReduction ReducePhiNode(Node* N);

  void appendValueUsage(Node* Usr, Node* Src, Node* Val);

  Graph& G;
  Node* DeadNode;

public:
  explicit ValuePromotion(GraphEditor::Interface* editor);

  static constexpr
  const char* name() { return "value-promotion"; }

  GraphReduction Reduce(Node* N);
};
} // end namespace gross
#endif
