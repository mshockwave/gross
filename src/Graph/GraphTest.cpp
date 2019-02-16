#include "gross/Graph/Graph.h"
#include "gross/Graph/GraphReducer.h"
#include "gtest/gtest.h"
#include <sstream>

using namespace gross;

TEST(GraphUnitTest, TestGraphvizConcept) {
  // this test is more like a 'compile test'
  // since boost::write_graphviz use lots of
  // meta programming stuff to verify correctness
  Graph G;
  std::stringstream SS;
  G.dumpGraphviz(SS);
}

TEST(GraphUnitTest, TestGraphReducerConcept) {
  Graph G;
  struct DummyReducer {
    GraphReduction Reduce(Node* N) {
      return GraphReduction();
    }

    static constexpr
    const char* name() { return "dummy-reducer"; }
  };

  GraphReducer::Run<DummyReducer>(G);

  struct DummyAdvanceReducer : public GraphEditor {
    DummyAdvanceReducer(GraphEditor::Interface* editor)
      : GraphEditor(editor) {}

    GraphReduction Reduce(Node* N) {
      return NoChange();
    }

    static constexpr
    const char* name() { return "dummy-advance-reducer"; }
  };

  GraphReducer::RunWithEditor<DummyAdvanceReducer>(G);
}
