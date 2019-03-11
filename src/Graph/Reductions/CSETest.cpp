#include "gross/Graph/Reductions/CSE.h"
//#include "gross/Graph/Reductions/ValuePromotion.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(GRCSEUnitTest, ArithmeticCSETest) {
  {
    // test commutative
    Graph G;
    auto* Arg = NodeBuilder<IrOpcode::Argument>(&G, "a").Build();
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_arithmetic_cse1")
                 .AddParameter(Arg)
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 94)
                   .Build();
    auto* Val1 = NodeBuilder<IrOpcode::BinAdd>(&G)
                 .LHS(Const1).RHS(Arg)
                 .Build();
    auto* Val2 = NodeBuilder<IrOpcode::BinAdd>(&G)
                 .LHS(Arg).RHS(Const1)
                 .Build();
    auto* Val3 = NodeBuilder<IrOpcode::BinDiv>(&G)
                 .LHS(Val1).RHS(Val2)
                 .Build();

    auto* Val4 = NodeBuilder<IrOpcode::BinSub>(&G)
                 .LHS(Arg).RHS(Const1)
                 .Build();
    auto* Val5 = NodeBuilder<IrOpcode::BinSub>(&G)
                 .LHS(Const1).RHS(Arg)
                 .Build();
    auto* Val6 = NodeBuilder<IrOpcode::BinMul>(&G)
                 .LHS(Val4).RHS(Val5)
                 .Build();

    auto* RetVal = NodeBuilder<IrOpcode::BinAdd>(&G)
                   .LHS(Val3).RHS(Val6)
                   .Build();
    auto* Return = NodeBuilder<IrOpcode::Return>(&G, RetVal)
                   .Build();
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Return)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestCSEArithmetic1.dot");
      G.dumpGraphviz(OF);
    }

    GraphReducer::RunWithEditor<CSEReducer>(G);
    {
      std::ofstream OF("TestCSEArithmetic1.after.dot");
      G.dumpGraphviz(OF);
    }
    NodeProperties<IrOpcode::VirtBinOps> BNP3(Val3);
    EXPECT_EQ(BNP3.LHS(), BNP3.RHS());
    NodeProperties<IrOpcode::VirtBinOps> BNP6(Val6);
    EXPECT_NE(BNP6.LHS(), BNP6.RHS());
  }
}

TEST(GRCSEUnitTest, MemoryCSETest) {
  Graph G;
  auto* Arg = NodeBuilder<IrOpcode::Argument>(&G, "a").Build();
  auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
               .FuncName("func_memory_cse1")
               .AddParameter(Arg)
               .Build();
  auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 94)
                 .Build();
  auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 87)
                 .Build();

  auto* Alloca = NodeBuilder<IrOpcode::Alloca>(&G)
                 .Size(Const1).Build();
  auto* Store1 = NodeBuilder<IrOpcode::MemStore>(&G)
                 .BaseAddr(Alloca).Offset(Const2)
                 .Src(Arg).Build();

  auto* Offset1 = NodeBuilder<IrOpcode::BinMul>(&G)
                  .LHS(Arg).RHS(Const1)
                  .Build();
  auto* Offset2 = NodeBuilder<IrOpcode::BinMul>(&G)
                  .LHS(Const1).RHS(Arg)
                  .Build();
  auto* Load1 = NodeBuilder<IrOpcode::MemLoad>(&G)
                .BaseAddr(Alloca).Offset(Offset1)
                .Build();
  auto* Load2 = NodeBuilder<IrOpcode::MemLoad>(&G)
                .BaseAddr(Alloca).Offset(Offset2)
                .Build();
  Load1->appendEffectInput(Store1);
  Load2->appendEffectInput(Store1);

  auto* RetVal = NodeBuilder<IrOpcode::BinAdd>(&G)
                 .LHS(Load1).RHS(Load2)
                 .Build();
  auto* Return = NodeBuilder<IrOpcode::Return>(&G, RetVal)
                 .Build();
  auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
              .AddTerminator(Return)
              .Build();
  SubGraph FuncSG(End);
  G.AddSubRegion(FuncSG);
  {
    std::ofstream OF("TestCSEMemory1.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<CSEReducer>(G);
  {
    std::ofstream OF("TestCSEMemory1.after.dot");
    G.dumpGraphviz(OF);
  }
  NodeProperties<IrOpcode::VirtBinOps> BNP(RetVal);
  EXPECT_EQ(BNP.LHS(), BNP.RHS());
}
