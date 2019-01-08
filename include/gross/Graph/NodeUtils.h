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
struct NodeBuilder<IrOpcode::SrcSymbolName> {
  NodeBuilder(Graph* graph, const std::string& Name)
    : G(graph),
      SymName(Name) {}

  Node* Build(){
    if(auto* N = G->ConstStrPool.find_node(SymName))
      return N;
    else {
      // New constant Node
      Node* NewN = new Node(IrOpcode::SrcSymbolName);
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
    Node* SymNameNode = NodeBuilder<IrOpcode::SrcSymbolName>(G, SymName).Build();
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

} // end namespace gross
#endif
