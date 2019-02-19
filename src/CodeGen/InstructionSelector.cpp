#include "gross/CodeGen/InstructionSelector.h"
#include "gross/Graph/NodeUtils.h"
#include "DLXNodeUtils.h"
#include <cmath>

using namespace gross;

InstructionSelector::InstructionSelector(GraphEditor::Interface* editor)
  : GraphEditor(editor),
    G(Editor->GetGraph()){}

GraphReduction InstructionSelector::SelectArithmetic(Node* N) {
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
    double Exp = std::exp2(RHSInt);
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

GraphReduction InstructionSelector::Reduce(Node* N) {
  switch(N->getOp()) {
  default:
    return NoChange();
  case IrOpcode::BinAdd:
  case IrOpcode::BinSub:
  case IrOpcode::BinMul:
  case IrOpcode::BinDiv:
    return SelectArithmetic(N);
  }
}
