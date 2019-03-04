#include "Frontend/Parser.h"
#include "gross/Graph/Reductions/Peephole.h"
#include "gross/Graph/Reductions/ValuePromotion.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(ValueAssignIntegrateTest, TestBasicControlStructure) {
  std::ifstream IF("value_assignment1.txt");

  Graph G;
  Parser P(IF, G);
  (void) P.getLexer().getNextToken();
  P.NewSymScope(); // global scope

  ASSERT_TRUE(P.ParseFuncDecl());
  {
    std::ofstream OF("TestBasicCtrlStructure.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<ValuePromotion>(G);
  {
    std::ofstream OF("TestBasicCtrlStructure.mem2reg.dot");
    G.dumpGraphviz(OF);
  }
}

TEST(ValueAssignIntegrateTest, TestSimpleLoop) {
  std::ifstream IF("value_assignment2.txt");

  Graph G;
  Parser P(IF, G);
  (void) P.getLexer().getNextToken();
  P.NewSymScope(); // global scope

  ASSERT_TRUE(P.ParseFuncDecl());
  {
    std::ofstream OF("TestSimpleLoop.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<ValuePromotion>(G);
  {
    std::ofstream OF("TestSimpleLoop.mem2reg.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<PeepholeReducer>(G);
  {
    std::ofstream OF("TestSimpleLoop.mem2reg.peephole.dot");
    G.dumpGraphviz(OF);
  }
}

TEST(ValueAssignIntegrateTest, TestComplexCtrlStructure) {
  std::ifstream IF("value_assignment3.txt");

  Graph G;
  Parser P(IF, G);
  (void) P.getLexer().getNextToken();
  P.NewSymScope(); // global scope

  ASSERT_TRUE(P.ParseFuncDecl());
  {
    std::ofstream OF("TestComplexCtrlStructure.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<ValuePromotion>(G);
  {
    std::ofstream OF("TestComplexCtrlStructure.mem2reg.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<PeepholeReducer>(G);
  {
    std::ofstream OF("TestComplexCtrlStructure.mem2reg.peephole.dot");
    G.dumpGraphviz(OF);
  }
}
