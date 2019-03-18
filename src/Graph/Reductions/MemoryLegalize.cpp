#include "gross/Graph/Reductions/MemoryLegalize.h"
#include "gross/Graph/NodeUtils.h"
#include <iterator>
#include <set>
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

/// ===== DLXMemoryLegalize =====
Node* DLXMemoryLegalize::MergeAllocas(const std::set<Node*>& Allocas) {
  auto* TotalSize = NodeBuilder<IrOpcode::ConstantInt>(&G, 0).Build();
  // alloca -> offset
  std::unordered_map<Node*, Node*> Offsets;
  for(auto* Alloca : Allocas) {
    assert(Alloca->getNumValueInput() > 0);
    auto* Size = Alloca->getValueInput(0);
    Offsets[Alloca] = TotalSize;
    TotalSize = NodeBuilder<IrOpcode::BinAdd>(&G)
                .LHS(TotalSize).RHS(Size)
                .Build();
  }

  if(Offsets.empty()) return nullptr;
  auto* MergedAlloca = NodeBuilder<IrOpcode::Alloca>(&G)
                       .Size(TotalSize).Build();

  std::set<Node*> MemUsrs;
  for(auto& Pair : Offsets) {
    auto* Alloca = Pair.first;
    auto* Offset = Pair.second;
    // transform to top-down addressing
    Offset = NodeBuilder<IrOpcode::BinSub>(&G)
             .LHS(Offset).RHS(TotalSize)
             .Build();

    // do not append offset on the base address of
    // memory operations
    MemUsrs.clear();
    for(auto* N : Alloca->value_users()) {
      if(NodeProperties<IrOpcode::VirtMemOps>(N))
        MemUsrs.insert(N);
    }
    for(auto* Mem : MemUsrs) {
      NodeProperties<IrOpcode::VirtMemOps> MNP(Mem);
      auto* OrigOffset = MNP.Offset();
      auto* NewOffset = NodeBuilder<IrOpcode::BinAdd>(&G)
                        .LHS(OrigOffset).RHS(Offset)
                        .Build();
      Mem->ReplaceUseOfWith(OrigOffset, NewOffset, Use::K_VALUE);
      Mem->ReplaceUseOfWith(MNP.BaseAddr(), MergedAlloca, Use::K_VALUE);
    }
  }
  return MergedAlloca;
}

void DLXMemoryLegalize::RunOnFunction(SubGraph& SG) {
  std::set<Node*> LocalAllocas;
  for(auto* N : SG.nodes()) {
    if(N->getOp() == IrOpcode::Alloca &&
       !G.IsGlobalVar(N))
      LocalAllocas.insert(N);
  }
  (void) MergeAllocas(LocalAllocas);
}

void DLXMemoryLegalize::Run() {
  // local allocas
  for(auto& SG : G.subregions()) {
    RunOnFunction(SG);
  }

  // global allocas
  std::set<Node*> GlobalAllocas(G.global_var_begin(),
                                G.global_var_end());
  auto* NewGV = MergeAllocas(GlobalAllocas);
  assert(GlobalAllocas.empty() || NewGV);
  for(auto* GA: GlobalAllocas) {
    G.ReplaceGlobalVar(GA, NewGV);
  }
}
