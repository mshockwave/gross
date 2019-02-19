#ifndef GROSS_CODEGEN_DLXNODEUTILS_H
#define GROSS_CODEGEN_DLXNODEUTILS_H
#include "gross/Graph/NodeUtilsBase.h"

namespace gross {
NODE_PROPERTIES(VirtDLXOps) {
  NodeProperties(Node* N)
    : NODE_PROP_BASE(VirtDLXOps, N) {}

  operator bool() const {
    if(!NodePtr) return false;
    switch(NodePtr->getOp()) {
#define DLX_ARITH_OP(OC)  \
    case IrOpcode::DLX##OC:  \
    case IrOpcode::DLX##OC##I:
#define DLX_COMMON(OC)  \
    case IrOpcode::DLX##OC:
#include "gross/Graph/DLXOpcodes.def"
      return true;
    default:
      return false;
    }
  }
};

template<>
struct NodeBuilder<IrOpcode::VirtDLXBinOps> {
  NodeBuilder(Graph* graph, IrOpcode::ID Op, bool Imm = false)
    : G(graph), OC(Op), IsImmediate(Imm),
      LHSVal(nullptr), RHSVal(nullptr) {}

  NodeBuilder& LHS(Node* N) {
    if(IsImmediate)
      assert(N->getOp() != IrOpcode::ConstantInt);
    LHSVal = N;
    return *this;
  }

  NodeBuilder& RHS(Node* N) {
    if(IsImmediate)
      assert(N->getOp() == IrOpcode::ConstantInt);
    RHSVal = N;
    return *this;
  }

  Node* Build() {
    assert(LHSVal && RHSVal);
    auto* N = new Node(OC, {LHSVal, RHSVal});
    LHSVal->Users.push_back(N);
    RHSVal->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  IrOpcode::ID OC;
  bool IsImmediate;
  Node *LHSVal, *RHSVal;
};
} // end namespace gross
#endif
