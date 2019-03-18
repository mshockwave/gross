#include "gross/Graph/NodeUtils.h"
#include "DLXNodeUtils.h"
#include "PostMachineLowering.h"

using namespace gross;

PostMachineLowering::PostMachineLowering(GraphSchedule& schedule)
  : Schedule(schedule),
    G(Schedule.getGraph()) {}

void PostMachineLowering::ControlFlowLowering() {
  auto mapRelOp = [](IrOpcode::ID OC) -> IrOpcode::ID {
    switch(OC) {
    default: gross_unreachable("Unsupported OC");
    case IrOpcode::BinLt: return IrOpcode::DLXBlt;
    case IrOpcode::BinLe: return IrOpcode::DLXBle;
    case IrOpcode::BinGt: return IrOpcode::DLXBgt;
    case IrOpcode::BinGe: return IrOpcode::DLXBge;
    case IrOpcode::BinEq: return IrOpcode::DLXBeq;
    case IrOpcode::BinNe: return IrOpcode::DLXBne;
    case IrOpcode::DLXBlt: return IrOpcode::DLXBge;
    case IrOpcode::DLXBle: return IrOpcode::DLXBgt;
    case IrOpcode::DLXBgt: return IrOpcode::DLXBle;
    case IrOpcode::DLXBge: return IrOpcode::DLXBlt;
    case IrOpcode::DLXBeq: return IrOpcode::DLXBne;
    case IrOpcode::DLXBne: return IrOpcode::DLXBeq;
    }
  };

  // {old node, new node or null, which means remove old node}
  std::vector<std::pair<Node*, Node*>> Staging;
  for(auto* BB : Schedule.rpo_blocks()) {
    auto BBRPOIdx = BB->getRPOIndex();
    for(auto* N : BB->nodes()) {
      if(N->getOp() != IrOpcode::If) continue;
      auto* Zero = NodeBuilder<IrOpcode::ConstantInt>(&G, 0)
                   .Build();
      // retreive branch targets
      NodeProperties<IrOpcode::If> BNP(N);
      auto* FalseBB = Schedule.MapBlock(BNP.FalseBranch());
      auto* TrueBB = Schedule.MapBlock(BNP.TrueBranch());
      assert(TrueBB && FalseBB);
      auto* TargetBB = (TrueBB->getRPOIndex() == BBRPOIdx + 1)?
                        FalseBB :
                        (FalseBB->getRPOIndex() == BBRPOIdx + 1)?
                        TrueBB : nullptr;
      assert(TargetBB && "no adjacent block?");
      auto* TargetOffset = Schedule.MapBlockOffset(TargetBB);

      assert(N->getNumValueInput() == 1);
      auto* Predicate = N->getValueInput(0);
      Staging.push_back({Predicate, nullptr});
      if(Predicate->getOp() == IrOpcode::ConstantInt) {
        // FIXME: rarely happens, one should optimize control
        // flow in middle-end
        Predicate = NodeBuilder<IrOpcode::BinNe>(&G)
                    .LHS(Predicate).RHS(Zero)
                    .Build();
      }
      NodeProperties<IrOpcode::VirtBinOps> NP(Predicate);
      assert(NP);
      assert(NP.RHS() == Zero && "predicate RHS not zero?");
      auto NewOC = mapRelOp(Predicate->getOp());
      if(TargetBB == FalseBB) NewOC = mapRelOp(NewOC);
      auto* NewBr = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, NewOC)
                    .LHS(NP.LHS()).RHS(TargetOffset)
                    .Build();
      Staging.push_back({N, NewBr});
    }

    for(auto& P : Staging) {
      auto* OldNode = P.first;
      auto* NewNode = P.second;
      auto* OldBB = Schedule.MapBlock(OldNode);
      if(!OldBB) continue;
      if(!NewNode) {
        Schedule.RemoveNode(OldBB, OldNode);
      } else {
        Schedule.ReplaceNode(OldBB, OldNode, NewNode);
      }
    }
    Staging.clear();

    if(BB->succ_size() != 1) continue;
    BasicBlock* SuccBB = *BB->succ_begin();
    if(SuccBB->getRPOIndex() != BBRPOIdx + 1) {
      // insert unconditional jumps
      auto* Zero = NodeBuilder<IrOpcode::ConstantInt>(&G, 0)
                   .Build();
      auto* Offset = Schedule.MapBlockOffset(SuccBB);
      auto* Jmp = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXBeq)
                  .LHS(Zero).RHS(Offset)
                  .Build();
      Schedule.AddNode(BB, Jmp);
    }
  }
}

