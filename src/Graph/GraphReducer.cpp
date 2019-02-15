#include "gross/Graph/BGL.h"
#include "gross/Graph/GraphReducer.h"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/properties.hpp"
#include <vector>
#include <iostream>

using namespace gross;

namespace {
using RSMarkerTy = NodeMarker<ReductionState>;

template<class GraphT>
struct ReductionVisitor
  : public boost::default_dfs_visitor {
  explicit ReductionVisitor(std::vector<Node*>& Preced,
                            RSMarkerTy& M)
    : Precedence(Preced), Marker(M) {}

  void finish_vertex(Node* N, const GraphT& G) {
    Precedence.push_back(N);
    Marker.Set(N, ReductionState::OnStack);
  }

private:
  std::vector<Node*>& Precedence;
  RSMarkerTy& Marker;
};

void runOnFunctionGraph(SubGraph& SG,
                        NodeMarker<ReductionState>& RSMarker,
                        _detail::ReducerConcept* Reducer) {
  std::vector<Node*> ReductionStack, RevisitStack;
  ReductionVisitor<SubGraph> Vis(ReductionStack, RSMarker);
  std::unordered_map<Node*,boost::default_color_type> ColorStorage;
  StubColorMap<decltype(ColorStorage)> ColorMap(ColorStorage);
  boost::depth_first_search(SG, Vis, std::move(ColorMap));

  auto revisit = [&RSMarker,&RevisitStack](Node* N) {
    RSMarker.Set(N, ReductionState::Revisit);
    RevisitStack.insert(RevisitStack.cbegin(), N);
  };

  while(!ReductionStack.empty() || !RevisitStack.empty()) {
    while(!ReductionStack.empty()) {
      Node* N = ReductionStack.front();
      auto RP = Reducer->Reduce(N);

      ReductionStack.erase(ReductionStack.cbegin());
      if(!RP.Changed()) {
        RSMarker.Set(N, ReductionState::Visited);
      } else if(RP.ReplacementNode() == N) {
        if(RP.Revisit()) {
          // some inputs pending, push to revisit
          revisit(N);
        } else {
          // in-place replacement, revisit all the users
          for(auto* Usr : N->users()) {
            if(RSMarker.Get(Usr) == ReductionState::Visited)
              revisit(Usr);
          }
        }
      } else {
        RSMarker.Set(N, ReductionState::Visited);

        Node* NewNode = RP.ReplacementNode();
        ReductionStack.insert(ReductionStack.cbegin(), NewNode);
        RSMarker.Set(NewNode, ReductionState::OnStack);
      }
    }

    while(!RevisitStack.empty()) {
      Node* N = RevisitStack.front();
      RevisitStack.erase(RevisitStack.cbegin());

      ReductionStack.insert(ReductionStack.cbegin(), N);
      RSMarker.Set(N, ReductionState::OnStack);
    }
  }
}
} // end anonymous namespace

void gross::_detail::runReducerImpl(Graph& G,
                                    NodeMarker<ReductionState>& RSMarker,
                                    _detail::ReducerConcept* Reducer) {
  for(auto& SG : G.subregions())
    runOnFunctionGraph(SG, RSMarker, Reducer);
}
