#ifndef GROSS_GRAPH_GRAPHREDUCER_H
#define GROSS_GRAPH_GRAPHREDUCER_H
#include "gross/Graph/Graph.h"
#include "gross/Graph/Node.h"
#include "gross/Graph/NodeMarker.h"
#include <utility>

namespace gross {
struct GraphReduction {
  explicit GraphReduction(Node* N = nullptr)
    : ReplacementNode(N) {}

  Node* Replacement() const { return ReplacementNode; }
  bool Changed() const { return Replacement() != nullptr; }

private:
  Node* ReplacementNode;
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
} // end namespace _detail

/// Mixin for reducer to modify other nodes
struct GraphEditor {
  struct Interface {
    virtual ~Interface() {}

    virtual void Replace(Node* N, Node* Replacement) = 0;
    virtual void Revisit(Node* N) = 0;

    virtual Graph& GetGraph() = 0;
  };

protected:
  Interface* Editor;

  explicit GraphEditor(Interface* editor) : Editor(editor) {}

  // some utility for subclass
  Graph& GetGraph() {
    return Editor->GetGraph();
  }
  void Replace(Node* N, Node* Replacement) {
    Editor->Replace(N, Replacement);
  }
  void Revisit(Node* N) {
    Editor->Revisit(N);
  }
  // for single node reduction
  static GraphReduction Replace(Node* N) {
    return GraphReduction(N);
  }
  static GraphReduction NoChange() {
    return GraphReduction();
  }
};

/// The primary graph reduction algorithm implement
class GraphReducer : public GraphEditor::Interface {
  enum class ReductionState : uint8_t {
    Unvisited = 0, // Default state
    Revisit,       // Revisit later
    OnStack,       // Observed and on stack
    Visited        // Finished
  };

  struct DFSVisitor;

  Graph& G;
  Node* DeadNode;

  // visiting stacks
  std::vector<Node*> ReductionStack, RevisitStack;

  // visiting marker
  NodeMarker<ReductionState> RSMarker;

  bool DoTrimGraph;

  GraphReducer(Graph& graph, bool TrimGraph = true);

  // implement GraphEditor::Interface
  void Replace(Node* N, Node* Replacement) override;
  void Revisit(Node* N) override;
  Graph& GetGraph() override { return G; }

  void Push(Node* N);
  void Pop();
  // conditionally revisit
  bool Recurse(Node* N);

  void DFSVisit(SubGraph& SG, NodeMarker<ReductionState>& Marker);

  void runImpl(_detail::ReducerConcept* R);
  void runOnFunctionGraph(SubGraph& SG, _detail::ReducerConcept* R);

public:
  template<class ReducerT, class... Args>
  static void Run(Graph& G, Args &&... CtorArgs) {
    GraphReducer GR(G);
    _detail::ReducerModel<ReducerT, Args...> RM(
      std::forward<Args>(CtorArgs)...
    );
    GR.runImpl(&RM);
  }

  template<class ReducerT, class... Args>
  static void RunWithEditor(Graph& G, Args &&...CtorArgs) {
    GraphReducer GR(G);
    _detail::ReducerModel<ReducerT, GraphEditor::Interface*, Args...> RM(
      &GR, // first argument must be GraphEditor::Interface*
      std::forward<Args>(CtorArgs)...
    );
    GR.runImpl(&RM);
  }
};
} // end namespace gross
#endif
