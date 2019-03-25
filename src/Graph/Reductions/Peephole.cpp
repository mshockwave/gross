#include "gross/Graph/Reductions/Peephole.h"
#include "gross/Graph/NodeUtils.h"

using namespace gross;

PeepholeReducer::PeepholeReducer(GraphEditor::Interface* editor)
  : GraphEditor(editor),
    G(Editor->GetGraph()),
    DeadNode(NodeBuilder<IrOpcode::Dead>(&G).Build()) {}

GraphReduction PeepholeReducer::ReduceArithmetic(Node* N) {
  NodeProperties<IrOpcode::VirtBinOps> NP(N);
  // constant reductions
  if(NP.LHS()->getOp() == IrOpcode::ConstantInt &&
     NP.RHS()->getOp() == IrOpcode::ConstantInt) {
    NodeProperties<IrOpcode::ConstantInt> LNP(NP.LHS()),
                                          RNP(NP.RHS());
    switch(N->getOp()) {
    case IrOpcode::BinAdd: {
      auto LHSVal = LNP.as<int32_t>(G),
           RHSVal = RNP.as<int32_t>(G);
      auto* NewNode
        = NodeBuilder<IrOpcode::ConstantInt>(&G, LHSVal + RHSVal).Build();
      return Replace(NewNode);
    }
    case IrOpcode::BinSub: {
      auto LHSVal = LNP.as<int32_t>(G),
           RHSVal = RNP.as<int32_t>(G);
      auto* NewNode
        = NodeBuilder<IrOpcode::ConstantInt>(&G, LHSVal - RHSVal).Build();
      return Replace(NewNode);
    }
    case IrOpcode::BinMul: {
      auto LHSVal = LNP.as<int32_t>(G),
           RHSVal = RNP.as<int32_t>(G);
      auto* NewNode
        = NodeBuilder<IrOpcode::ConstantInt>(&G, LHSVal * RHSVal).Build();
      return Replace(NewNode);
    }
    case IrOpcode::BinDiv: {
      // integer divistion
      auto LHSVal = LNP.as<int32_t>(G),
           RHSVal = RNP.as<int32_t>(G);
      auto* NewNode
        = NodeBuilder<IrOpcode::ConstantInt>(&G, LHSVal / RHSVal).Build();
      return Replace(NewNode);
    }
    default:
      return NoChange();
    }
  }
  return NoChange();
}

GraphReduction PeepholeReducer::ReduceRelation(Node* N) {
  NodeProperties<IrOpcode::VirtBinOps> NP(N);
  assert(NP);
  // constant reduction
  if(NP.LHS()->getOp() == IrOpcode::ConstantInt &&
     NP.RHS()->getOp() == IrOpcode::ConstantInt) {
    NodeProperties<IrOpcode::ConstantInt> LNP(NP.LHS()),
                                          RNP(NP.RHS());
#define REL_CASE(OC, Op)  \
  case IrOpcode::OC: {  \
    auto LHSVal = LNP.as<int32_t>(G), \
         RHSVal = RNP.as<int32_t>(G); \
    auto* NewNode \
      = NodeBuilder<IrOpcode::ConstantInt>(&G,  \
                                           LHSVal Op RHSVal? 1 : 0) \
        .Build(); \
    return Replace(NewNode);  \
  }
    switch(N->getOp()) {
    REL_CASE(BinLe, <=)
    REL_CASE(BinLt, <)
    REL_CASE(BinGe, >=)
    REL_CASE(BinGt, >)
    REL_CASE(BinNe, !=)
    REL_CASE(BinEq, ==)
    default:
      return NoChange();
    }
#undef REL_CASE
  } else if(NP.RHS()->getOp() != IrOpcode::ConstantInt ||
            NodeProperties<IrOpcode::ConstantInt>(NP.RHS())
            .as<int32_t>(G) != 0) {
    // always put zero at RHS
    auto* OldLHS = NP.LHS();
    auto* OldRHS = NP.RHS();
    auto* NewLHS = NodeBuilder<IrOpcode::BinSub>(&G)
                   .LHS(OldLHS).RHS(OldRHS)
                   .Build();
    auto* Zero = NodeBuilder<IrOpcode::ConstantInt>(&G, 0)
                 .Build();
    N->ReplaceUseOfWith(OldLHS, NewLHS, Use::K_VALUE);
    N->ReplaceUseOfWith(OldRHS, Zero, Use::K_VALUE);
    return Replace(N);
  }
  return NoChange();
}

GraphReduction PeepholeReducer::DeadPHIElimination(Node* N) {
  using input_iterators = llvm::iterator_range<typename Node::input_iterator>;
  auto TryMerge = [](input_iterators const& Inputs) -> Node* {
    Node* Val = nullptr;
    for(auto* NI : Inputs) {
      if(!Val) Val = NI;
      else if(Val != NI) return nullptr;
    }
    return Val;
  };

  if(N->input_size() > 1) {
    auto* MergeVal = TryMerge(N->value_inputs());
    if(MergeVal) {
      N->ReplaceWith(MergeVal, Use::K_VALUE);
      for(auto* U : N->value_users())
        Revisit(U);
    }

    auto* MergeEffect = TryMerge(N->effect_inputs());
    if(MergeEffect) {
      N->ReplaceWith(MergeEffect, Use::K_EFFECT);
      for(auto* U : N->effect_users())
        Revisit(U);
    }

    auto* MergeControl = TryMerge(N->control_inputs());
    if(MergeControl) {
      N->ReplaceWith(MergeControl, Use::K_CONTROL);
      for(auto* U : N->control_users())
        Revisit(U);
    }

    if(MergeVal && MergeEffect && MergeControl)
      return Replace(DeadNode);
  }
  return NoChange();
}

GraphReduction PeepholeReducer::Reduce(Node* N) {
  switch(N->getOp()) {
  case IrOpcode::BinAdd:
  case IrOpcode::BinSub:
  case IrOpcode::BinMul:
  case IrOpcode::BinDiv:
    return ReduceArithmetic(N);
  case IrOpcode::BinLe:
  case IrOpcode::BinLt:
  case IrOpcode::BinGe:
  case IrOpcode::BinGt:
  case IrOpcode::BinEq:
  case IrOpcode::BinNe:
    return ReduceRelation(N);
  case IrOpcode::Phi:
    return DeadPHIElimination(N);
  default:
    return NoChange();
  }
}
