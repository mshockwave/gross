#ifndef GROSS_GRAPH_GRAPHREDUCER_H
#define GROSS_GRAPH_GRAPHREDUCER_H
#include "gross/Graph/Graph.h"
#include "gross/Graph/Node.h"
#include "gross/Graph/NodeMarker.h"
#include <utility>

namespace gross {
struct GraphReduction {
  explicit GraphReduction(Node* N = nullptr, bool RV = false)
    : Replacement(N),
      NeedRevisit(RV) {}

  Node* ReplacementNode() const { return Replacement; }
  bool Changed() const { return Replacement != nullptr; }
  bool Revisit() const { return NeedRevisit; }

private:
  Node* Replacement;
  bool NeedRevisit;
};
// some utilities
namespace graph_reduction {
inline GraphReduction NoChange() { return GraphReduction(nullptr); }
inline GraphReduction Replace(Node* RN) {
  return GraphReduction(RN);
}
inline GraphReduction Revisit(Node* N) {
  return GraphReduction(N, true);
}
} // end namespace graph_reduction

enum class ReductionState : uint8_t {
  OnStack,   // Observed and on stack
  Revisit,   // Revisit later since some inputs are not visited
  Visited    // Finished
};

// template<class T>
// concept ReducerConcept = requires(T& R, Node* N) {
//  { R::name() } -> const char*;
//  { R.Reduce(N) } -> GraphReduction;
// };
//
// template<class T>
// concept GlobalReducerConcept = requires(T& R, Graph& G) {
//  { R::name() } -> const char*;
//  { R.Reduce(G) } -> void;
// };

namespace _detail {
// abstract base class used to dispatch
// polymorphically over reducer objects
struct ReducerConcept {
  virtual const char* name() const = 0;

  virtual GraphReduction Reduce(Node* N) = 0;
};
struct GlobalReducerConcept {
  virtual const char* name() const = 0;

  virtual void Reduce(Graph& G) = 0;
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
template<class ReducerT, class... CtorArgs>
struct GlobalReducerModel : public GlobalReducerConcept {
  GlobalReducerModel(CtorArgs &&... args)
    : Reducer(std::forward<CtorArgs>(args)...) {}

  void Reduce(Graph& G) override { Reducer.Reduce(G); }

  const char* name() const override { return ReducerT::name(); }

private:
  ReducerT Reducer;
};

void runReducerImpl(Graph& G, NodeMarker<ReductionState>& RSMarker,
                    ReducerConcept* Reducer);
} // end namespace _detail

template<class ReducerT, class... Args>
void RunReducer(Graph& G, Args &&... CtorArgs) {
  _detail::ReducerModel<ReducerT, Args...> RM(
    std::forward<Args>(CtorArgs)...
  );
  NodeMarker<ReductionState> RSMarker(G, 3);
  _detail::runReducerImpl(G, RSMarker, &RM);
}

template<class ReducerT, class... Args>
void RunGlobalReducer(Graph& G, Args &&... CtorArgs) {
  _detail::GlobalReducerModel<ReducerT, Args...> RM(
    std::forward<Args>(CtorArgs)...
  );
  RM.Reduce(G);
}
} // end namespace gross
#endif
