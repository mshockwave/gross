#ifndef GROSS_GRAPH_BGL_H
#define GROSS_GRAPH_BGL_H
/// Defining required traits for boost graph library
#include "boost/graph/graph_traits.hpp"
#include "gross/Graph/Graph.h"
#include "gross/Graph/Node.h"

template<>
struct boost::graph_traits<gross::Graph> {
  /// Graph concepts
  using vertex_descriptor = gross::Node*;
  using edge_descriptor = gross::Use;
  using directed_category = boost::directed_tag;
  using edge_parallel_category = boost::allow_parallel_edge_tag;
  // FIXME: make sure this is correct
  using traversal_category = boost::vertex_list_graph_tag;
  vertex_descriptor null_vertex() {
    return nullptr;
  }
};
#endif
