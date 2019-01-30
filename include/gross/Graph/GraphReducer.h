#ifndef GROSS_GRAPH_GRAPHREDUCER_H
#define GROSS_GRAPH_GRAPHREDUCER_H
#include "gross/Graph/Graph.h"
#include "gross/Graph/Node.h"
#include <utility>

namespace gross {
struct GraphReduction {
  explicit GraphReduction(Node* N = nullptr)
    : Replacement(N) {}

  Node* ReplacementNode() const { return Replacement; }
  bool Changed() const { return Replacement != nullptr; }

private:
  Node* Replacement;
};

// template<class T>
// concept ReducerConcept = requires(T& R, Node* N) {
//  { R::name() } -> const char*;
//  { R.Reduce(N) } -> GraphReduction;
// };

namespace _detail {
// abstract base class used to dispatch
// polymorphically over reducer objects
struct ReducerConcept {
  virtual const char* name() const = 0;

  virtual GraphReduction Reduce(Node* N) = 0;
};

// a template wrapper used to implement the polymorphic API
template<class ReducerT, class... CtorArgs>
struct ReducerModel : public ReducerConcept {
  ReducerModel(CtorArgs &&... args)
    : Reducer(std::forward<CtorArgs>(args)...) {}

  GraphReduction Reduce(Node* N) override {
    return Reducer.Reduce(N);
  }

  const char* name() const override { return ReducerT::name(); }

private:
  ReducerT Reducer;
};

// Take Reducer's ownership
void runReducerImpl(Graph& G, ReducerConcept* Reducer);
} // end namespace _detail

template<class ReducerT, class... Args>
void RunReducer(Graph& G, Args &&... CtorArgs) {
  _detail::ReducerModel<ReducerT> RM(CtorArgs...);
  _detail::runReducerImpl(G, &RM);
}
} // end namespace gross
#endif
