#include "gross/Graph/Graph.h"
#include "gross/Graph/BGL.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Graph/NodeMarker.h"
#include "boost/graph/graphviz.hpp"
#include <algorithm>
#include <iterator>

using namespace gross;

// put here since lazy_edge_iterator is declared
// out-of-line
Graph::edge_iterator Graph::edge_begin() { return edge_iterator(this); }
Graph::edge_iterator Graph::edge_end() { return edge_iterator(this, true); }
size_t Graph::edge_size() { return std::distance(edge_begin(),
                                                 edge_end()); }

SubGraph::edge_iterator SubGraph::edge_begin() { return edge_iterator(this); }
SubGraph::edge_iterator SubGraph::edge_end() {
  return edge_iterator(this, true);
}
size_t SubGraph::edge_size() { return std::distance(edge_begin(),
                                                    edge_end()); }

void Graph::MarkGlobalVar(Node* N) {
  assert(N->getOp() == IrOpcode::SrcVarDecl ||
         N->getOp() == IrOpcode::SrcArrayDecl ||
         N->getOp() == IrOpcode::Alloca);
  GlobalVariables.insert(N);
}

void Graph::ReplaceGlobalVar(Node* Old, Node* New) {
  if(IsGlobalVar(Old)) {
    GlobalVariables.erase(Old);
    MarkGlobalVar(New);
  }
}

void Graph::InsertNode(Node* N) {
  Nodes.emplace_back(N);
  if(NodeIdxMarker)
    NodeIdxMarker->Set(N, NodeIdxCounter++);
}

typename Graph::node_iterator
Graph::RemoveNode(typename Graph::node_iterator NI) {
  auto* N = NI->get();
  if(!N->IsDead()) {
    auto* DeadNode = NodeBuilder<IrOpcode::Dead>(this).Build();
    N->Kill(DeadNode);
  }
  // unlink it with DeadNode
  N->removeValueInputAll(DeadNode);
  N->removeEffectInputAll(DeadNode);
  N->removeControlInputAll(DeadNode);
  return Nodes.erase(NI);
}

void Graph::AddSubRegion(const SubGraph& SG) {
  SubRegions.push_back(SG);
}
void Graph::AddSubRegion(SubGraph&& SG) {
  SubRegions.push_back(SG);
}

void Graph::dumpGraphviz(std::ostream& OS) {
  boost::write_graphviz(OS, *this,
                        graph_vertex_prop_writer(*this),
                        graph_edge_prop_writer{},
                        graph_prop_writer{});
}

size_t SubGraph::node_size() const {
  return std::distance(node_cbegin(), node_cend());
}
