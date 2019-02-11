#include "Parser.h"
#include "ParserTest.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"
#include <sstream>
#include <fstream>

using namespace gross;

TEST(ParserUnitTest, TestVarDecl) {
  std::stringstream SS;
  {
    // single declaration
    SS << "var foo;";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);
    EXPECT_TRUE(P.ParseVarDecl<IrOpcode::SrcVarDecl>());
  }
  SS.clear();
  {
    // multiple declarations
    SS << "var foo, bar;";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);
    EXPECT_TRUE(P.ParseVarDecl<IrOpcode::SrcVarDecl>());
  }
  SS.clear();
  {
    // single-dim array declaration
    SS << "array[94] foo;";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);

    std::vector<Node*> DeclNodes;
    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcArrayDecl>(&DeclNodes));
    ASSERT_EQ(DeclNodes.size(), 1);
    NodeProperties<IrOpcode::SrcArrayDecl> NP(DeclNodes.at(0));
    ASSERT_TRUE(NP);
    ASSERT_EQ(NP.dim_size(), 1);
    auto* Dim0 = NP.dim(0);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(Dim0)
              .as<int32_t>(G), 94);
  }
  SS.clear();
  {
    // multi-dims array declaration
    SS << "array[94][87] foo;";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);

    std::vector<Node*> DeclNodes;
    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcArrayDecl>(&DeclNodes));
    ASSERT_EQ(DeclNodes.size(), 1);
    NodeProperties<IrOpcode::SrcArrayDecl> NP(DeclNodes.at(0));
    ASSERT_TRUE(NP);
    ASSERT_EQ(NP.dim_size(), 2);
    Node *Dim0 = NP.dim(0),
         *Dim1 = NP.dim(1);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(Dim0)
              .as<int32_t>(G), 94);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(Dim1)
              .as<int32_t>(G), 87);
  }
}

TEST(ParserUnitTest, ParseBasicFuncDecl) {
  std::stringstream SS;
  {
    // only function header
    SS << "function func1;\n"
       << "{};\n";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);

    ASSERT_TRUE(P.ParseFuncDecl());
  }
  SS.clear();
  {
    // function header with some arguments
    SS << "function func2(arg1,arg2);\n"
       << "{};\n";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);

    ASSERT_TRUE(P.ParseFuncDecl());
  }
  SS.clear();
  {
    // empty function with some decls
    SS << "function func3(arg1,arg2);\n"
       << "var hello;\n"
       << "array[8][7] foo, bar;\n"
       << "{};\n";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);

    ASSERT_TRUE(P.ParseFuncDecl());
  }
}

TEST(ParserUnitTest, ParseCompoundFuncDecl) {
  std::stringstream SS;
  {
    SS << "function foo; {\n"
       << "  return 1 + 2 * 3\n"
       << "};";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);

    ASSERT_TRUE(P.ParseFuncDecl());

    std::ofstream OF("TestComplexFuncDecl1.dot");
    G.dumpGraphviz(OF);
  }
  SS.clear();
  {
    SS << "function foo(a,b,c); {\n"
       << "  return a + b * c\n"
       << "};";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);

    ASSERT_TRUE(P.ParseFuncDecl());

    std::ofstream OF("TestComplexFuncDecl1-2.dot");
    G.dumpGraphviz(OF);
  }
}
