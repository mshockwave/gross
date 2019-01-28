#ifndef GROSS_GRAPH_GRAPH_H
#define GROSS_GRAPH_GRAPH_H
#include "boost/iterator/iterator_facade.hpp"
#include "gross/Graph/Node.h"
#include <memory>
#include <unordered_map>
#include <iostream>
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

  class lazy_edge_iterator;

public:
  using node_iterator = typename decltype(Nodes)::iterator;
  using const_node_iterator = typename decltype(Nodes)::const_iterator;
  node_iterator node_begin() { return Nodes.begin(); }
  const_node_iterator node_cbegin() const { return Nodes.cbegin(); }
  node_iterator node_end() { return Nodes.end(); }
  const_node_iterator node_cend() const { return Nodes.cend(); }
  size_t node_size() const { return Nodes.size(); }

  using edge_iterator = lazy_edge_iterator;
  edge_iterator edge_begin();
  edge_iterator edge_end();
  size_t edge_size();
  size_t edge_size() const { return const_cast<Graph*>(this)->edge_size(); }

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

  // dump to GraphViz graph
  void dumpGraphviz(std::ostream& OS);
};

class Graph::lazy_edge_iterator
  : public boost::iterator_facade<Graph::lazy_edge_iterator,
                                  Use, // Value type
                                  boost::forward_traversal_tag, // Traversal tag
                                  Use // Reference type
                                  > {
  friend class boost::iterator_core_access;
  Graph* G;
  unsigned CurInput;
  typename Graph::node_iterator CurNodeIt, EndNodeIt;

  // no checks!
  inline Node* CurNode() const {
    return CurNodeIt->get();
  }

  inline bool isValid() const {
    return G &&
           CurNodeIt != EndNodeIt &&
           CurInput < CurNode()->Inputs.size();
  }

  bool equal(const lazy_edge_iterator& Other) const {
    return G == Other.G &&
           CurNodeIt == Other.CurNodeIt &&
           (CurInput == Other.CurInput ||
            CurNodeIt == EndNodeIt);
  }

  void nextValidPos() {
    while(CurNodeIt != EndNodeIt &&
          CurInput >= CurNode()->Inputs.size()) {
      // switch node
      ++CurNodeIt;
      CurInput = 0;
    }
  }

  void increment() {
    ++CurInput;
    nextValidPos();
  }

  Use dereference() const {
    //if(!isValid()) return Use();
    assert(isValid() && "can not dereference invalid iterator");
    auto DepK = CurNode()->inputUseKind(CurInput);
    return Use(CurNode(), CurNode()->Inputs.at(CurInput), DepK);
  }

public:
  lazy_edge_iterator()
    : G(nullptr),
      CurInput(0) {}
  explicit lazy_edge_iterator(Graph* graph, bool isEnd = false)
    : G(graph),
      CurInput(0) {
    if(G) {
      EndNodeIt = G->node_end();
      if(!isEnd) {
        CurNodeIt = G->node_begin();
        nextValidPos();
      } else {
        CurNodeIt = EndNodeIt;
      }
    }
  }
};

// default(empty) implementation
template<class PropertyTag>
struct Graph::id_map {};
} // end namespace gross
#endif
