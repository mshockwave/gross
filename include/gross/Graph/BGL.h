#ifndef GROSS_GRAPH_BGL_H
#define GROSS_GRAPH_BGL_H
/// Defining required traits for boost graph library
#include "boost/graph/graph_traits.hpp"
#include "boost/graph/properties.hpp"
#include "boost/property_map/property_map.hpp"
#include "boost/iterator/transform_iterator.hpp"
#include "gross/Graph/Graph.h"
#include "gross/Graph/Node.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Support/STLExtras.h"
#include "gross/Support/type_traits.h"
#include <utility>
#include <iostream>

namespace boost {
template<>
struct graph_traits<gross::Graph> {
  /// GraphConcept
  using vertex_descriptor = gross::Node*;
  using edge_descriptor = gross::Use;
  using directed_category = boost::directed_tag;
  using edge_parallel_category = boost::allow_parallel_edge_tag;

  static vertex_descriptor null_vertex() {
    return nullptr;
  }

  /// VertexListGraphConcept
  using vertex_iterator
    = boost::transform_iterator<gross::unique_ptr_unwrapper<gross::Node>,
                                typename gross::Graph::node_iterator,
                                gross::Node*, // Refrence type
                                gross::Node* // Value type
                                >;
  using vertices_size_type = size_t;

  /// EdgeListGraphConcept
  using edges_size_type = size_t;
  using edge_iterator = typename gross::Graph::edge_iterator;

  /// IncidenceGraphConcept
  using out_edge_iterator
    = boost::transform_iterator<typename gross::Use::BuilderFunctor,
                                typename gross::Node::input_iterator,
                                gross::Use, // Reference type
                                gross::Use // Value type
                                >;
  using degree_size_type = size_t;

  struct traversal_category :
    public boost::vertex_list_graph_tag,
    public boost::edge_list_graph_tag,
    public boost::incidence_graph_tag {};
};

template<>
struct graph_traits<gross::SubGraph> {
  /// GraphConcept
  using vertex_descriptor = gross::Node*;
  using edge_descriptor = gross::Use;
  using directed_category = boost::directed_tag;
  using edge_parallel_category = boost::allow_parallel_edge_tag;

  static vertex_descriptor null_vertex() {
    return nullptr;
  }

  /// VertexListGraphConcept
  using vertex_iterator = typename gross::SubGraph::node_iterator;
  using vertices_size_type = size_t;

  /// EdgeListGraphConcept
  using edges_size_type = size_t;
  using edge_iterator = typename gross::SubGraph::edge_iterator;

  /// IncidenceGraphConcept
  using out_edge_iterator
    = boost::transform_iterator<typename gross::Use::BuilderFunctor,
                                typename gross::Node::input_iterator,
                                gross::Use, // Reference type
                                gross::Use // Value type
                                >;
  using degree_size_type = size_t;

