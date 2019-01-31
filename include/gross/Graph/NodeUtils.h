#ifndef GROSS_GRAPH_NODE_UTILS_H
#define GROSS_GRAPH_NODE_UTILS_H
#include "gross/Graph/Node.h"
#include "gross/Graph/Graph.h"
#include "gross/Support/type_traits.h"
#include <string>

namespace gross {
template<IrOpcode::ID Op>
class NodePropertiesBase {
protected:
  Node* NodePtr;

  NodePropertiesBase(Node* N)
    : NodePtr(N) {}

public:
  operator bool() const {
    return NodePtr && NodePtr->Op == Op;
  }
};

template<IrOpcode::ID Op>
struct NodeProperties : public NodePropertiesBase<Op> {
  NodeProperties(Node* N)
    : NodePropertiesBase<Op>(N) {}
};

#define NODE_PROPERTIES(OP) \
  template<>  \
  struct NodeProperties<IrOpcode::OP> : public NodePropertiesBase<IrOpcode::OP>
#define NODE_PROPERTIES_VIRT(OP, VIRTOP) \
  template<>  \
  struct NodeProperties<IrOpcode::OP> : public NodeProperties<IrOpcode::VIRTOP>

#define NODE_PROP_BASE(OP, NODE) \
  NodePropertiesBase<IrOpcode::OP>(NODE)
#define NODE_PROP_VIRT(VIRTOP, NODE)  \
  NodeProperties<IrOpcode::VIRTOP>(NODE)

NODE_PROPERTIES(ConstantInt) {
  NodeProperties(Node *N)
    : NODE_PROP_BASE(ConstantInt, N) {}

  template<typename T>
  T as(const Graph& G) const {
    if(!*this) return T();
    if(auto* V = G.ConstNumberPool.find_value(NodePtr))
      return static_cast<T>(*V);
    else
      return T();
  }
};
NODE_PROPERTIES(ConstantStr) {
  NodeProperties(Node *N)
    : NODE_PROP_BASE(ConstantStr, N) {}

  const std::string& str(const Graph& G) const {
    assert(*this && "Invalid Node");
    const auto* V = G.ConstStrPool.find_value(NodePtr);
    assert(V && "string not found");
    return *V;
  }
  const std::string str_val(const Graph& G) const {
    if(!*this) return "";
    if(const auto* V = G.ConstStrPool.find_value(NodePtr))
      return *V;
    else
      return "";
  }
};

NODE_PROPERTIES(VirtSrcDecl) {
  NodeProperties(Node *N)
    : NODE_PROP_BASE(VirtSrcDecl, N) {}

  operator bool() const {
    if(!NodePtr) return false;
    auto Op = NodePtr->getOp();
    return Op == IrOpcode::SrcVarDecl ||
           Op == IrOpcode::SrcArrayDecl;
  }

  Node* getSymbolName() const {
    assert(NodePtr->getNumValueInput() > 0);
    return NodePtr->getValueInput(0);
  }

  const std::string& ident_name(const Graph& G) const {
    auto* SymStrNode = getSymbolName();
    return NodeProperties<IrOpcode::ConstantStr>(SymStrNode)
           .str(G);
  }

};

NODE_PROPERTIES_VIRT(SrcVarDecl, VirtSrcDecl) {
  NodeProperties(Node *N)
    : NODE_PROP_VIRT(VirtSrcDecl, N) {}

  operator bool() const {
    return NodePtr && NodePtr->getOp() == IrOpcode::SrcVarDecl;
  }
};

NODE_PROPERTIES_VIRT(SrcArrayDecl, VirtSrcDecl) {
  NodeProperties(Node *N)
    : NODE_PROP_VIRT(VirtSrcDecl, N) {}

  operator bool() const {
    return NodePtr && NodePtr->getOp() == IrOpcode::SrcArrayDecl;
  }

  size_t dim_size() const {
    assert(NodePtr->getNumValueInput() > 0);
    return NodePtr->getNumValueInput() - 1U;
  }
  Node* dim(size_t idx) const {
    assert(idx < dim_size() && "dim index out-of-bound");
    return NodePtr->getValueInput(idx + 1);
  }
};

NODE_PROPERTIES(VirtSrcDesigAccess) {
  NodeProperties(Node *N)
    : NODE_PROP_BASE(VirtSrcDesigAccess, N) {}

  operator bool() const {
    if(!NodePtr) return false;
    auto Op = NodePtr->getOp();
    return Op == IrOpcode::SrcVarAccess ||
           Op == IrOpcode::SrcArrayAccess;
  }

  Node* decl() const {
    assert(NodePtr->getNumValueInput() > 0);
    return NodePtr->getValueInput(0);
  }

  Node* effect_dependency() const {
    if(NodePtr->getNumEffectInput() > 0)
      return NodePtr->getEffectInput(0);
    else
      return nullptr;
  }
};

template<>
struct NodeProperties<IrOpcode::SrcVarAccess>
  : public NodeProperties<IrOpcode::VirtSrcDesigAccess> {
  NodeProperties(Node *N)
    : NodeProperties<IrOpcode::VirtSrcDesigAccess>(N) {}

  operator bool() const {
    return NodePtr && NodePtr->Op == IrOpcode::SrcVarAccess;
  }
};

template<>
struct NodeProperties<IrOpcode::SrcArrayAccess>
  : public NodeProperties<IrOpcode::VirtSrcDesigAccess> {
  NodeProperties(Node *N)
    : NodeProperties<IrOpcode::VirtSrcDesigAccess>(N) {}

  operator bool() const {
    return NodePtr && NodePtr->Op == IrOpcode::SrcArrayAccess;
  }

  size_t dim_size() const {
    assert(NodePtr->getNumValueInput() > 0);
    return NodePtr->getNumValueInput() - 1U;
  }
  Node* dim(size_t idx) const {
    assert(idx < dim_size() && "dim index out-of-bound");
    return NodePtr->getValueInput(idx + 1);
  }
};

NODE_PROPERTIES(SrcAssignStmt) {
  NodeProperties(Node *N)
    : NODE_PROP_BASE(SrcAssignStmt, N) {}

  Node* source() const {
    assert(NodePtr->getNumValueInput() > 1);
    return NodePtr->getValueInput(1);
  }
  Node* dest() const {
    assert(NodePtr->getNumValueInput() > 0);
    return NodePtr->getValueInput(0);
  }
};

NODE_PROPERTIES(Merge) {
  NodeProperties(Node *N)
    : NODE_PROP_BASE(Merge, N) {}

  // fortunetly our language is simple enough
  // to have only two branches at all time
  Node* TrueBranch() const {
    assert(NodePtr->getNumControlInput() > 0);
    for(unsigned i = 0, CSize = NodePtr->getNumControlInput();
        i < CSize; ++i) {
      Node* N = NodePtr->getControlInput(i);
      if(NodeProperties<IrOpcode::IfTrue>(N)) return N;
    }
    return nullptr;
  }
  Node* FalseBranch() const {
    for(unsigned i = 0, CSize = NodePtr->getNumControlInput();
        i < CSize; ++i) {
      Node* N = NodePtr->getControlInput(i);
      if(NodeProperties<IrOpcode::IfFalse>(N)) return N;
    }
    return nullptr;
  }
};

NODE_PROPERTIES(VirtIfBranches) {
  NodeProperties(Node *N)
    : NODE_PROP_BASE(VirtIfBranches, N) {}

  operator bool() const {
    if(!NodePtr) return false;
    auto Op = NodePtr->getOp();
    return Op == IrOpcode::IfTrue ||
           Op == IrOpcode::IfFalse;
  }
};

NODE_PROPERTIES(VirtGlobalValues) {
  NodeProperties(Node *N)
    : NODE_PROP_BASE(VirtGlobalValues, N) {}

  operator bool() const {
    if(!NodePtr) return false;
    auto Op = NodePtr->getOp();
    return Op == IrOpcode::ConstantStr ||
           Op == IrOpcode::ConstantInt ||
           Op == IrOpcode::Start;
  }
};

NODE_PROPERTIES(VirtCtrlPoints) {
  NodeProperties(Node *N)
    : NODE_PROP_BASE(VirtCtrlPoints, N) {}

  operator bool() const {
    if(!NodePtr) return false;
    switch(NodePtr->getOp()) {
    case IrOpcode::If:
    case IrOpcode::IfTrue:
    case IrOpcode::IfFalse:
    case IrOpcode::Merge:
    case IrOpcode::Start:
    case IrOpcode::End:
    case IrOpcode::Return:
      return true;
    default:
      return false;
    }
  }
};
#undef NODE_PROP_BASE
#undef NODE_PROPERTIES

template<IrOpcode::ID OP>
struct NodeBuilder {
  Node* Build() {
    gross_unreachable("Unimplemented");
    return nullptr;
  }
};

template<>
struct NodeBuilder<IrOpcode::ConstantInt> {
  NodeBuilder(Graph* graph, int32_t val)
    : G(graph), Val(val) {}

