#ifndef GROSS_GRAPH_REDUCTIONS_MEMORY_LEGALIZE_H
#define GROSS_GRAPH_REDUCTIONS_MEMORY_LEGALIZE_H
#include "gross/Graph/GraphReducer.h"

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
} // end namespace gross
#endif
