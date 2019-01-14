#include "Parser.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
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

TEST(ParserTest, TestSimpleTerm) {
  std::stringstream SS;
  {
    // single term
    SS << "94 * 87";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    auto* TermNode = P.ParseTerm();
    ASSERT_TRUE(NodeProperties<IrOpcode::BinMul>(TermNode));

    Node *LHS = TermNode->getValueInput(0),
         *RHS = TermNode->getValueInput(1);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(LHS)
              .as<int32_t>(G), 94);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 87);
  }
  SS.clear();
  {
    // multiple terms
    SS << "94 * 87 / 43 * 7";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    auto* TermNode = P.ParseTerm();
    ASSERT_TRUE(NodeProperties<IrOpcode::BinMul>(TermNode));

    Node *LHS = nullptr, *RHS = nullptr;
    // SubTree - * - 7
    LHS = TermNode->getValueInput(0); // SubTree
    RHS = TermNode->getValueInput(1); // 7
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 7);
    ASSERT_TRUE(NodeProperties<IrOpcode::BinDiv>(LHS));
    TermNode = LHS;
    // SubTree - / - 43
    LHS = TermNode->getValueInput(0); // SubTree
    RHS = TermNode->getValueInput(1); // 43
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 43);
    ASSERT_TRUE(NodeProperties<IrOpcode::BinMul>(LHS));
    TermNode = LHS;
    // 94 - * - 87
    LHS = TermNode->getValueInput(0); // 94
    RHS = TermNode->getValueInput(1); // 87
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(LHS)
              .as<int32_t>(G), 94);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 87);
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
    auto* ExprNode = P.ParseExpr();
    ASSERT_TRUE(NodeProperties<IrOpcode::BinAdd>(ExprNode));

    Node *LHS = ExprNode->getValueInput(0),
         *RHS = ExprNode->getValueInput(1);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(LHS)
              .as<int32_t>(G), 94);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 87);
  }
  SS.clear();
  {
    // multiple exprs
    SS << "94 + 87 - 43 + 7";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    auto* ExprNode = P.ParseExpr();
    ASSERT_TRUE(NodeProperties<IrOpcode::BinAdd>(ExprNode));

    Node *LHS = nullptr, *RHS = nullptr;
    // SubTree - + - 7
    LHS = ExprNode->getValueInput(0); // SubTree
    RHS = ExprNode->getValueInput(1); // 7
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 7);
    ASSERT_TRUE(NodeProperties<IrOpcode::BinSub>(LHS));
    ExprNode = LHS;
    // SubTree - (-) - 43
    LHS = ExprNode->getValueInput(0); // SubTree
    RHS = ExprNode->getValueInput(1); // 43
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 43);
    ASSERT_TRUE(NodeProperties<IrOpcode::BinAdd>(LHS));
    ExprNode = LHS;
    // 94 - + - 87
    LHS = ExprNode->getValueInput(0); // 94
    RHS = ExprNode->getValueInput(1); // 87
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(LHS)
              .as<int32_t>(G), 94);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 87);
  }
  SS.clear();
  {
    // compound exprs
    SS << "94 + 87 * 43 - 7";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    auto* ExprNode = P.ParseExpr();
    ASSERT_TRUE(NodeProperties<IrOpcode::BinSub>(ExprNode));

    Node *LHS = nullptr, *RHS = nullptr;
    // SubTree - (-) - 7
    LHS = ExprNode->getValueInput(0); // SubTree
    RHS = ExprNode->getValueInput(1); // 7
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 7);
    ASSERT_TRUE(NodeProperties<IrOpcode::BinAdd>(LHS));
    ExprNode = LHS;
    // 94 - + - SubTree
    LHS = ExprNode->getValueInput(0); // 94
    RHS = ExprNode->getValueInput(1); // SubTree
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(LHS)
              .as<int32_t>(G), 94);
    ASSERT_TRUE(NodeProperties<IrOpcode::BinMul>(RHS));
    ExprNode = RHS;
    // 87 - * - 43
    LHS = ExprNode->getValueInput(0); // 87
    RHS = ExprNode->getValueInput(1); // 43
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(LHS)
              .as<int32_t>(G), 87);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 43);
  }
  SS.clear();
  {
    // compound exprs
    SS << "94 + 87 * (43 - 7) + 5 + 9 / 2";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    auto* ExprNode = P.ParseExpr();
    ASSERT_TRUE(NodeProperties<IrOpcode::BinAdd>(ExprNode));

    Node *LHS = nullptr, *RHS = nullptr;
    // SubTree1 - + - SubTree2
    LHS = ExprNode->getValueInput(0); // SubTree1
    RHS = ExprNode->getValueInput(1); // SubTree2
    ASSERT_TRUE(NodeProperties<IrOpcode::BinDiv>(RHS));
    ExprNode = RHS;
    // 9 - / - 2
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(
                ExprNode->getValueInput(0)
              ).as<int32_t>(G), 9);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(
                ExprNode->getValueInput(1)
              ).as<int32_t>(G), 2);
    ASSERT_TRUE(NodeProperties<IrOpcode::BinAdd>(LHS));
    ExprNode = LHS;
    // SubTree - + - 5
    LHS = ExprNode->getValueInput(0); // SubTree
    RHS = ExprNode->getValueInput(1); // 5
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 5);
    ASSERT_TRUE(NodeProperties<IrOpcode::BinAdd>(LHS));
    ExprNode = LHS;
    // 94 - + - SubTree
    LHS = ExprNode->getValueInput(0); // 94
    RHS = ExprNode->getValueInput(1); // SubTree
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(LHS)
              .as<int32_t>(G), 94);
    ASSERT_TRUE(NodeProperties<IrOpcode::BinMul>(RHS));
    ExprNode = RHS;
    // 87 - * - SubTree
    LHS = ExprNode->getValueInput(0); // 87
    RHS = ExprNode->getValueInput(1); // SubTree
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(LHS)
              .as<int32_t>(G), 87);
    ASSERT_TRUE(NodeProperties<IrOpcode::BinSub>(RHS));
    ExprNode = RHS;
    // 43 - (-) - 7
    LHS = ExprNode->getValueInput(0); // 43
    RHS = ExprNode->getValueInput(1); // 7
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(LHS)
              .as<int32_t>(G), 43);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 7);
  }
}
