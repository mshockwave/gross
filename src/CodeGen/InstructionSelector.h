#ifndef GROSS_CODEGEN_INSTRUCTIONSELECTOR_H
#define GROSS_CODEGEN_INSTRUCTIONSELECTOR_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
class InstructionSelector : public GraphEditor {
public:
  InstructionSelector(GraphEditor::Interface* editor);

  static constexpr
  const char* name() { return "isel"; }

  GraphReduction Reduce(Node* N);
};
} // end namespace gross
#endif
