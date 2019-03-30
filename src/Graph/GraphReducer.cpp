#include "gross/Graph/BGL.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Graph/GraphReducer.h"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/properties.hpp"
#include <vector>
#include <iostream>

using namespace gross;

struct GraphReducer::DFSVisitor
  : public boost::default_dfs_visitor {
  DFSVisitor(std::vector<Node*>& Preced,
             NodeMarker<GraphReducer::ReductionState>& M)
    : Precedence(Preced), Marker(M) {
    Precedence.clear();
  }

  void finish_vertex(Node* N, const SubGraph& G) {
    Precedence.push_back(N);
    Marker.Set(N, GraphReducer::ReductionState::OnStack);
  }

private:
  std::vector<Node*>& Precedence;
  NodeMarker<GraphReducer::ReductionState>& Marker;
};

GraphReducer::GraphReducer(Graph& graph, bool TrimGraph)
  : G(graph),
    DeadNode(NodeBuilder<IrOpcode::Dead>(&G).Build()),
    RSMarker(G, 4),
    DoTrimGraph(TrimGraph) {}

void GraphReducer::Replace(Node* N, Node* Replacement) {
  for(auto* Usr : N->users()) {
    Revisit(Usr);
  }
  N->ReplaceWith(Replacement);
  N->Kill(DeadNode);
  Recurse(Replacement);
}

void GraphReducer::Revisit(Node* N) {
  if(RSMarker.Get(N) == ReductionState::Visited) {
    RSMarker.Set(N, ReductionState::Revisit);
    RevisitStack.insert(RevisitStack.begin(), N);
  }
}

void GraphReducer::Push(Node* N) {
  RSMarker.Set(N, ReductionState::OnStack);
  ReductionStack.insert(ReductionStack.begin(), N);
}

void GraphReducer::Pop() {
  auto* TopNode = ReductionStack.front();
  ReductionStack.erase(ReductionStack.begin());
  RSMarker.Set(TopNode, ReductionState::Visited);
}

bool GraphReducer::Recurse(Node* N) {
  if(RSMarker.Get(N) > ReductionState::Revisit) return false;
  Push(N);
  return true;
}

void GraphReducer::DFSVisit(SubGraph& SG, NodeMarker<ReductionState>& Marker) {
  DFSVisitor Vis(ReductionStack, Marker);
  std::unordered_map<Node*,boost::default_color_type> ColorStorage;
  StubColorMap<decltype(ColorStorage), Node> ColorMap(ColorStorage);
  boost::depth_first_search(SG, Vis, std::move(ColorMap));
}

void GraphReducer::runOnFunctionGraph(SubGraph& SG,
                                      _detail::ReducerConcept* Reducer) {
  DFSVisit(SG, RSMarker);

  while(!ReductionStack.empty() || !RevisitStack.empty()) {
    while(!ReductionStack.empty()) {
      Node* N = ReductionStack.front();
      if(N->getOp() == IrOpcode::Dead) {
        Pop();
        continue;
      }

      auto RP = Reducer->Reduce(N);

      if(!RP.Changed()) {
        Pop();
        continue;
      }

      if(RP.Replacement() == N) {
        // in-place replacement, recurse on input
        bool IsRecursed = false;
        for(auto* Input : N->inputs()) {
          if(Input != N)
            IsRecursed |= Recurse(Input);
        }
        if(IsRecursed) continue;
      }

      Pop();

      if(RP.Replacement() != N) {
        Replace(N, RP.Replacement());
      } else {
        // revisit all the users
        for(auto* Usr : N->users()) {
          if(Usr != N) Revisit(Usr);
        }
      }
    }

    while(!RevisitStack.empty()) {
      Node* N = RevisitStack.front();
      RevisitStack.erase(RevisitStack.begin());

      if(RSMarker.Get(N) == ReductionState::Revisit) {
        Push(N);
      }
    }
  }
}

void GraphReducer::runImpl(_detail::ReducerConcept* Reducer) {
  for(auto& SG : G.subregions())
    runOnFunctionGraph(SG, Reducer);

  if(DoTrimGraph) {
    // remove nodes that are unreachable from any function
    NodeMarker<ReductionState> TrimMarker(G, 4);
    for(auto& SG : G.subregions()) {
      DFSVisit(SG, TrimMarker);
    }
    for(auto NI = G.node_begin(); NI != G.node_end();) {
      auto* N = Graph::GetNodeFromIt(NI);
      if(TrimMarker.Get(N) == ReductionState::Unvisited &&
         !NodeProperties<IrOpcode::VirtGlobalValues>(N) &&
         !G.IsGlobalVar(N)) {
        NI = G.RemoveNode(NI);
      } else {
        ++NI;
      }
    }

    // remove all deps to Dead node
    auto* DeadNode = NodeBuilder<IrOpcode::Dead>(&G).Build();
    std::vector<Node*> DeadUsrs;
    DeadUsrs.assign(DeadNode->value_users().begin(),
                    DeadNode->value_users().end());
    for(auto* N : DeadUsrs)
      N->removeValueInputAll(DeadNode);
    DeadUsrs.assign(DeadNode->effect_users().begin(),
                    DeadNode->effect_users().end());
    for(auto* N : DeadUsrs)
      N->removeEffectInputAll(DeadNode);
    DeadUsrs.assign(DeadNode->control_users().begin(),
                    DeadNode->control_users().end());
    for(auto* N : DeadUsrs)
      N->removeEffectInputAll(DeadNode);

  }
}