  Node* Build() {
    if(auto* N = G->ConstNumberPool.find_node(Val))
      return N;
    else {
      // New constant Node
      Node* NewN = new Node(IrOpcode::ConstantInt);
      G->ConstNumberPool.insert({NewN, Val});
      G->InsertNode(NewN);
      return NewN;
    }
  }

public:
  Graph* G;
  int32_t Val;
};

template<>
struct NodeBuilder<IrOpcode::ConstantStr> {
  NodeBuilder(Graph* graph, const std::string& Name)
    : G(graph),
      SymName(Name) {}

  Node* Build(){
    if(auto* N = G->ConstStrPool.find_node(SymName))
      return N;
    else {
      // New constant Node
      Node* NewN = new Node(IrOpcode::ConstantStr);
      G->ConstStrPool.insert({NewN, SymName});
      G->InsertNode(NewN);
      return NewN;
    }
  }

private:
  Graph *G;
  const std::string& SymName;
};

template<>
struct NodeBuilder<IrOpcode::SrcVarDecl> {
  NodeBuilder(Graph *graph)
    : G(graph) {}

  NodeBuilder& SetSymbolName(const std::string& Name) {
    SymName = Name;
    return *this;
  }

  Node* Build() {
    Node* SymNameNode = NodeBuilder<IrOpcode::ConstantStr>(G, SymName).Build();
    // Value dependency
    Node* VarDeclNode = new Node(IrOpcode::SrcVarDecl, {SymNameNode});
    SymNameNode->Users.push_back(VarDeclNode);
    G->InsertNode(VarDeclNode);
    return VarDeclNode;
  }

private:
  Graph *G;
  std::string SymName;
};

template<>
struct NodeBuilder<IrOpcode::SrcArrayDecl> {
  NodeBuilder(Graph *graph)
    : G(graph) {}

