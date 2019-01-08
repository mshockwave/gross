#ifndef GROSS_GRAPH_NODE_UTILS_H
#define GROSS_GRAPH_NODE_UTILS_H
#include "gross/Graph/Node.h"
#include "gross/Graph/Graph.h"
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

#define NODE_PROP_BASE(OP, NODE) \
  NodePropertiesBase<IrOpcode::OP>(NODE)

NODE_PROPERTIES(SrcVarDecl) {
  NodeProperties(Node *N)
    : NODE_PROP_BASE(SrcVarDecl, N) {}

  Node* getSymbolName() const {
    assert(NodePtr->getNumValueInput() > 0);
    return NodePtr->getValueInput(0);
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

} // end namespace gross
#endif
