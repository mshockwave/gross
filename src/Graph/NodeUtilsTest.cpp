#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"

using namespace gross;

TEST(NodeBuilderTest, TestConstantNode) {
  Graph G;

  Node* NumNode = NodeBuilder<IrOpcode::ConstantInt>(&G, 87)
                  .Build();
  ASSERT_TRUE(NumNode);
  ASSERT_EQ(NumNode->getNumValueInput(), 0);
  ASSERT_EQ(NumNode->getNumControlInput(), 0);
  ASSERT_EQ(NumNode->getNumEffectInput(), 0);
  ASSERT_EQ(G.getNumConstNumber(), 1);

  Node* StrNode = NodeBuilder<IrOpcode::ConstantStr>(&G, "rem_the_best")
                  .Build();
  ASSERT_TRUE(StrNode);
  ASSERT_EQ(StrNode->getNumValueInput(), 0);
  ASSERT_EQ(StrNode->getNumControlInput(), 0);
  ASSERT_EQ(StrNode->getNumEffectInput(), 0);
  ASSERT_EQ(G.getNumConstStr(), 1);
}

TEST(NodeBuilderTest, TestVarArrayDecls) {
  Graph G;
  // TODO
}