  NodeBuilder& SetSymbolName(const std::string& Name) {
    SymName = Name;
    return *this;
  }

  // add new dimension
  NodeBuilder& AddDim(Node* DN) {
    Dims.push_back(DN);
    return *this;
  }
  NodeBuilder& AddConstDim(uint32_t Dim) {
    assert(Dim > 0);
    NodeBuilder<IrOpcode::ConstantInt> NB(G, static_cast<int32_t>(Dim));
    return AddDim(NB.Build());
  }
  NodeBuilder& ResetDims() {
    Dims.clear();
    return *this;
  }

  Node* Build() {
    Node* SymNode = NodeBuilder<IrOpcode::ConstantStr>(G, SymName).Build();
    // value dependency
    // 0: symbol string
    // 1..N: dimension expression
    std::vector<Node*> ValDeps{SymNode};
    ValDeps.insert(ValDeps.end(), Dims.begin(), Dims.end());
    Node* ArrDeclNode = new Node(IrOpcode::SrcArrayDecl, ValDeps);
    for(auto* N : ValDeps)
      N->Users.push_back(ArrDeclNode);
    G->InsertNode(ArrDeclNode);
    return ArrDeclNode;
  }

private:
  Graph *G;
  std::string SymName;
  std::vector<Node*> Dims;
};

namespace _details {
// (Simple)Binary Ops
template<IrOpcode::ID OC,
         class SubT = NodeBuilder<OC>>
struct BinOpNodeBuilder {
  BinOpNodeBuilder(Graph* graph) : G(graph) {}

