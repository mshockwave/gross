#ifndef GROSS_GRAPH_GRAPH_H
#define GROSS_GRAPH_GRAPH_H
#include "boost/iterator/iterator_facade.hpp"
#include "gross/Graph/Node.h"
#include <memory>
#include <iostream>
#include <vector>

namespace gross {
// Forward declaration
template<class GraphT>
class lazy_edge_iterator;
class SubGraph;

// map from vertex or edge to an unique id
template<class GraphT, class PropertyTag>
struct graph_id_map {};

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

  // Basically functions
  std::vector<SubGraph> SubRegions;

public:
  using node_iterator = typename decltype(Nodes)::iterator;
  using const_node_iterator = typename decltype(Nodes)::const_iterator;
  static Node* GetNodeFromIt(const node_iterator& NodeIt) {
    return NodeIt->get();
  }
  static const Node* GetNodeFromIt(const const_node_iterator& NodeIt) {
    return NodeIt->get();
  }
  node_iterator node_begin() { return Nodes.begin(); }
  const_node_iterator node_cbegin() const { return Nodes.cbegin(); }
  node_iterator node_end() { return Nodes.end(); }
  const_node_iterator node_cend() const { return Nodes.cend(); }
  Node* getNode(size_t idx) const { return Nodes.at(idx).get(); }
  size_t node_size() const { return Nodes.size(); }

  using edge_iterator = lazy_edge_iterator<Graph>;
  edge_iterator edge_begin();
  edge_iterator edge_end();
  size_t edge_size();
  size_t edge_size() const { return const_cast<Graph*>(this)->edge_size(); }

  void InsertNode(Node* N);

  void AddSubRegion(const SubGraph& SubG);
  void AddSubRegion(SubGraph&& SubG);

  size_t getNumConstStr() const {
    return ConstStrPool.size();
  }
  size_t getNumConstNumber() const {
    return ConstNumberPool.size();
  }

  // dump to GraphViz graph
  void dumpGraphviz(std::ostream& OS);
};

// lightweight nodes holder for part of the Graph
class SubGraph {
  std::vector<Node*> Nodes;

public:
  SubGraph() = default;

  template<class InputIt>
  SubGraph(InputIt begin, InputIt end)
    : Nodes(begin, end) {}
  template<class ContainerT>
  explicit SubGraph(const ContainerT& InitNodes)
    : Nodes(InitNodes.cbegin(), InitNodes.cend()) {}

  explicit SubGraph(std::vector<Node*>&& OtherNodes)
    : Nodes(OtherNodes) {}

  using node_iterator = typename decltype(Nodes)::iterator;
  using const_node_iterator = typename decltype(Nodes)::const_iterator;
  static Node* GetNodeFromIt(const node_iterator& NodeIt) { return *NodeIt; }
  static const Node* GetNodeFromIt(const const_node_iterator& NodeIt) {
    return *NodeIt;
  }
  node_iterator node_begin() { return Nodes.begin(); }
  node_iterator node_end() { return Nodes.end(); }
  const_node_iterator node_cbegin() const { return Nodes.cbegin(); }
  const_node_iterator node_cend() const { return Nodes.cend(); }
  size_t node_size() const { return Nodes.size(); }

  using edge_iterator = lazy_edge_iterator<SubGraph>;
  edge_iterator edge_begin();
  edge_iterator edge_end();
  size_t edge_size();
  size_t edge_size() const { return const_cast<SubGraph*>(this)->edge_size(); }
};

template<class GraphT>
class lazy_edge_iterator
  : public boost::iterator_facade<lazy_edge_iterator<GraphT>,
                                  Use, // Value type
                                  boost::forward_traversal_tag, // Traversal tag
                                  Use // Reference type
                                  > {
  friend class boost::iterator_core_access;
  GraphT* G;
  unsigned CurInput;
  typename GraphT::node_iterator CurNodeIt, EndNodeIt;

  // no checks!
  inline Node* CurNode() const {
    return GraphT::GetNodeFromIt(CurNodeIt);
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
  explicit lazy_edge_iterator(GraphT* graph, bool isEnd = false)
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
} // end namespace gross
#endif
