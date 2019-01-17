#ifndef GROSS_GRAPH_BGL_H
#define GROSS_GRAPH_BGL_H
/// Defining required traits for boost graph library
#include <utility>
#include "boost/graph/graph_traits.hpp"
#include "boost/iterator/transform_iterator.hpp"
#include "gross/Graph/Graph.h"
#include "gross/Graph/Node.h"
#include "gross/Support/STLExtras.h"

namespace boost {
template<>
struct graph_traits<gross::Graph> {
  /// GraphConcept
  using vertex_descriptor = gross::Node*;
  using edge_descriptor = gross::Use;
  using directed_category = boost::directed_tag;
  using edge_parallel_category = boost::allow_parallel_edge_tag;
  struct traversal_category :
    public boost::vertex_list_graph_tag,
    public boost::edge_list_graph_tag {};

  vertex_descriptor null_vertex() {
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
};

/// VertexListGraphConcept
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
std::pair<typename boost::graph_traits<gross::Graph>::vertex_iterator,
          typename boost::graph_traits<gross::Graph>::vertex_iterator>
vertices(const gross::Graph& g) {
  return vertices(const_cast<gross::Graph&>(g));
}

typename boost::graph_traits<gross::Graph>::vertices_size_type
num_vertices(gross::Graph& g) {
  return const_cast<const gross::Graph&>(g).node_size();
}
typename boost::graph_traits<gross::Graph>::vertices_size_type
num_vertices(const gross::Graph& g) {
  return g.node_size();
}

/// EdgeListGraphConcept
std::pair<typename boost::graph_traits<gross::Graph>::edge_iterator,
          typename boost::graph_traits<gross::Graph>::edge_iterator>
edges(gross::Graph& g) {
  return std::make_pair(g.edge_begin(), g.edge_end());
}
std::pair<typename boost::graph_traits<gross::Graph>::edge_iterator,
          typename boost::graph_traits<gross::Graph>::edge_iterator>
edges(const gross::Graph& g) {
  return edges(const_cast<gross::Graph&>(g));
}

typename boost::graph_traits<gross::Graph>::edges_size_type
num_edges(gross::Graph& g) {
  return const_cast<const gross::Graph&>(g).edge_size();
}
typename boost::graph_traits<gross::Graph>::edges_size_type
num_edges(const gross::Graph& g) {
  return g.edge_size();
}

typename boost::graph_traits<gross::Graph>::vertex_descriptor
source(const gross::Use& e, const gross::Graph& g) {
  return const_cast<gross::Node*>(e.Source);
}
typename boost::graph_traits<gross::Graph>::vertex_descriptor
target(const gross::Use& e, const gross::Graph& g) {
  return const_cast<gross::Node*>(e.Dest);
}
} // end namespace boost
#endif
