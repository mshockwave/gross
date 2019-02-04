#ifndef GROSS_GRAPH_REDUCTIONS_VALUE_PROMOTION_H
#define GROSS_GRAPH_REDUCTIONS_VALUE_PROMOTION_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
class ValuePromotion {
public:
  ValuePromotion() = default;

  static constexpr
  const char* name() { return "value-promotion"; }

  GraphReduction Reduce(Node* N);
};
} // end namespace gross
#endif
