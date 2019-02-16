#ifndef GROSS_SUPPORT_GRAPH_H
#define GROSS_SUPPORT_GRAPH_H
#include "boost/iterator/iterator_facade.hpp"
#include "gross/Graph/Node.h"
#include "gross/Support/type_traits.h"
#include <vector>
#include <set>

namespace gross {
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
  // we use ADT instead of NodeMarker because
  // the latter will make empty iterator(i.e. default ctor)
  // more difficult
  std::set<Node*> Visited;

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
    while((CurNodeIt != EndNodeIt &&
           CurInput >= CurNode()->Inputs.size()) ||
          (CurNodeIt != EndNodeIt &&
           Visited.count(CurNode()))) {
      // switch node
      ++CurNodeIt;
      CurInput = 0;

      if(CurNodeIt != EndNodeIt) Visited.insert(CurNode());
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

// Basically a BFS traversal iterator
template<class GraphT, bool IsConst,
         typename NodeT = gross::conditional_t<IsConst, const Node*, Node*>>
class lazy_node_iterator
  : public boost::iterator_facade<lazy_node_iterator<GraphT,IsConst,NodeT>,
                                  NodeT, // Value type
                                  boost::forward_traversal_tag, // Traversal tag
                                  NodeT // Reference type
                                  > {
  friend class boost::iterator_core_access;
  std::vector<NodeT> Queue;
  std::set<NodeT> Visited;

  bool equal(const lazy_node_iterator& Other) const {
    if(Queue.size() != Other.Queue.size()) return false;
    if(!Queue.size()) return true;
    // deep compare
    for(auto I1 = Queue.cbegin(), I2 = Other.Queue.cbegin(),
             E1 = Queue.cend(), E2 = Other.Queue.cend();
        I1 != E1 && I2 != E2; ++I1, ++I2) {
      if(*I1 != *I2) return false;
    }
    return true;
  }

  NodeT dereference() const {
    assert(!Queue.empty());
    return Queue.front();
  }

  void increment() {
    NodeT Top = Queue.front();
    Queue.erase(Queue.cbegin());
    Visited.insert(Top);
    for(NodeT N : Top->inputs()) {
      if(Visited.count(N)) continue;
      Queue.push_back(N);
    }
  }

public:
  lazy_node_iterator() = default;
  explicit lazy_node_iterator(NodeT EndNode) {
    Queue.push_back(EndNode);
    Visited.insert(EndNode);
  }
};
} // end namespace gross
#endif
