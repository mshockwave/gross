#ifndef GROSS_GRAPH_REDUCTIONS_CSE_H
#define GROSS_GRAPH_REDUCTIONS_CSE_H
#include "gross/Graph/GraphReducer.h"
#include <unordered_set>

namespace gross {
class CSEReducer : public GraphEditor {
  Graph& G;
  // FIXME: these state information would preserved through
  // the entire graph instead of function subgraph
  // FIXME: just a temporary storage for trivial cases
  std::unordered_set<NodeHandle> TrivialVals;

  GraphReduction ReduceTrivialValues(Node* N);

public:
  static constexpr
  const char* name() { return "cse"; }

  CSEReducer(GraphEditor::Interface* editor)
    : GraphEditor(editor),
      G(Editor->GetGraph()) {}

  GraphReduction Reduce(Node* N);
};
} // end namespace gross
#endif
