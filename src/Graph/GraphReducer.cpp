#include "gross/Graph/BGL.h"
#include "gross/Graph/GraphReducer.h"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/properties.hpp"
#include <vector>
#include <iostream>

using namespace gross;

namespace {
using RSMarker = NodeMarker<ReductionState>;

template<class GraphT>
struct ReductionVisitor
  : public boost::default_dfs_visitor {
  explicit ReductionVisitor(std::vector<Node*>& Preced)
    : Precedence(Preced) {}

  void finish_vertex(Node* N, const GraphT& G) {
    Precedence.insert(Precedence.cbegin(), N);
    RSMarker::Set(N, ReductionState::OnStack);
  }

private:
  std::vector<Node*>& Precedence;
};

void runOnFunctionGraph(SubGraph& SG,
                        _detail::ReducerConcept* Reducer) {
  std::vector<Node*> ReductionStack, RevisitStack;
  ReductionVisitor<SubGraph> Vis(ReductionStack);
  std::unordered_map<Node*,boost::default_color_type> ColorStorage;
  StubColorMap<decltype(ColorStorage)> ColorMap(ColorStorage);
  boost::depth_first_search(SG, Vis, std::move(ColorMap));

  while(!ReductionStack.empty() || !RevisitStack.empty()) {
    while(!ReductionStack.empty()) {
      Node* N = ReductionStack.front();
      auto RP = Reducer->Reduce(N);

      ReductionStack.erase(ReductionStack.cbegin());
      if(!RP.Changed()) {
        RSMarker::Set(N, ReductionState::Visited);
      } else if(RP.ReplacementNode() == N) {
        // some inputs pending, push to revisit
        RSMarker::Set(N, ReductionState::Revisit);
        RevisitStack.insert(RevisitStack.cbegin(), N);
      } else {
        RSMarker::Set(N, ReductionState::Visited);
        Node* NewNode = RP.ReplacementNode();
        ReductionStack.insert(ReductionStack.cbegin(), NewNode);
        RSMarker::Set(NewNode, ReductionState::OnStack);
      }
    }

    while(!RevisitStack.empty()) {
      Node* N = RevisitStack.front();
      auto RP = Reducer->Reduce(N);

      RevisitStack.erase(RevisitStack.cbegin());
      if(!RP.Changed()) {
        RSMarker::Set(N, ReductionState::Visited);
      } else {
        auto* NewNode = RP.ReplacementNode();
        assert(NewNode != N && "Recursive revisit occur");
        RSMarker::Set(N, ReductionState::Visited);
        ReductionStack.insert(ReductionStack.cbegin(), NewNode);
        RSMarker::Set(NewNode, ReductionState::OnStack);
      }
    }
  }
}
} // end anonymous namespace

void gross::_detail::runReducerImpl(Graph& G,
                                    _detail::ReducerConcept* Reducer) {
  for(auto& SG : G.subregions())
    runOnFunctionGraph(SG, Reducer);
}