  struct traversal_category :
    public boost::vertex_list_graph_tag,
    public boost::edge_list_graph_tag,
    public boost::incidence_graph_tag {};
};

/// Note: We mark most of the BGL trait functions here as inline
/// because they're trivial.
/// FIXME: Will putting them into separated source file helps reducing
/// compilation time?

/// VertexListGraphConcept
inline
std::pair<typename boost::graph_traits<gross::Graph>::vertex_iterator,
          typename boost::graph_traits<gross::Graph>::vertex_iterator>
vertices(gross::Graph& g) {
  using vertex_it_t
    = typename boost::graph_traits<gross::Graph>::vertex_iterator;
  gross::unique_ptr_unwrapper<gross::Node> functor;
  return std::make_pair(
    vertex_it_t(g.node_begin(), functor),
    vertex_it_t(g.node_end(), functor)
  );
}
inline
std::pair<typename boost::graph_traits<gross::Graph>::vertex_iterator,
          typename boost::graph_traits<gross::Graph>::vertex_iterator>
vertices(const gross::Graph& g) {
  return vertices(const_cast<gross::Graph&>(g));
}
// SubGraph
inline
std::pair<typename boost::graph_traits<gross::SubGraph>::vertex_iterator,
          typename boost::graph_traits<gross::SubGraph>::vertex_iterator>
vertices(gross::SubGraph& g) {
  return std::make_pair(g.node_begin(), g.node_end());
}
inline
std::pair<typename boost::graph_traits<gross::SubGraph>::vertex_iterator,
          typename boost::graph_traits<gross::SubGraph>::vertex_iterator>
vertices(const gross::SubGraph& g) {
  return vertices(const_cast<gross::SubGraph&>(g));
}

inline typename boost::graph_traits<gross::Graph>::vertices_size_type
num_vertices(gross::Graph& g) {
  return const_cast<const gross::Graph&>(g).node_size();
}
inline typename boost::graph_traits<gross::Graph>::vertices_size_type
num_vertices(const gross::Graph& g) {
  return g.node_size();
}
// SubGraph
inline typename boost::graph_traits<gross::SubGraph>::vertices_size_type
num_vertices(gross::SubGraph& g) {
  return const_cast<const gross::SubGraph&>(g).node_size();
}
inline typename boost::graph_traits<gross::SubGraph>::vertices_size_type
num_vertices(const gross::SubGraph& g) {
  return g.node_size();
}

/// EdgeListGraphConcept
inline
std::pair<typename boost::graph_traits<gross::Graph>::edge_iterator,
          typename boost::graph_traits<gross::Graph>::edge_iterator>
edges(gross::Graph& g) {
  return std::make_pair(g.edge_begin(), g.edge_end());
}
inline
std::pair<typename boost::graph_traits<gross::Graph>::edge_iterator,
          typename boost::graph_traits<gross::Graph>::edge_iterator>
edges(const gross::Graph& g) {
  return edges(const_cast<gross::Graph&>(g));
}
// SubGraph
inline
std::pair<typename boost::graph_traits<gross::SubGraph>::edge_iterator,
          typename boost::graph_traits<gross::SubGraph>::edge_iterator>
edges(gross::SubGraph& g) {
  return std::make_pair(g.edge_begin(), g.edge_end());
}
inline
std::pair<typename boost::graph_traits<gross::SubGraph>::edge_iterator,
          typename boost::graph_traits<gross::SubGraph>::edge_iterator>
edges(const gross::SubGraph& g) {
  return edges(const_cast<gross::SubGraph&>(g));
}

inline typename boost::graph_traits<gross::Graph>::edges_size_type
num_edges(gross::Graph& g) {
  return const_cast<const gross::Graph&>(g).edge_size();
}
inline typename boost::graph_traits<gross::Graph>::edges_size_type
num_edges(const gross::Graph& g) {
  return g.edge_size();
}
// SubGraph
inline typename boost::graph_traits<gross::SubGraph>::edges_size_type
num_edges(gross::SubGraph& g) {
  return const_cast<const gross::SubGraph&>(g).edge_size();
}
inline typename boost::graph_traits<gross::SubGraph>::edges_size_type
num_edges(const gross::SubGraph& g) {
  return g.edge_size();
}

template<class GraphT>
inline gross::enable_if_t<
  std::is_same<GraphT,gross::Graph>::value ||
  std::is_same<GraphT,gross::SubGraph>::value,
  typename boost::graph_traits<GraphT>::vertex_descriptor>
source(const gross::Use& e, const GraphT& g) {
  return const_cast<gross::Node*>(e.Source);
}
template<class GraphT>
inline gross::enable_if_t<
  std::is_same<GraphT,gross::Graph>::value ||
  std::is_same<GraphT,gross::SubGraph>::value,
  typename boost::graph_traits<GraphT>::vertex_descriptor>
target(const gross::Use& e, const GraphT& g) {
  return const_cast<gross::Node*>(e.Dest);
}

/// IncidenceGraphConcept
template<class T>
inline gross::enable_if_t<
  std::is_same<T, gross::Graph>::value ||
  std::is_same<T, gross::SubGraph>::value,
  std::pair<typename boost::graph_traits<T>::out_edge_iterator,
            typename boost::graph_traits<T>::out_edge_iterator>>
out_edges(gross::Node* u, const T& g) {
  // for now, we don't care about the kind of edge
  using edge_it_t
    = typename boost::graph_traits<T>::out_edge_iterator;
  gross::Use::BuilderFunctor functor(u, gross::Use::K_NONE,
                                     g.GetEdgePatcher());
  return std::make_pair(
    edge_it_t(u->inputs().begin(), functor),
    edge_it_t(u->inputs().end(), functor)
  );
}

template<class T>
inline gross::enable_if_t<
  std::is_same<T, gross::Graph>::value ||
  std::is_same<T, gross::SubGraph>::value,
  typename boost::graph_traits<T>::degree_size_type>
out_degree(gross::Node* u, const T& g) {
  return u->getNumValueInput()
         + u->getNumControlInput()
         + u->getNumEffectInput();
}
} // end namespace boost

