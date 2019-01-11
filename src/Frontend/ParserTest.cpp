#include "Parser.h"
#include "gross/Graph/Graph.h"
#include "gtest/gtest.h"
#include <sstream>

using namespace gross;

TEST(ParserTest, TestScalarVarDecl) {
  std::stringstream SS;
  {
    // single declaration
    SS << "var foo;";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    P.NewSymScope();
    EXPECT_TRUE(P.ParseVarDecl<IrOpcode::SrcVarDecl>());
  }
  SS.clear();
  {
    // multiple declarations
    SS << "var foo, bar;";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    P.NewSymScope();
    EXPECT_TRUE(P.ParseVarDecl<IrOpcode::SrcVarDecl>());
  }
}

// TODO: Test factor when its rules are completed

// TODO: Check shape of the generated graph
TEST(ParserTest, TestSimpleTerm) {
  std::stringstream SS;
  {
    // single term
    SS << "94 * 87";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    EXPECT_TRUE(P.ParseTerm());
  }
  SS.clear();
  {
    // multiple terms
    SS << "94 * 87 / 43 * 7";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    EXPECT_TRUE(P.ParseTerm());
  }
}

TEST(ParserTest, TestSimpleExpr) {
  std::stringstream SS;
  {
    // single expr
    SS << "94 + 87";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    EXPECT_TRUE(P.ParseExpr());
  }
  SS.clear();
  {
    // multiple exprs
    SS << "94 + 87 - 43 + 7";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    EXPECT_TRUE(P.ParseExpr());
  }
  SS.clear();
  {
    // compound exprs
    SS << "94 + 87 * 43 - 7";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    EXPECT_TRUE(P.ParseExpr());
  }
  SS.clear();
  {
    // compound exprs
    SS << "94 + 87 * (43 - 7) + 5 + 9 / 2";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    EXPECT_TRUE(P.ParseExpr());
  }
}