  SubT& LHS(Node* N) {
    LHSNode = N;
    return downstream();
  }
  SubT& RHS(Node* N) {
    RHSNode = N;
    return downstream();
  }

  Node* Build() {
    auto* BinOp = new Node(OC, {LHSNode, RHSNode});
    LHSNode->Users.push_back(BinOp);
    RHSNode->Users.push_back(BinOp);
    G->InsertNode(BinOp);
    return BinOp;
  }

protected:
  Graph *G;
  Node *LHSNode, *RHSNode;

  SubT& downstream() { return *static_cast<SubT*>(this); }
};
} // end namespace _details

#define TRIVIAL_BIN_OP_BUILDER(OC)  \
template<>  \
struct NodeBuilder<IrOpcode::OC> :  \
  public _details::BinOpNodeBuilder<IrOpcode::OC> { \
  NodeBuilder(Graph* graph) : BinOpNodeBuilder(graph) {}  \
}

TRIVIAL_BIN_OP_BUILDER(BinAdd);
TRIVIAL_BIN_OP_BUILDER(BinSub);
TRIVIAL_BIN_OP_BUILDER(BinMul);
TRIVIAL_BIN_OP_BUILDER(BinDiv);
TRIVIAL_BIN_OP_BUILDER(BinLe);
TRIVIAL_BIN_OP_BUILDER(BinLt);
TRIVIAL_BIN_OP_BUILDER(BinGe);
TRIVIAL_BIN_OP_BUILDER(BinGt);
TRIVIAL_BIN_OP_BUILDER(BinEq);
TRIVIAL_BIN_OP_BUILDER(BinNe);
#undef TRIVIAL_BIN_OP_BUILDER

template<>
struct NodeBuilder<IrOpcode::SrcVarAccess> {
  NodeBuilder(Graph* graph) :
    G(graph),
    VarDecl(nullptr), EffectDep(nullptr) {}

  NodeBuilder& Decl(Node* N) {
    VarDecl = N;
    return *this;
  }

  NodeBuilder& Effect(Node* N) {
    EffectDep = N;
    return *this;
  }

  Node* Build(bool Verify = true) {
    if(Verify) {
      NodeProperties<IrOpcode::SrcVarDecl> NP(VarDecl);
      assert(NP && "original decl should be SrcVarDecl");
    }

    std::vector<Node*> Effects;
    if(EffectDep) Effects.push_back(EffectDep);
    auto* N = new Node(IrOpcode::SrcVarAccess,
                       {VarDecl},// value inputs
                       {}/*control inputs*/,
                       Effects/*effect inputs*/);
    VarDecl->Users.push_back(N);
    if(EffectDep) EffectDep->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  Node *VarDecl, *EffectDep;
};

template<>
struct NodeBuilder<IrOpcode::SrcArrayAccess> {
  NodeBuilder(Graph *graph) :
    G(graph),
    VarDecl(nullptr), EffectDep(nullptr) {}

  NodeBuilder& Decl(Node* D) {
    VarDecl = D;
    return *this;
  }
  NodeBuilder& Effect(Node* N) {
    EffectDep = N;
    return *this;
  }

