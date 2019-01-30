#include "gross/Graph/BGL.h"
#include "gross/Graph/GraphReducer.h"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/properties.hpp"
#include <vector>
#include <iostream>

using namespace gross;

namespace {
enum class ReductionState : uint8_t {
  Unvisited,
  Revisit,
  Visited,
};

// FIXME: replace me
template<class T>
struct MyVisitor : public boost::default_dfs_visitor {
  MyVisitor(T& C) : Container(C) {}

  void finish_vertex(Node* N, const Graph& G) {
    Container.push_back(N);
  }

private:
  T& Container;
};
} // end anonymous namespace

void gross::_detail::runReducerImpl(Graph& G,
                                    _detail::ReducerConcept* Reducer) {
  std::vector<Node*> Trace;
  MyVisitor<decltype(Trace)> vis(Trace);
  std::unordered_map<Node*,boost::default_color_type> storage;
  StubColorMap<decltype(storage)> color_map(storage);

  boost::depth_first_search(G, vis, color_map);

  // FIXME: remove me
  graph_vertex_prop_writer Writer(G);
  for(auto* N : Trace) {
    Writer(std::cout << std::endl, N);
  }
}
