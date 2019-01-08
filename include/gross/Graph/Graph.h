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

  std::vector<std::unique_ptr<Node>> Nodes;

  // Constant pools
  NodeBiMap<std::string> ConstStrPool;
  NodeBiMap<int32_t> ConstNumberPool;

  // Function map
  // function name -> Start node of a function
  std::unordered_map<std::string, Node*> FuncMap;

public:
  void InsertNode(Node* N);
};
} // end namespace gross
#endif
