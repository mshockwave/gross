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

NODE_PROPERTIES(VirtDLXBinOps) {
  NodeProperties(Node* N)
    : NODE_PROP_BASE(VirtDLXBinOps, N) {}

  Node* LHS() const {
    if(NodePtr->getNumValueInput() > 0)
      return NodePtr->getValueInput(0);
    else
      return nullptr;
  }

  Node* RHS() const {
    if(NodePtr->getNumValueInput() > 1)
      return NodePtr->getValueInput(1);
    else
      return nullptr;
  }
  Node* ImmRHS() const {
    if(auto* V = RHS()) {
      if(V->getOp() == IrOpcode::ConstantInt)
        return V;
      else
        return nullptr;
    } else {
      return nullptr;
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

template<>
struct NodeBuilder<IrOpcode::DLXLdW>
  : public _internal::MemNodeBuilder<IrOpcode::DLXLdW> {
  NodeBuilder(Graph* graph)
    : _internal::MemNodeBuilder<IrOpcode::DLXLdW>(graph) {}

  Node* Build() {
    assert(OffsetNode->getOp() == IrOpcode::ConstantInt &&
           "Offset not constant?");
    auto* N = new Node(IrOpcode::DLXLdW,
                       {BaseAddrNode, OffsetNode});
    BaseAddrNode->Users.push_back(N);
    OffsetNode->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }
};
template<>
struct NodeBuilder<IrOpcode::DLXLdX>
  : public _internal::MemNodeBuilder<IrOpcode::DLXLdX> {
  NodeBuilder(Graph* graph)
    : _internal::MemNodeBuilder<IrOpcode::DLXLdX>(graph) {}

  Node* Build() {
    auto* N = new Node(IrOpcode::DLXLdX,
                       {BaseAddrNode, OffsetNode});
    BaseAddrNode->Users.push_back(N);
    OffsetNode->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }
};

template<>
struct NodeBuilder<IrOpcode::DLXStW>
  : public _internal::MemNodeBuilder<IrOpcode::DLXStW> {
  NodeBuilder(Graph* graph)
    : _internal::MemNodeBuilder<IrOpcode::DLXStW>(graph) {}

  NodeBuilder& Src(Node* N) {
    SrcNode = N;
    return *this;
  }

  Node* Build() {
    assert(OffsetNode->getOp() == IrOpcode::ConstantInt &&
           "Offset not constant?");
    auto* N = new Node(IrOpcode::DLXStW,
                       {BaseAddrNode, OffsetNode,
                        SrcNode});
    BaseAddrNode->Users.push_back(N);
    OffsetNode->Users.push_back(N);
    SrcNode->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Node* SrcNode;
};
template<>
struct NodeBuilder<IrOpcode::DLXStX>
  : public _internal::MemNodeBuilder<IrOpcode::DLXStX> {
  NodeBuilder(Graph* graph)
    : _internal::MemNodeBuilder<IrOpcode::DLXStX>(graph) {}

  NodeBuilder& Src(Node* N) {
    SrcNode = N;
    return *this;
  }

  Node* Build() {
    auto* N = new Node(IrOpcode::DLXStX,
                       {BaseAddrNode, OffsetNode,
                        SrcNode});
    BaseAddrNode->Users.push_back(N);
    OffsetNode->Users.push_back(N);
    SrcNode->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Node* SrcNode;
};
} // end namespace gross
#endif
