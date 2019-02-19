#ifndef GROSS_GRAPH_NODE_UTILS_BASE_H
#define GROSS_GRAPH_NODE_UTILS_BASE_H
#include "gross/Graph/Node.h"
#include "gross/Graph/Graph.h"

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

template<IrOpcode::ID OP>
struct NodeBuilder {
  Node* Build() {
    gross_unreachable("Unimplemented");
    return nullptr;
  }
};
} // end namespace gross
#endif
