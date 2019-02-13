#include "gross/Graph/Reductions/Peephole.h"
#include "gross/Graph/NodeUtils.h"

using namespace gross;
using namespace gross::graph_reduction;

PeepholeReducer::PeepholeReducer(Graph& graph) : G(graph), DeadNode(nullptr) {
  DeadNode = NodeBuilder<IrOpcode::Dead>(&G).Build();
}

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
      N->ReplaceWith(NewNode, Use::K_VALUE);
      N->Kill(DeadNode);
      return Replace(NewNode);
    }
    case IrOpcode::BinSub: {
      auto LHSVal = LNP.as<int32_t>(G),
           RHSVal = RNP.as<int32_t>(G);
      // only handle non-negative result
      if(LHSVal < RHSVal) return NoChange();
      auto* NewNode
        = NodeBuilder<IrOpcode::ConstantInt>(&G, LHSVal - RHSVal).Build();
      N->ReplaceWith(NewNode, Use::K_VALUE);
      N->Kill(DeadNode);
      return Replace(NewNode);
    }
    case IrOpcode::BinMul: {
      auto LHSVal = LNP.as<int32_t>(G),
           RHSVal = RNP.as<int32_t>(G);
      auto* NewNode
        = NodeBuilder<IrOpcode::ConstantInt>(&G, LHSVal * RHSVal).Build();
      N->ReplaceWith(NewNode, Use::K_VALUE);
      N->Kill(DeadNode);
      return Replace(NewNode);
    }
    // do not handle div for now
    default:
      return NoChange();
    }
  }
  return NoChange();
}

GraphReduction PeepholeReducer::ReduceRelation(Node* N) {
  NodeProperties<IrOpcode::VirtBinOps> NP(N);
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
    N->ReplaceWith(NewNode, Use::K_VALUE);  \
    N->Kill(DeadNode);  \
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
  default:
    return NoChange();
  }
}