/// Property Map Concept
namespace gross {
template<class GraphT>
struct graph_id_map<GraphT, boost::vertex_index_t> {
  using value_type = size_t;
  using reference = size_t;
  using key_type = Node*;
  struct category : public boost::readable_property_map_tag {};

  graph_id_map(const GraphT& g) : G(g) {}

  reference operator[](const key_type& key) const {
    // just use linear search for now
    // TODO: improve time complexity
    value_type Idx = 0;
    for(auto I = G.node_cbegin(), E = G.node_cend();
        I != E; ++I, ++Idx) {
      if(GraphT::GetNodeFromIt(I) == key) break;
    }
    return Idx;
  }

private:
  const GraphT &G;
};
} // end namespace gross

namespace boost {
// get() for vertex id property map
template<class GraphT>
inline gross::enable_if_t<
  std::is_same<GraphT,gross::Graph>::value ||
  std::is_same<GraphT,gross::SubGraph>::value,
  typename gross::graph_id_map<GraphT,boost::vertex_index_t>::reference>
get(const typename gross::graph_id_map<GraphT,boost::vertex_index_t> &pmap,
    const typename gross::graph_id_map<GraphT,boost::vertex_index_t>::key_type
    &key) {
  return pmap[key];
}
// get() for getting vertex id property map from graph
template<class GraphT>
inline gross::enable_if_t<
  std::is_same<GraphT,gross::Graph>::value ||
  std::is_same<GraphT,gross::SubGraph>::value,
  typename gross::graph_id_map<GraphT,boost::vertex_index_t>>
get(boost::vertex_index_t tag, const GraphT& g) {
  return gross::graph_id_map<GraphT,boost::vertex_index_t>(g);
}
} // end namespace boost

/// PropertyWriter Concept
namespace gross {
struct graph_prop_writer {
  void operator()(std::ostream& OS) const {
    // print the graph 'upside down'
    OS << "rankdir = BT;" << std::endl;
  }
};

struct graph_vertex_prop_writer {
  graph_vertex_prop_writer(const Graph& g) : G(g) {}

  void operator()(std::ostream& OS, const Node* v) const {
    Node* N = const_cast<Node*>(v);
    OS << "[label=\"";
    IrOpcode::Print(G, OS, N);
    OS << "\"]";
  }

private:
  const Graph& G;
};

struct graph_edge_prop_writer {
  void operator()(std::ostream& OS, const Use& U) const {
    switch(U.DepKind) {
    case Use::K_VALUE:
      OS << "[color=\"black\"]";
      break;
    case Use::K_CONTROL:
      OS << "[color=\"blue\"]";
      break;
    case Use::K_EFFECT:
      OS << "[color=\"red\", style=\"dashed\"]";
      break;
    default:
      gross_unreachable("Invalid edge kind");
    }
  }
};
} // end namespace gross
#endif
