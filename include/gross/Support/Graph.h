#ifndef GROSS_SUPPORT_GRAPH_H
#define GROSS_SUPPORT_GRAPH_H
#include "boost/iterator/iterator_facade.hpp"
#include "boost/graph/properties.hpp"
#include "gross/Graph/Node.h"
#include "gross/Support/type_traits.h"
#include <vector>
#include <set>

namespace gross {
// map from vertex or edge to an unique id
template<class GraphT, class PropertyTag>
struct graph_id_map {};

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
  // we use ADT instead of NodeMarker because
  // the latter will make empty iterator(i.e. default ctor)
  // more difficult
  std::set<NodeT> Visited;

  void Enqueue(NodeT N) {
    Queue.push_back(N);
    Visited.insert(N);
  }

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
    Queue.erase(Queue.begin());
    for(NodeT N : Top->inputs()) {
      if(Visited.count(N)) continue;
      Enqueue(N);
    }
  }

public:
  lazy_node_iterator() = default;
  explicit lazy_node_iterator(NodeT EndNode) {
    Enqueue(EndNode);
  }
};

// since boost::depth_first_search has some really STUPID
// copy by value ColorMap parameter, we need some stub/proxy
// to hold map storage across several usages.
template<class T, class VertexTy>
struct StubColorMap {
  using value_type = boost::default_color_type;
  using reference = value_type&;
  using key_type = VertexTy*;
  struct category : public boost::read_write_property_map_tag {};

  StubColorMap(T& Impl) : Storage(Impl) {}
  StubColorMap() = delete;
  StubColorMap(const StubColorMap& Other) = default;

  reference get(const key_type& key) const {
    return const_cast<reference>(Storage.at(key));
  }
  void put(const key_type& key, const value_type& val) {
    Storage[key] = val;
  }

private:
  T& Storage;
};

// it is very strange that both StubColorMap and graph_id_map<G,T>
// are PropertyMapConcept, but the get() function for the former one
// should be defined in namespace gross :\
/// ColorMap PropertyMap
template<class T, class Vertex>
inline typename gross::StubColorMap<T,Vertex>::reference
get(const gross::StubColorMap<T,Vertex>& pmap,
    const typename gross::StubColorMap<T,Vertex>::key_type& key) {
  return pmap.get(key);
}

template<class T, class Vertex> inline
void put(gross::StubColorMap<T,Vertex>& pmap,
         const typename gross::StubColorMap<T,Vertex>::key_type& key,
         const typename gross::StubColorMap<T,Vertex>::value_type& val) {
  return pmap.put(key, val);
}
} // end namespace gross
#endif