void PostMachineLowering::FunctionCallLowering() {
  // 1. Insert VirtDLXCallsiteBegin
  // 2. Insert VirtDLXPassParam for every actual parameter
  // =======Before Call==========
  // 3. Replace Call with BSR/JSR
  // ========After Call==========
  // 4. Insert ADDI #0, R1 and replace all usages of original
  //    function return value with that.
  // 5. Insert VirtDLXCallsiteEnd

  std::vector<Node*> Callsites;
  for(auto* BB : Schedule.rpo_blocks()) {
    for(auto* N : BB->nodes()) {
      if(N->getOp() == IrOpcode::Call)
        Callsites.push_back(N);
    }
  }

  for(auto* CS : Callsites) {
    auto* BB = Schedule.MapBlock(CS);
    NodeProperties<IrOpcode::Call> CNP(CS);
    // remove FunctionStub node if any
    Schedule.RemoveNode(BB, CNP.getFuncStub());

    // add VirtDLXCallsiteBegin and End
    auto* CallsiteBegin
      = NodeBuilder<IrOpcode::VirtDLXCallsiteBegin>(&G).Build();
    Schedule.AddNodeBefore(BB, CS, CallsiteBegin);
    auto* CallsiteEnd
      = NodeBuilder<IrOpcode::VirtDLXCallsiteEnd>(&G, CallsiteBegin)
        .Build();
    Schedule.AddNodeAfter(BB, CS, CallsiteEnd);

    // handle parameter passing
    for(auto* Param : CNP.params()) {
      auto* ParamPass
        = NodeBuilder<IrOpcode::VirtDLXPassParam>(&G, Param)
          .SetCallsite(CallsiteBegin)
          .Build();
      Schedule.AddNodeBefore(BB, CS, ParamPass);
    }

    // handle return value if any
    std::vector<Node*> ReturnUsrs;
    for(auto* VU : CS->value_users()) {
      ReturnUsrs.push_back(VU);
    }
    if(!ReturnUsrs.empty()) {
      auto* R1 = NodeBuilder<IrOpcode::DLXr1>(&G).Build();
      auto* NewReturn
        = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI, true)
          .LHS(R1)
          .RHS(NodeBuilder<IrOpcode::ConstantInt>(&G, 0).Build())
          .Build();
      for(auto* RU : ReturnUsrs) {
        RU->ReplaceUseOfWith(CS, NewReturn, Use::K_VALUE);
      }
      Schedule.AddNodeAfter(BB, CS, NewReturn);
    }

    // replace Call with BSR
    // TODO
  }
}

void PostMachineLowering::Trimming() {
  std::vector<Node*> Staging;
  for(auto* BB : Schedule.rpo_blocks()) {
    for(auto* N : BB->nodes()) {
      switch(N->getOp()) {
      case IrOpcode::If:
      case IrOpcode::Loop:
      case IrOpcode::IfTrue:
      case IrOpcode::IfFalse:
      case IrOpcode::Merge:
        Staging.push_back(N);
        break;
      // also remove EffectPHI
      case IrOpcode::Phi:
        if(!N->getNumValueInput() &&
           N->getNumEffectInput()) {
          Staging.push_back(N);
        }
        break;
      default: break;
      }
    }

    for(auto* N : Staging) {
      Schedule.RemoveNode(BB, N);
    }
    Staging.clear();
  }
}

void PostMachineLowering::Run() {
  // lowering branches and insert
  // BB jumps
  ControlFlowLowering();

  // lowering function calls
  FunctionCallLowering();

  // remove redundant control nodes
  Trimming();
}
