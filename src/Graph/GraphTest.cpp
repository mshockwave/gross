#include "gross/Graph/Graph.h"
#include "gtest/gtest.h"
#include <sstream>

using namespace gross;

TEST(GraphTest, TestGraphvizConcept) {
  // this test is more like a 'compile test'
  // since boost::write_graphviz use lots of
  // meta programming stuff to verify correctness
  Graph G;
  std::stringstream SS;
  G.dumpGraphviz(SS);
}
