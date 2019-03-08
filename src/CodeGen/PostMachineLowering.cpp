#include "gross/CodeGen/PostMachineLowering.h"
#include "gross/Graph/NodeUtils.h"
#include "DLXNodeUtils.h"

using namespace gross;

PostMachineLowering::PostMachineLowering(GraphSchedule& schedule)
  : Schedule(schedule),
    G(Schedule.getGraph()) {}

static void ChangePredBlock(BasicBlock* BB,
                            BasicBlock* FromBB, BasicBlock* ToBB) {
  if(BB->RemovePredBlock(FromBB) &&
     !ToBB->HasSuccBlock(BB)) {
    FromBB->RemoveSuccBlock(BB);
    ToBB->AddSuccBlock(BB);
  }
}

void PostMachineLowering::BranchesLowering() {
  auto mapRelOp = [](IrOpcode::ID OC) -> IrOpcode::ID {
    switch(OC) {
    default: gross_unreachable("Unsupported OC");
    case IrOpcode::BinLt: return IrOpcode::DLXBge;
    case IrOpcode::BinLe: return IrOpcode::DLXBgt;
    case IrOpcode::BinGt: return IrOpcode::DLXBle;
    case IrOpcode::BinGe: return IrOpcode::DLXBlt;
    case IrOpcode::BinEq: return IrOpcode::DLXBne;
    case IrOpcode::BinNe: return IrOpcode::DLXBeq;
    }
  };

  // {old node, new node or null, which means remove old node}
  std::vector<std::pair<Node*, Node*>> Staging;
  for(auto* BB : Schedule.rpo_blocks()) {
    for(auto* N : BB->nodes()) {
      if(N->getOp() != IrOpcode::If) continue;
      // retreive branch targets
      NodeProperties<IrOpcode::If> BNP(N);
      auto* FalseBB = Schedule.MapBlock(BNP.FalseBranch());
      assert(FalseBB);
      auto* FalseOffset = Schedule.MapBlockOffset(FalseBB);

      assert(N->getNumValueInput() == 1);
      auto* Predicate = N->getValueInput(0);
      Staging.push_back({Predicate, nullptr});
      if(Predicate->getOp() == IrOpcode::ConstantInt) {
        // TODO: optimize control flow
      } else {
        NodeProperties<IrOpcode::VirtBinOps> NP(Predicate);
        assert(NP);
        auto NewOC = mapRelOp(Predicate->getOp());
        auto* NewBr = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, NewOC)
                      .LHS(NP.LHS()).RHS(FalseOffset)
                      .Build();
        Staging.push_back({N, NewBr});
      }
    }
  }
}

void PostMachineLowering::FunctionCallLowering() {
}

void PostMachineLowering::Trimming() {
}

void PostMachineLowering::Run() {
  // lowering branches
  BranchesLowering();

  // lowering function calls
  FunctionCallLowering();

  // remove redundant control nodes
  Trimming();
}
