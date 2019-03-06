#include "gross/Graph/Reductions/MemoryLegalize.h"
#include "gross/Graph/NodeUtils.h"
#include <iterator>
#include <vector>

using namespace gross;

MemoryLegalize::MemoryLegalize(GraphEditor::Interface* editor)
  : GraphEditor(editor),
    G(Editor->GetGraph()){}

GraphReduction MemoryLegalize::ReduceMemStore(Node* N) {
  std::vector<Node*> PHINodes;
  for(auto* EU : N->effect_users()) {
    if(EU->getOp() == IrOpcode::Phi) PHINodes.push_back(EU);
  }

  if(!PHINodes.empty()) {
    std::vector<Node*> MemLoads;
    for(auto* EU : N->effect_users()) {
      if(EU->getOp() == IrOpcode::MemLoad)
        MemLoads.push_back(EU);
    }
    if(MemLoads.empty()) return NoChange();
    Node* NewEffect = nullptr;
    if(MemLoads.size() == 1) {
      NewEffect = MemLoads.front();
    } else {
      NodeBuilder<IrOpcode::EffectMerge> Builder(&G);
      for(auto* Load : MemLoads)
        Builder.AddEffectInput(Load);
      NewEffect = Builder.Build();
    }

    for(auto* PHI : PHINodes) {
      PHI->ReplaceUseOfWith(N, NewEffect, Use::K_EFFECT);
      Revisit(PHI);
    }
  }
  return NoChange();
}

GraphReduction MemoryLegalize::Reduce(Node* N) {
  if(N->getOp() == IrOpcode::MemStore)
    return ReduceMemStore(N);

  return NoChange();
}
