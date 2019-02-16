#include "gross/Graph/BGL.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Graph/GraphReducer.h"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/properties.hpp"
#include <vector>
#include <iostream>

using namespace gross;

struct GraphReducer::ReductionVisitor
  : public boost::default_dfs_visitor {
  explicit ReductionVisitor(std::vector<Node*>& Preced,
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

GraphReducer::GraphReducer(Graph& graph)
  : G(graph),
    DeadNode(NodeBuilder<IrOpcode::Dead>(&G).Build()),
    RSMarker(G, 4) {}

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
    RevisitStack.insert(RevisitStack.cbegin(), N);
  }
}

void GraphReducer::Push(Node* N) {
  RSMarker.Set(N, ReductionState::OnStack);
  ReductionStack.insert(ReductionStack.cbegin(), N);
}

void GraphReducer::Pop() {
  auto* TopNode = ReductionStack.front();
  ReductionStack.erase(ReductionStack.cbegin());
  RSMarker.Set(TopNode, ReductionState::Visited);
}

bool GraphReducer::Recurse(Node* N) {
  if(RSMarker.Get(N) > ReductionState::Revisit) return false;
  Push(N);
  return true;
}

void GraphReducer::runOnFunctionGraph(SubGraph& SG,
                                      _detail::ReducerConcept* Reducer) {
  ReductionVisitor Vis(ReductionStack, RSMarker);
  std::unordered_map<Node*,boost::default_color_type> ColorStorage;
  StubColorMap<decltype(ColorStorage)> ColorMap(ColorStorage);
  boost::depth_first_search(SG, Vis, std::move(ColorMap));

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
      RevisitStack.erase(RevisitStack.cbegin());

      if(RSMarker.Get(N) == ReductionState::Revisit) {
        Push(N);
      }
    }
  }
}

void GraphReducer::runImpl(_detail::ReducerConcept* Reducer) {
  for(auto& SG : G.subregions())
    runOnFunctionGraph(SG, Reducer);
}
