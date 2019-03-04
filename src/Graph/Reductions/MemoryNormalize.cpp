#include "gross/Graph/Reductions/MemoryNormalize.h"
#include <set>

using namespace gross;

MemoryNormalize::MemoryNormalize(GraphEditor::Interface* editor)
  : GraphEditor(editor) {}

static bool IsMutableMemOps(Node* N) {
  if(!N) return false;
  return N->getOp() == IrOpcode::MemStore ||
         N->getOp() == IrOpcode::Phi;
}

GraphReduction MemoryNormalize::ReduceMutableMemOps(Node* N) {
  std::set<Node*> Loads;
  std::set<Node*> StagingInputs;
  for(auto* EI : N->effect_inputs()) {
    if(!IsMutableMemOps(EI)) continue;
    for(auto* EU : EI->effect_users()) {
      if(EU->getOp() == IrOpcode::MemLoad) Loads.insert(EU);
    }
    if(!Loads.empty()) {
      // remove original mutable memory ops inputs
      // and effect depends on Loads
      StagingInputs.insert(EI);
      for(auto* Load : Loads) {
        N->appendEffectInput(Load);
      }
      Loads.clear();
    }
  }
  if(StagingInputs.empty()) return NoChange();

  for(auto* EI : StagingInputs) {
    N->removeEffectInputAll(EI);
  }
  return Replace(N);
}

GraphReduction MemoryNormalize::Reduce(Node* N) {
  if(IsMutableMemOps(N))
    return ReduceMutableMemOps(N);

  return NoChange();
}
