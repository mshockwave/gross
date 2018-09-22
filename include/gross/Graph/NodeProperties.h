#ifndef GROSS_GRAPH_NODE_PROPERTIES_H
#define GROSS_GRAPH_NODE_PROPERTIES_H
#include "gross/Graph/Node.h"
#include <string>

namespace gross {
// Forward declaration
class Graph;

template<IrOpcode::ID Op>
class NodePropertiesBase {
protected:
  Node* NodePtr;

  NodePropertiesBase(Node* N)
    : NodePtr(N) {}

public:
  bool operator bool() const {
    return NodePtr && NodePtr->Op == Op;
  }
}

template<IrOpcode::ID Op>
struct NodeProperties : public NodePropertiesBase<Op> {
  NodeProperties(Node* N) : NodePropertiesBase(N) {}
};

#define NODE_PROPERTIES(OP) \
  template<>  \
  struct NodeProperties<IrOpcode::OP> : public NodePropertiesBase<IrOpcode::OP>

NODE_PROPERTIES(SrcVarDecl) {
  NodeProperties(Node *N)
    : NodePropertiesBase(N) {}

  Node* getSymbolName() const {
    assert(NodePtr->getNumValueInput() > 0);
    return NodePtr->getValueInput(0);
  }
};

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
  NodeBuilder(const std::string& Name)
    : NewNode(nullptr),
      SymName(Name) {}

  Node* Build();

private:
  Node* NewNode;
  const std::string& SymName;
};

template<>
struct NodeBuilder<IrOpcode::SrcVarDecl> {
  NodeBuilder()
    : NewNode(nullptr) {}

  NodeBuilder& SetSymbolName(const std::string& Name);

  Node* Build();

private:
  Node* NewNode;
};

} // end namespace gross
#endif
