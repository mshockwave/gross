#ifndef GROSS_GRAPH_REDUCTIONS_MEMORY_NORMALIZE_H
#define GROSS_GRAPH_REDUCTIONS_MEMORY_NORMALIZE_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
struct MemoryNormalize : public GraphEditor {
  explicit MemoryNormalize(GraphEditor::Interface* editor);

  static constexpr
  const char* name() { return "memory-normalize"; }

  GraphReduction Reduce(Node* N);

private:
  GraphReduction ReduceMutableMemOps(Node* N);
};
} // end namespace gross
#endif
