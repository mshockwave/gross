#include "gross/Graph/Graph.h"
#include "gross/Graph/BGL.h"
#include "boost/graph/graphviz.hpp"
#include <iterator>

using namespace gross;

// put here since lazy_edge_iterator is declared
// out-of-line
Graph::edge_iterator Graph::edge_begin() { return edge_iterator(this); }
Graph::edge_iterator Graph::edge_end() { return edge_iterator(this, true); }
size_t Graph::edge_size() { return std::distance(edge_begin(),
                                                 edge_end()); }

void Graph::InsertNode(Node* N) {
  Nodes.emplace_back(N);
}

void Graph::dumpGraphviz(std::ostream& OS) {
  boost::write_graphviz(OS, *this,
                        graph_vertex_prop_writer(*this),
                        graph_edge_prop_writer{},
                        graph_prop_writer{});
}
