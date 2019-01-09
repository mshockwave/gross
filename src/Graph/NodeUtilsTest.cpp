#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"

using namespace gross;

TEST(NodeBuilderTest, TestConstantNode) {
  Graph G;

  Node* NumNode = NodeBuilder<IrOpcode::ConstantInt>(&G, 87)
                  .Build();
  ASSERT_TRUE(NumNode);
  EXPECT_EQ(NumNode->getNumValueInput(), 0);
  EXPECT_EQ(NumNode->getNumControlInput(), 0);
  EXPECT_EQ(NumNode->getNumEffectInput(), 0);
  EXPECT_EQ(G.getNumConstNumber(), 1);

  Node* StrNode = NodeBuilder<IrOpcode::ConstantStr>(&G, "rem_the_best")
                  .Build();
  ASSERT_TRUE(StrNode);
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
    EXPECT_EQ(ArrDeclNode->getNumValueInput(), 3);
    EXPECT_EQ(ArrDeclNode->getNumControlInput(), 0);
    EXPECT_EQ(ArrDeclNode->getNumEffectInput(), 0);
    EXPECT_EQ(G.getNumConstStr(), 2);
    EXPECT_EQ(G.getNumConstNumber(), 5);
  }
}