  NodeBuilder& AppendAccessDim(Node* DimExpr) {
    Dims.push_back(DimExpr);
    return *this;
  }
  NodeBuilder& ResetDims() {
    Dims.clear();
    return *this;
  }

  Node* Build(bool Verify=true) {
    if(Verify) {
      // Make sure the amount of dims
      // matches that of original array decl
      NodeProperties<IrOpcode::SrcArrayDecl> NP(VarDecl);
      assert(NP && NP.dim_size() == Dims.size() &&
             "illegal array decl");
    }

    // value dependency
    // 0: array decl
    // 1..N: dimension expression
    std::vector<Node*> ValDeps{VarDecl};
    ValDeps.insert(ValDeps.end(), Dims.begin(), Dims.end());
    std::vector<Node*> EffectDeps;
    if(EffectDep) EffectDeps.push_back(EffectDep);
    Node* ArrAccessNode = new Node(IrOpcode::SrcArrayAccess,
                                   ValDeps, // value dependencies
                                   {}, // control dependencies
                                   EffectDeps); // effect dependencies
    for(auto* N : ValDeps)
      N->Users.push_back(ArrAccessNode);
    if(EffectDep) EffectDep->Users.push_back(ArrAccessNode);
    G->InsertNode(ArrAccessNode);
    return ArrAccessNode;
  }

private:
  Graph *G;
  Node *VarDecl, *EffectDep;
  std::vector<Node*> Dims;
};

template<>
struct NodeBuilder<IrOpcode::SrcAssignStmt> {
  NodeBuilder(Graph *graph) : G(graph) {}

  NodeBuilder& Dest(Node* N) {
    DestNode = N;
    return *this;
  }
  NodeBuilder& Src(Node* N) {
    SrcNode = N;
    return *this;
  }

  Node* Build() {
    auto* N = new Node(IrOpcode::SrcAssignStmt,
                       {DestNode, SrcNode});
    DestNode->Users.push_back(N);
    SrcNode->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Graph *G;
  Node *DestNode, *SrcNode;
};

template<>
struct NodeBuilder<IrOpcode::VirtIfBranches> {
  NodeBuilder(Graph* graph, bool IsTrueBr)
    : G(graph),
      IfNode(nullptr), BranchKind(IsTrueBr) {}

  NodeBuilder& IfStmt(Node* N) {
    IfNode = N;
    return *this;
  }

  Node* Build() {
    assert(IfNode && "If node cannot be null");
    auto* N = new Node(BranchKind? IrOpcode::IfTrue : IrOpcode::IfFalse,
                       {}, {IfNode});
    IfNode->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  Node *IfNode;
  bool BranchKind;
};

template<>
struct NodeBuilder<IrOpcode::If> {
  NodeBuilder(Graph* graph)
    : G(graph),
      Predicate(nullptr) {}

  NodeBuilder& Condition(Node* N) {
    Predicate = N;
    return *this;
  }

  Node* Build() {
    assert(Predicate && "condition can not be null");
    auto* N = new Node(IrOpcode::If,
                       {Predicate});
    Predicate->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }
private:
  Graph* G;
  Node *Predicate;
};

template<>
struct NodeBuilder<IrOpcode::Merge> {
  NodeBuilder(Graph* graph) : G(graph) {}

  NodeBuilder& AddCtrlInput(Node* N) {
    Ctrls.push_back(N);
    return *this;
  }

  Node* Build() {
    auto* N = new Node(IrOpcode::Merge,
                       {}, Ctrls);
    for(auto* Ctrl : Ctrls)
      Ctrl->Users.push_back(Ctrl);
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  std::vector<Node*> Ctrls;
};

template<>
struct NodeBuilder<IrOpcode::Phi> {
  NodeBuilder(Graph* graph) : G(graph) {}

  NodeBuilder& AddValueInput(Node* N) {
    ValueDeps.push_back(N);
    return *this;
  }

