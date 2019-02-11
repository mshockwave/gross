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

TEST(GraphUnitTest, TestBasicGraphReducer) {
  Graph G;
  struct DummyReducer {
    GraphReduction Reduce(Node* N) {
      return GraphReduction();
    }

    static constexpr
    const char* name() { return "dummy-reducer"; }
  };

  RunReducer<DummyReducer>(G);
}
