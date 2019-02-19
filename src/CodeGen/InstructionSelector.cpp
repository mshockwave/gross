#include "InstructionSelector.h"
#include "DLXNodeUtils.h"

using namespace gross;

InstructionSelector::InstructionSelector(GraphEditor::Interface* editor)
  : GraphEditor(editor) {}

GraphReduction InstructionSelector::Reduce(Node* N) {
  return NoChange();
}
