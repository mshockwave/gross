#include "gross/Graph/Reductions/DeadCodeElimination.h"
#include "gross/Graph/Node.h"
#include "gross/Graph/NodeUtils.h"

namespace gross {
namespace _internal {
enum VisitState : uint8_t {
  K_REACHABLE,
  K_UNREACHABLE
};
} // end namespace _internal
} // end namespace gross

using namespace gross;

void DCEReducer::EliminateUnreachable(Graph& G) {
  // remove (non-global) nodes that are not included
  // in any function
  using VisMarker = NodeMarker<_internal::VisitState>;
  for(auto I = G.node_begin(), E = G.node_end();
      I != E; ++I) {
    auto* N = Graph::GetNodeFromIt(I);
    if(NodeProperties<IrOpcode::VirtGlobalValues>(N))
      VisMarker::Set(N, _internal::K_REACHABLE);
    else
      VisMarker::Set(N, _internal::K_UNREACHABLE);
  }

  for(auto& SR : G.subregions()) {
    for(auto I = SR.node_begin(), E = SR.node_end();
        I != E; ++I) {
      VisMarker::Set(SubGraph::GetNodeFromIt(I),
                     _internal::K_REACHABLE);
    }
  }

  for(auto I = G.node_cbegin(); I != G.node_cend();) {
    auto* N = const_cast<Node*>(Graph::GetNodeFromIt(I));
    if(VisMarker::Get(N) == _internal::K_UNREACHABLE) {
      I = G.RemoveNode(I);
    } else {
      ++I;
    }
  }
}

void DCEReducer::DeadControlElimination(Graph& G) {
  // TODO
}

void DCEReducer::Reduce(Graph& G) {
  EliminateUnreachable(G);
}
