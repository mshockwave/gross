#ifndef GROSS_CODEGEN_DLXNODEUTILS_H
#define GROSS_CODEGEN_DLXNODEUTILS_H
#include "gross/Graph/NodeUtilsBase.h"
#include <array>
#include <vector>

namespace gross {
// Forward declarations
class GraphSchedule;

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
#define DLX_CONST(OC) DLX_COMMON(OC)
#include "gross/Graph/DLXOpcodes.def"
      return true;
    default:
      return false;
    }
  }
};

NODE_PROPERTIES(VirtDLXRegisters) {
  NodeProperties(Node* N)
    : NODE_PROP_BASE(VirtDLXRegisters, N) {}

  operator bool() const {
    if(!NodePtr) return false;
    auto OC = NodePtr->getOp();
    return OC >= IrOpcode::DLXr0 &&
           OC <= IrOpcode::DLXr31;
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

NODE_PROPERTIES(VirtDLXTerminate) {
  NodeProperties(Node* N)
    : NODE_PROP_BASE(VirtDLXTerminate, N) {}
  operator bool() const {
    if(!NodePtr) return false;
    switch(NodePtr->getOp()) {
#define DLX_CTRL_OP(OC) \
    case IrOpcode::DLX##OC:
#include "gross/Graph/DLXOpcodes.def"
      return true;
    default:
      return false;
    }
  }
};

NODE_PROPERTIES(VirtDLXCallsiteBegin) {
  NodeProperties(Node* N)
    : NODE_PROP_BASE(VirtDLXCallsiteBegin, N) {}

  // the first effect_user
  Node* getCallsiteEnd() const {
    assert(NodePtr->effect_users().begin() !=
           NodePtr->effect_users().end());
    return *NodePtr->effect_users().begin();
  }

  using param_iterator = typename Node::effect_user_iterator;
  param_iterator param_begin() {
    return std::next(NodePtr->effect_users().begin(), 1);
  }
  param_iterator param_end() {
    return NodePtr->effect_users().end();
  }
  llvm::iterator_range<param_iterator> params() {
    return llvm::make_range(param_begin(), param_end());
  }
  size_t param_size() {
    return std::distance(param_begin(), param_end());
  }
};

NODE_PROPERTIES(VirtDLXCallsiteEnd) {
  NodeProperties(Node* N)
    : NODE_PROP_BASE(VirtDLXCallsiteEnd, N) {}

  Node* getCallsiteBegin() const {
    assert(NodePtr->getNumEffectInput() > 0);
    return NodePtr->getEffectInput(0);
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

template<>
struct NodeBuilder<IrOpcode::VirtDLXTriOps> {
  NodeBuilder(Graph* graph, IrOpcode::ID Op)
    : G(graph), OC(Op) {
    Vals.fill(nullptr);
  }

  template<size_t Idx>
  NodeBuilder& SetVal(Node* N) {
    static_assert(Idx < 3, "index out of bound");
    Vals[Idx] = N;
    return *this;
  }

  Node* Build() {
    auto* N = new Node(OC,
                       {Vals[0], Vals[1], Vals[2]});
    for(auto* V: Vals)
      V->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  IrOpcode::ID OC;
  std::array<Node*, 3> Vals;
};

template<>
struct NodeBuilder<IrOpcode::VirtDLXCallsiteEnd> {
  NodeBuilder(Graph* graph, Node* Callsite)
    : G(graph),
      CallsiteBegin(Callsite) {}

  Node* Build() {
    assert(CallsiteBegin);
    auto* N = new Node(IrOpcode::VirtDLXCallsiteEnd,
                       {}, {},
                       {CallsiteBegin});
    CallsiteBegin->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  Node* CallsiteBegin;
};

template<>
struct NodeBuilder<IrOpcode::VirtDLXPassParam> {
  NodeBuilder(Graph* graph, Node* Val)
    : G(graph),
      ParamVal(Val) {}

  NodeBuilder& SetCallsite(Node* N) {
    CallsiteBegin = N;
    return *this;
  }

  Node* Build() {
    assert(ParamVal && CallsiteBegin);
    auto* N = new Node(IrOpcode::VirtDLXPassParam,
                       {ParamVal}, {},
                       {CallsiteBegin});
    ParamVal->Users.push_back(N);
    CallsiteBegin->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  Node* CallsiteBegin;
  Node* ParamVal;
};

template<>
struct NodeBuilder<IrOpcode::DLXRet> {
  NodeBuilder(Graph* graph, Node* LR)
    : G(graph),
      LinkReg(LR) {}

  Node* Build() {
    assert(LinkReg);
    auto* N = new Node(IrOpcode::DLXRet,
                       {LinkReg});
    LinkReg->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  Node* LinkReg;
};

struct StackUtils {
  static constexpr IrOpcode::ID SpReg = IrOpcode::DLXr29;
  static constexpr IrOpcode::ID FpReg = IrOpcode::DLXr28;

  StackUtils(GraphSchedule& schedule);

  Node* FramePointer() const { return Fp; }
  Node* StackPointer() const { return Sp; }

  void ReserveSlots(size_t Num, std::vector<Node*>& Result);

  // reserve multiple slots without
  // zero initialized each slot
  Node* ReserveSlots(size_t Num, Node* Val = nullptr);

  Node* RestoreSlot(Node* Dest);

  // slot beyond local var slots
  Node* NonLocalSlotOffset(size_t Idx);

  // spilled parameter slots
  Node* SpilledParamOffset(size_t Idx);

private:
  GraphSchedule& Schedule;
  Graph& G;
  Node *Fp, *Sp;
};
} // end namespace gross
#endif
