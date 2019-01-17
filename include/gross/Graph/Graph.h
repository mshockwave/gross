#ifndef GROSS_GRAPH_GRAPH_H
#define GROSS_GRAPH_GRAPH_H
#include "gross/Graph/Node.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gross {
// Owner of nodes
class Graph {
  template<IrOpcode::ID Op>
  friend class NodeBuilder;
  template<IrOpcode::ID Op>
  friend struct NodeProperties;

  std::vector<std::unique_ptr<Node>> Nodes;
  std::unordered_set<Use> Edges;

  // Constant pools
  NodeBiMap<std::string> ConstStrPool;
  NodeBiMap<int32_t> ConstNumberPool;

  // Function map
  // function name -> Start node of a function
  std::unordered_map<std::string, Node*> FuncMap;

public:
  using node_iterator = typename decltype(Nodes)::iterator;
  using const_node_iterator = typename decltype(Nodes)::const_iterator;
  node_iterator node_begin() { return Nodes.begin(); }
  const_node_iterator node_cbegin() const { return Nodes.cbegin(); }
  node_iterator node_end() { return Nodes.end(); }
  const_node_iterator node_cend() const { return Nodes.cend(); }
  size_t node_size() const { return Nodes.size(); }

  using edge_iterator = typename decltype(Edges)::iterator;
  edge_iterator edge_begin() { return Edges.begin(); }
  edge_iterator edge_end() { return Edges.end(); }
  size_t edge_size() const { return Edges.size(); }

  void InsertNode(Node* N);

  size_t getNumConstStr() const {
    return ConstStrPool.size();
  }
  size_t getNumConstNumber() const {
    return ConstNumberPool.size();
  }

  // map from vertex or edge to an unique id
  template<class PropertyTag>
  struct id_map;
};

// default(empty) implementation
template<class PropertyTag>
struct Graph::id_map {};
} // end namespace gross
#endif
