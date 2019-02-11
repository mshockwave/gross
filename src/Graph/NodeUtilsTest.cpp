#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"

using namespace gross;

TEST(NodeBuilderUnitTest, TestConstantNode) {
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

TEST(NodeBuilderUnitTest, TestVarArrayDecls) {
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

TEST(NodeBuilderUnitTest, TestSimpleBinOps) {
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

TEST(NodeBuilderUnitTest, TestSrcDesigAccess) {
  {
    // access scalar with no side effects
    Graph G;
    auto* VarDeclNode = NodeBuilder<IrOpcode::SrcVarDecl>(&G)
                        .SetSymbolName("rem_my_wife")
                        .Build();
    auto* AccessNode = NodeBuilder<IrOpcode::SrcVarAccess>(&G)
                       .Decl(VarDeclNode)
                       .Build();
    ASSERT_TRUE(AccessNode);
    EXPECT_EQ(AccessNode->getOp(), IrOpcode::SrcVarAccess);
    EXPECT_EQ(AccessNode->getNumValueInput(), 1);
    EXPECT_EQ(AccessNode->getNumControlInput(), 0);
    EXPECT_EQ(AccessNode->getNumEffectInput(), 0);
  }
  {
    // access scalar with side effects
    Graph G;
    auto* VarDeclNode = NodeBuilder<IrOpcode::SrcVarDecl>(&G)
                        .SetSymbolName("rem_my_wife")
                        .Build();

    auto* Dummy = NodeBuilder<IrOpcode::ConstantInt>(&G, 94).Build();

    auto* AccessNode = NodeBuilder<IrOpcode::SrcVarAccess>(&G)
                       .Decl(VarDeclNode).Effect(Dummy)
                       .Build();
    ASSERT_TRUE(AccessNode);
    EXPECT_EQ(AccessNode->getOp(), IrOpcode::SrcVarAccess);
    EXPECT_EQ(AccessNode->getNumValueInput(), 1);
    EXPECT_EQ(AccessNode->getNumControlInput(), 0);
    ASSERT_EQ(AccessNode->getNumEffectInput(), 1);
    EXPECT_EQ(AccessNode->getEffectInput(0), Dummy);
  }
  {
    // access scalar with no side effects
    Graph G;
    auto* Dim0 = NodeBuilder<IrOpcode::ConstantInt>(&G, 94).Build();
    auto* Dim1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 87).Build();

    auto* ArrDeclNode = NodeBuilder<IrOpcode::SrcArrayDecl>(&G)
                        .SetSymbolName("rem_my_wifes")
                        .AddDim(Dim0).AddDim(Dim1)
                        .Build();

    auto* ADim0 = NodeBuilder<IrOpcode::ConstantInt>(&G, 0).Build();
    auto* ADim1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* AccessNode = NodeBuilder<IrOpcode::SrcArrayAccess>(&G)
                       .Decl(ArrDeclNode)
                       .AppendAccessDim(ADim0).AppendAccessDim(ADim1)
                       .Build();
    ASSERT_TRUE(AccessNode);
    EXPECT_EQ(AccessNode->getOp(), IrOpcode::SrcArrayAccess);
    EXPECT_EQ(AccessNode->getNumValueInput(), 3);
    EXPECT_EQ(AccessNode->getNumControlInput(), 0);
    EXPECT_EQ(AccessNode->getNumEffectInput(), 0);
  }
  {
    // access scalar with side effects
    Graph G;
    auto* Dim0 = NodeBuilder<IrOpcode::ConstantInt>(&G, 94).Build();
    auto* Dim1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 87).Build();

    auto* ArrDeclNode = NodeBuilder<IrOpcode::SrcArrayDecl>(&G)
                        .SetSymbolName("rem_my_wifes")
                        .AddDim(Dim0).AddDim(Dim1)
                        .Build();

    auto* Dummy = NodeBuilder<IrOpcode::ConstantInt>(&G, 78).Build();

    auto* ADim0 = NodeBuilder<IrOpcode::ConstantInt>(&G, 0).Build();
    auto* ADim1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* AccessNode = NodeBuilder<IrOpcode::SrcArrayAccess>(&G)
                       .Decl(ArrDeclNode).Effect(Dummy)
                       .AppendAccessDim(ADim0).AppendAccessDim(ADim1)
                       .Build();
    ASSERT_TRUE(AccessNode);
    EXPECT_EQ(AccessNode->getOp(), IrOpcode::SrcArrayAccess);
    EXPECT_EQ(AccessNode->getNumValueInput(), 3);
    EXPECT_EQ(AccessNode->getNumControlInput(), 0);
    EXPECT_EQ(AccessNode->getNumEffectInput(), 1);
    EXPECT_EQ(AccessNode->getEffectInput(0), Dummy);
  }
}
