#include "gross/CodeGen/PostMachineLowering.h"

using namespace gross;

PostMachineLowering::PostMachineLowering(GraphEditor::Interface* editor)
  : GraphEditor(editor),
    G(Editor->GetGraph()) {}

GraphReduction PostMachineLowering::Reduce(Node* N) {
  return NoChange();
}
