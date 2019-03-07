#ifndef GROSS_CODEGEN_PREMACHINELOWERING_H
#define GROSS_CODEGEN_PREMACHINELOWERING_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
class PreMachineLowering : public GraphEditor {
  Graph& G;

  static Node* PropagateEffects(Node* Old, Node* New);

  GraphReduction SelectArithmetic(Node* N);
  GraphReduction SelectMemOperations(Node* N);

public:
  PreMachineLowering(GraphEditor::Interface* editor);

  static constexpr
  const char* name() { return "pre-machine-lowering"; }

  GraphReduction Reduce(Node* N);
};
} // end namespace gross
#endif
