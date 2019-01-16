#ifndef GROSS_GRAPH_GRAPH_H
#define GROSS_GRAPH_GRAPH_H
#include "gross/Graph/Node.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace gross {
// Owner of nodes
class Graph {
  template<IrOpcode::ID Op>
  friend class NodeBuilder;
  template<IrOpcode::ID Op>
  friend struct NodeProperties;

  std::vector<std::unique_ptr<Node>> Nodes;

  // Constant pools
  NodeBiMap<std::string> ConstStrPool;
  NodeBiMap<int32_t> ConstNumberPool;

  // Function map
  // function name -> Start node of a function
  std::unordered_map<std::string, Node*> FuncMap;

public:
  using node_iterator = typename decltype(Nodes)::iterator;
  node_iterator node_begin() { return Nodes.begin(); }
  node_iterator node_end() { return Nodes.end(); }

  size_t node_size() const { return Nodes.size(); }

  void InsertNode(Node* N);

  size_t getNumConstStr() const {
    return ConstStrPool.size();
  }
  size_t getNumConstNumber() const {
    return ConstNumberPool.size();
  }
};
} // end namespace gross
#endif
