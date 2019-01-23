#include "Parser.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"
#include <sstream>

using namespace gross;

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
