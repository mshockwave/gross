#ifndef GROSS_GRAPH_REDUCTIONS_DCE_H
#define GROSS_GRAPH_REDUCTIONS_DCE_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
struct DCEReducer {
  static constexpr
  const char* name() { return "dce"; }

  GraphReduction Reduce(Node* N);

private:
  GraphReduction EliminateDead(Node* N);
};
} // end namespace gross
#endif
