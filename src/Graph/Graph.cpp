#include "gross/Graph/Graph.h"
#include "gross/Graph/BGL.h"
#include "boost/graph/graphviz.hpp"

using namespace gross;

void Graph::InsertNode(Node* N) {
  Nodes.emplace_back(N);
}

void Graph::dumpGraphviz(std::ostream& OS) {
  // collect edges now
  Edges.clear();
  for(auto& NodePtr : Nodes) {
    Node* N = NodePtr.get();
    for(size_t i = 0, Num = N->getNumValueInput(); i < Num; ++i) {
      Node* Dest = N->getValueInput(i);
      Edges.insert(Use(N, Dest, Use::K_VALUE));
    }
    for(size_t i = 0, Num = N->getNumControlInput(); i < Num; ++i) {
      Node* Dest = N->getControlInput(i);
      Edges.insert(Use(N, Dest, Use::K_CONTROL));
    }
    for(size_t i = 0, Num = N->getNumEffectInput(); i < Num; ++i) {
      Node* Dest = N->getEffectInput(i);
      Edges.insert(Use(N, Dest, Use::K_EFFECT));
    }
  }
  boost::write_graphviz(OS, *this,
                        graph_vertex_prop_writer(*this),
                        graph_edge_prop_writer{},
                        graph_prop_writer{});
}
