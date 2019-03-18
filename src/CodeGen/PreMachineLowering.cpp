#include "gross/Graph/NodeUtils.h"
#include "DLXNodeUtils.h"
#include "PreMachineLowering.h"
#include <cmath>

using namespace gross;

PreMachineLowering::PreMachineLowering(GraphEditor::Interface* editor)
  : GraphEditor(editor),
    G(Editor->GetGraph()){}

// propagate all the effect/control inputs
Node* PreMachineLowering::PropagateEffects(Node* OldNode, Node* NewNode) {
  for(auto* EI : OldNode->effect_inputs())
    NewNode->appendEffectInput(EI);
  for(auto* CI : OldNode->control_inputs())
    NewNode->appendControlInput(CI);
  return NewNode;
}

GraphReduction PreMachineLowering::SelectArithmetic(Node* N) {
  NodeProperties<IrOpcode::VirtBinOps> NP(N);
  auto* LHSVal = NP.LHS();
  auto* RHSVal = NP.RHS();
  auto ToDLXOp = [](IrOpcode::ID OC, bool IsImm) -> IrOpcode::ID {
    switch(OC) {
    default:
      return IrOpcode::None;
    case IrOpcode::BinAdd:
      return IsImm? IrOpcode::DLXAddI : IrOpcode::DLXAdd;
    case IrOpcode::BinSub:
      return IsImm? IrOpcode::DLXSubI : IrOpcode::DLXSub;
    case IrOpcode::BinMul:
      return IsImm? IrOpcode::DLXMulI : IrOpcode::DLXMul;
    case IrOpcode::BinDiv:
      return IsImm? IrOpcode::DLXDivI : IrOpcode::DLXDiv;
    }
  };

  if(LHSVal->getOp() == IrOpcode::ConstantInt ||
     RHSVal->getOp() == IrOpcode::ConstantInt) {
    // use immediate family arithmetic instructions
    // or more advance reductions
    assert(!(LHSVal->getOp() == IrOpcode::ConstantInt &&
             RHSVal->getOp() == IrOpcode::ConstantInt) &&
           "Didn't run Peephole?");
    if(LHSVal->getOp() == IrOpcode::ConstantInt) {
      // switch operand order
      auto* tmp = LHSVal;
      LHSVal = RHSVal;
      RHSVal = tmp;
    }

    auto RHSInt = NodeProperties<IrOpcode::ConstantInt>(RHSVal)
                  .as<int32_t>(G);
    double Exp = std::log2(RHSInt);
    double FloatIntExp;
    if(N->getOp() == IrOpcode::BinMul &&
       std::modf(Exp, &FloatIntExp) == 0.0L) {
      // replace with left shift as RHS is power of two
      auto IntExp = static_cast<int32_t>(FloatIntExp);
      auto* NewNode
        = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXLshI, true)
          .LHS(LHSVal)
          .RHS(NodeBuilder<IrOpcode::ConstantInt>(&G, IntExp).Build())
          .Build();
      return Replace(NewNode);
    }

    auto NewOC = ToDLXOp(N->getOp(), true);
    assert(NewOC != IrOpcode::None);
    auto* NewNode = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, NewOC, true)
                    .LHS(LHSVal).RHS(RHSVal)
                    .Build();
    return Replace(NewNode);
  } else {
    auto NewOC = ToDLXOp(N->getOp(), false);
    assert(NewOC != IrOpcode::None);
    auto* NewNode = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, NewOC, false)
                    .LHS(LHSVal).RHS(RHSVal)
                    .Build();
    return Replace(NewNode);
  }
}

GraphReduction PreMachineLowering::SelectMemOperations(Node* N) {
  NodeProperties<IrOpcode::VirtMemOps> NP(N);
  assert(NP);
  auto* BaseAddr = NP.BaseAddr();
  auto* Offset = NP.Offset();

  if(N->getOp() == IrOpcode::MemLoad) {
    Node* NewNode;
    if(Offset->getOp() == IrOpcode::ConstantInt) {
      NewNode = NodeBuilder<IrOpcode::DLXLdW>(&G)
                .BaseAddr(BaseAddr).Offset(Offset)
                .Build();
    } else {
      NewNode = NodeBuilder<IrOpcode::DLXLdX>(&G)
                .BaseAddr(BaseAddr).Offset(Offset)
                .Build();
    }
    return Replace(PropagateEffects(N, NewNode));
  } else if(N->getOp() == IrOpcode::MemStore) {
    NodeProperties<IrOpcode::MemStore> StNP(N);
    Node* NewNode;
    if(Offset->getOp() == IrOpcode::ConstantInt) {
      NewNode = NodeBuilder<IrOpcode::DLXStW>(&G)
                .BaseAddr(BaseAddr).Offset(Offset)
                .Src(StNP.SrcVal())
                .Build();
    } else {
      NewNode = NodeBuilder<IrOpcode::DLXStX>(&G)
                .BaseAddr(BaseAddr).Offset(Offset)
                .Src(StNP.SrcVal())
                .Build();
    }
    return Replace(PropagateEffects(N, NewNode));
  } else {
    gross_unreachable("Unsupported Opcode");
    return NoChange();
  }
}

GraphReduction PreMachineLowering::Reduce(Node* N) {
  switch(N->getOp()) {
  default:
    return NoChange();
  case IrOpcode::BinAdd:
  case IrOpcode::BinSub:
  case IrOpcode::BinMul:
  case IrOpcode::BinDiv:
    return SelectArithmetic(N);
  case IrOpcode::MemLoad:
  case IrOpcode::MemStore:
    return SelectMemOperations(N);
  }
}
