#include "Parser.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"
#include <sstream>

using namespace gross;

TEST(ParserTest, TestVarDecl) {
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
  SS.clear();
  {
    // single-dim array declaration
    SS << "array[94] foo;";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    P.NewSymScope();

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
    P.NewSymScope();

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

