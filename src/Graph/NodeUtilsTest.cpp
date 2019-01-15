#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"

using namespace gross;

TEST(NodeBuilderTest, TestConstantNode) {
  Graph G;

  Node* NumNode = NodeBuilder<IrOpcode::ConstantInt>(&G, 87)
                  .Build();
  ASSERT_TRUE(NumNode);
  EXPECT_EQ(NumNode->getOp(), IrOpcode::ConstantInt);
  EXPECT_EQ(NumNode->getNumValueInput(), 0);
  EXPECT_EQ(NumNode->getNumControlInput(), 0);
  EXPECT_EQ(NumNode->getNumEffectInput(), 0);
  EXPECT_EQ(G.getNumConstNumber(), 1);

  Node* StrNode = NodeBuilder<IrOpcode::ConstantStr>(&G, "rem_the_best")
                  .Build();
  ASSERT_TRUE(StrNode);
  EXPECT_EQ(StrNode->getOp(), IrOpcode::ConstantStr);
  EXPECT_EQ(StrNode->getNumValueInput(), 0);
  EXPECT_EQ(StrNode->getNumControlInput(), 0);
  EXPECT_EQ(StrNode->getNumEffectInput(), 0);
  EXPECT_EQ(G.getNumConstStr(), 1);
}

TEST(NodeBuilderTest, TestVarArrayDecls) {
  Graph G;

  Node* VarDeclNode = NodeBuilder<IrOpcode::SrcVarDecl>(&G)
                      .SetSymbolName("rem_my_wife")
                      .Build();
  ASSERT_TRUE(VarDeclNode);
  EXPECT_EQ(VarDeclNode->getOp(), IrOpcode::SrcVarDecl);
  EXPECT_EQ(VarDeclNode->getNumValueInput(), 1);
  EXPECT_EQ(VarDeclNode->getNumControlInput(), 0);
  EXPECT_EQ(VarDeclNode->getNumEffectInput(), 0);
  EXPECT_EQ(G.getNumConstStr(), 1);

  // simple constant dimensions
  {
    Node* ArrDeclNode = NodeBuilder<IrOpcode::SrcArrayDecl>(&G)
                        .SetSymbolName("rem_my_wife")
                        .AddConstDim(9)
                        .AddConstDim(8)
                        .AddConstDim(7)
                        .Build();
    ASSERT_TRUE(ArrDeclNode);
    EXPECT_EQ(ArrDeclNode->getOp(), IrOpcode::SrcArrayDecl);
    EXPECT_EQ(ArrDeclNode->getNumValueInput(), 4);
    EXPECT_EQ(ArrDeclNode->getNumControlInput(), 0);
    EXPECT_EQ(ArrDeclNode->getNumEffectInput(), 0);
    EXPECT_EQ(G.getNumConstStr(), 1); // reuse the name string
    EXPECT_EQ(G.getNumConstNumber(), 3);
  }
  // aux dimension number nodes
  {
    Node *DimNode1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build(),
         *DimNode2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    Node* ArrDeclNode = NodeBuilder<IrOpcode::SrcArrayDecl>(&G)
                        .SetSymbolName("rem_the_best")
                        .AddDim(DimNode1)
                        .AddDim(DimNode2)
                        .Build();
    ASSERT_TRUE(ArrDeclNode);
    EXPECT_EQ(ArrDeclNode->getOp(), IrOpcode::SrcArrayDecl);
    EXPECT_EQ(ArrDeclNode->getNumValueInput(), 3);
    EXPECT_EQ(ArrDeclNode->getNumControlInput(), 0);
    EXPECT_EQ(ArrDeclNode->getNumEffectInput(), 0);
    EXPECT_EQ(G.getNumConstStr(), 2);
    EXPECT_EQ(G.getNumConstNumber(), 5);
  }
}

TEST(NodeBuilderTest, TestSimpleBinOps) {
  // And also the relation operator
  Graph G;

  auto* LHSNum = NodeBuilder<IrOpcode::ConstantInt>(&G, 94).Build();
  auto* RHSNum = NodeBuilder<IrOpcode::ConstantInt>(&G, 87).Build();

  // Since they share the same implementation
  // we only need to test one of them
  auto* NodeAdd = NodeBuilder<IrOpcode::BinAdd>(&G)
                  .LHS(LHSNum).RHS(RHSNum)
                  .Build();
  ASSERT_TRUE(NodeAdd);
  EXPECT_EQ(NodeAdd->getOp(), IrOpcode::BinAdd);
  EXPECT_EQ(NodeAdd->getNumValueInput(), 2);
  EXPECT_EQ(NodeAdd->getNumControlInput(), 0);
  EXPECT_EQ(NodeAdd->getNumEffectInput(), 0);
}

TEST(NodeBuilderTest, TestSrcAssignStmt) {
  {
    // assign to scalar with no side effects
    Graph G;
    auto* VarDeclNode = NodeBuilder<IrOpcode::SrcVarDecl>(&G)
                        .SetSymbolName("rem_my_wife")
                        .Build();
    auto* RHSValue = NodeBuilder<IrOpcode::ConstantInt>(&G, 94).Build();
    auto* AssignNode = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                       .Dest(VarDeclNode).Src(RHSValue)
                       .Build();
    ASSERT_TRUE(AssignNode);
    EXPECT_EQ(AssignNode->getOp(), IrOpcode::SrcAssignStmt);
    EXPECT_EQ(AssignNode->getNumValueInput(), 2);
    EXPECT_EQ(AssignNode->getNumControlInput(), 0);
    EXPECT_EQ(AssignNode->getNumEffectInput(), 0);
  }
  {
    // assign to scalar with side effects
    Graph G;
    auto* VarDeclNode = NodeBuilder<IrOpcode::SrcVarDecl>(&G)
                        .SetSymbolName("rem_my_wife")
                        .Build();

    auto* RHSNum1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 94).Build();
    auto* RHSNum2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 87).Build();

    auto* AssignNode = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                       .Dest(VarDeclNode).Src(RHSNum1)
                       .Build();
    auto* AssignNode2 = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                        .Dest(VarDeclNode, AssignNode).Src(RHSNum2)
                        .Build();
    ASSERT_TRUE(AssignNode2);
    EXPECT_EQ(AssignNode2->getOp(), IrOpcode::SrcAssignStmt);
    EXPECT_EQ(AssignNode2->getNumValueInput(), 2);
    EXPECT_EQ(AssignNode2->getNumControlInput(), 0);
    ASSERT_EQ(AssignNode2->getNumEffectInput(), 1);
    EXPECT_EQ(AssignNode2->getEffectInput(0), AssignNode);
  }
}
