#ifndef GROSS_GRAPH_REDUCTIONS_DCE_H
#define GROSS_GRAPH_REDUCTIONS_DCE_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
struct DCEReducer {
  static constexpr
  const char* name() { return "dce"; }

  void Reduce(Graph& G);

private:
  void EliminateUnreachable(Graph& G);
};
} // end namespace gross
#endif
