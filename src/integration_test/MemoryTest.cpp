#include "Frontend/Parser.h"
#include "gross/Graph/Reductions/Peephole.h"
#include "gross/Graph/Reductions/ValuePromotion.h"
#include "gross/Graph/Reductions/MemoryLegalize.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(MemoryIntegrateTest, TestBasicControlStructure) {
  std::ifstream IF("memory1.txt");

  Graph G;
  Parser P(IF, G);
  (void) P.getLexer().getNextToken();
  P.NewSymScope(); // global scope

  ASSERT_TRUE(P.ParseFuncDecl());
  {
    std::ofstream OF("TestMemoryBasicCtrlStructure.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<ValuePromotion>(G);
  {
    std::ofstream OF("TestMemoryBasicCtrlStructure.mem2reg.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<MemoryLegalize>(G);
  {
    std::ofstream OF("TestMemoryBasicCtrlStructure.mem2reg.legal.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<PeepholeReducer>(G);
  {
    std::ofstream OF("TestMemoryBasicCtrlStructure.mem2reg.legal.peephole.dot");
    G.dumpGraphviz(OF);
  }
}

TEST(MemoryIntegrateTest, TestSimpleLoop) {
  std::ifstream IF("memory2.txt");

  Graph G;
  Parser P(IF, G);
  (void) P.getLexer().getNextToken();
  P.NewSymScope(); // global scope

  ASSERT_TRUE(P.ParseFuncDecl());
  {
    std::ofstream OF("TestMemorySimpleLoop.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<ValuePromotion>(G);
  {
    std::ofstream OF("TestMemorySimpleLoop.mem2reg.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<MemoryLegalize>(G);
  {
    std::ofstream OF("TestMemorySimpleLoop.mem2reg.legal.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<PeepholeReducer>(G);
  {
    std::ofstream OF("TestMemorySimpleLoop.mem2reg.legal.peephole.dot");
    G.dumpGraphviz(OF);
  }
}
