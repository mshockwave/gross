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

TEST(ParserTest, TestRelation) {
  std::stringstream SS;
  {
    // trivial
    SS << "94 > 87";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    auto* RelNode = P.ParseRelation();
    ASSERT_TRUE(NodeProperties<IrOpcode::BinGt>(RelNode));
    Node *LHS = RelNode->getValueInput(0),
         *RHS = RelNode->getValueInput(1);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(LHS)
              .as<int32_t>(G), 94);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 87);
  }
  SS.clear();
  {
    SS << "94 <= 87";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    auto* RelNode = P.ParseRelation();
    ASSERT_TRUE(NodeProperties<IrOpcode::BinLe>(RelNode));
    Node *LHS = RelNode->getValueInput(0),
         *RHS = RelNode->getValueInput(1);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(LHS)
              .as<int32_t>(G), 94);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(RHS)
              .as<int32_t>(G), 87);
  }
  SS.clear();
  {
    // compound
    SS << "94 + 87 != 49 / 78";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    auto* RelNode = P.ParseRelation();
    ASSERT_TRUE(NodeProperties<IrOpcode::BinNe>(RelNode));
    Node *LHS = RelNode->getValueInput(0),
         *RHS = RelNode->getValueInput(1);
    EXPECT_TRUE(NodeProperties<IrOpcode::BinAdd>(LHS));
    EXPECT_TRUE(NodeProperties<IrOpcode::BinDiv>(RHS));
  }
}

TEST(ParserTest, TestAssignment) {
  std::stringstream SS;
  {
    // single assignment
    SS << "var megumin;"
       << "let megumin <- 2";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    P.NewSymScope();
    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcVarDecl>());

    Node* Assign = P.ParseAssignment();
    NodeProperties<IrOpcode::SrcAssignStmt> NPA(Assign);
    ASSERT_TRUE(NPA);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(NPA.source())
              .as<int32_t>(G), 2);
    EXPECT_TRUE(NodeProperties<IrOpcode::SrcVarAccess>(NPA.dest()));
  }
  SS.clear();
  {
    // multiple assignments
    SS << "var megumin;"
       << "let megumin <- 2\n"
       << "let megumin <- 87";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    P.NewSymScope();
    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcVarDecl>());

    Node* Assign1 = P.ParseAssignment();
    NodeProperties<IrOpcode::SrcAssignStmt> NPA1(Assign1);
    ASSERT_TRUE(NPA1);
    Node* Assign2 = P.ParseAssignment();
    NodeProperties<IrOpcode::SrcAssignStmt> NPA2(Assign2);
    ASSERT_TRUE(NPA2);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(NPA2.source())
              .as<int32_t>(G), 87);
    auto* Access2 = NPA2.dest();
    NodeProperties<IrOpcode::SrcVarAccess> NPAC2(Access2);
    ASSERT_TRUE(NPAC2);
    EXPECT_EQ(NPAC2.effect_dependency(), Assign1);
  }
  SS.clear();
  {
    // single array assignment
    SS << "array[94][87] megumin;"
       << "let megumin[1][5] <- 2";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    P.NewSymScope();
    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcArrayDecl>());

    Node* Assign = P.ParseAssignment();

    NodeProperties<IrOpcode::SrcAssignStmt> NPA(Assign);
    ASSERT_TRUE(NPA);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(NPA.source())
              .as<int32_t>(G), 2);

    NodeProperties<IrOpcode::SrcArrayAccess> NPAC(NPA.dest());
    ASSERT_TRUE(NPAC);
    ASSERT_EQ(NPAC.dim_size(), 2);
    Node *Dim0 = NPAC.dim(0),
         *Dim1 = NPAC.dim(1);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(Dim0)
              .as<int32_t>(G), 1);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(Dim1)
              .as<int32_t>(G), 5);
  }
  SS.clear();
  {
    // multiple array assignments
    SS << "array[94][87] megumin;"
       << "let megumin[0][8] <- 20\n"
       << "let megumin[9][97] <- 50";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    P.NewSymScope();
    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcArrayDecl>());

    Node* Assign1 = P.ParseAssignment();
    NodeProperties<IrOpcode::SrcAssignStmt> NPA1(Assign1);
    ASSERT_TRUE(NPA1);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(NPA1.source())
              .as<int32_t>(G), 20);

    Node* Assign2 = P.ParseAssignment();
    NodeProperties<IrOpcode::SrcAssignStmt> NPA2(Assign2);
    ASSERT_TRUE(NPA2);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(NPA2.source())
              .as<int32_t>(G), 50);

    auto* Access2 = NPA2.dest();
    NodeProperties<IrOpcode::SrcArrayAccess> NPAC2(Access2);
    ASSERT_TRUE(NPAC2);
    EXPECT_EQ(NPAC2.effect_dependency(), Assign1);
  }
}