  NodeBuilder& AddEffectInput(Node* N) {
    EffectDeps.push_back(N);
    return *this;
  }

  NodeBuilder& SetCtrlMerge(Node* N) {
    MergeNode = N;
    return *this;
  }

  Node* Build() {
    assert(MergeNode && "PHI require control merge point");
    auto* N = new Node(IrOpcode::Phi,
                       ValueDeps, {MergeNode},
                       EffectDeps);
    MergeNode->Users.push_back(N);
    for(auto* VD : ValueDeps)
      VD->Users.push_back(N);
    for(auto* ED : EffectDeps)
      ED->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  std::vector<Node*> ValueDeps, EffectDeps;
  Node* MergeNode;
};

template<>
struct NodeBuilder<IrOpcode::Argument> {
  NodeBuilder(Graph* graph, const std::string& Name)
    : G(graph),
      SrcArgName(Name) {}

  Node* Build() {
    auto* NameStrNode = NodeBuilder<IrOpcode::ConstantStr>(G, SrcArgName)
                        .Build();
    auto* N = new Node(IrOpcode::Argument, {NameStrNode});
    NameStrNode->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  const std::string& SrcArgName;
};

template<>
struct NodeBuilder<IrOpcode::VirtFuncPrototype> {
  NodeBuilder(Graph* graph) : G(graph) {}

  NodeBuilder& FuncName(const std::string& NameStr) {
    NameStrNode = NodeBuilder<IrOpcode::ConstantStr>(G, NameStr)
                  .Build();
    return *this;
  }

  NodeBuilder& AddParameter(Node* N) {
    Parameters.push_back(N);
    return *this;
  }

  Node* Build(bool Verify = true) {
    if(Verify) {
      if(!NameStrNode) {
        Log::E() << "Require a name for the function\n";
        return nullptr;
      }
      for(auto* PN : Parameters)
        if(!NodeProperties<IrOpcode::Argument>(PN)) {
          Log::E() << "Expecting Argument kind Node\n";
          return nullptr;
        }
    }

    // Start node has effect dependency on arguments
    auto* StartNode = new Node(IrOpcode::Start,
                               {NameStrNode},
                               {}, Parameters);
    NameStrNode->Users.push_back(StartNode);
    for(auto* PN : Parameters)
      PN->Users.push_back(StartNode);
    G->InsertNode(StartNode);
    return StartNode;
  }

private:
  Graph* G;
  Node *NameStrNode;
  std::vector<Node*> Parameters;
};

template<>
struct NodeBuilder<IrOpcode::End> {
  NodeBuilder(Graph* graph, Node* Start)
    : G(graph), StartNode(Start) {}

  NodeBuilder& AddTerminator(Node* N) {
    TermNodes.push_back(N);
    return *this;
  }

  Node* Build() {
    // control dependent on terminator nodes,
    // or start node if the former one is absent
    std::vector<Node*> CtrlDeps;
    if(TermNodes.empty())
      CtrlDeps = {StartNode};
    else
      CtrlDeps = std::move(TermNodes);
    auto* N = new Node(IrOpcode::End,
                       {}, CtrlDeps);
    for(auto* TN : CtrlDeps)
      TN->Users.push_back(N);
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  Node* StartNode;
  std::vector<Node*> TermNodes;
};

template<>
struct NodeBuilder<IrOpcode::Return> {
  explicit
  NodeBuilder(Graph* graph, Node* RetExpr = nullptr)
    : G(graph), ReturnExpr(RetExpr) {}

  Node* Build() {
    Node* N = nullptr;
    if(ReturnExpr) {
      N = new Node(IrOpcode::Return, {ReturnExpr});
      ReturnExpr->Users.push_back(N);
    } else {
      N = new Node(IrOpcode::Return, {});
    }
    G->InsertNode(N);
    return N;
  }

private:
  Graph* G;
  Node* ReturnExpr;
};

Node* FindNearestCtrlPoint(Node* N);
} // end namespace gross
#endif
