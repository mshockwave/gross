#include "gross/CodeGen/PreMachineLowering.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Graph/Graph.h"
#include "gtest/gtest.h"
#include "DLXNodeUtils.h"
#include <iterator>
#include <fstream>

using namespace gross;

TEST(CodeGenUnitTest, PreLoweringArithmeticOps) {
  {
    Graph G;
    auto* Arg1 = NodeBuilder<IrOpcode::Argument>(&G, "a").Build();
    auto* Arg2 = NodeBuilder<IrOpcode::Argument>(&G, "b").Build();
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_arithmetic_lowering1")
                 .AddParameter(Arg1).AddParameter(Arg2)
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 87)
                   .Build();
    auto* Val1 = NodeBuilder<IrOpcode::BinAdd>(&G)
                 .LHS(Const1).RHS(Arg1)
                 .Build();
    auto* Val2 = NodeBuilder<IrOpcode::BinAdd>(&G)
                 .LHS(Arg2).RHS(Val1)
                 .Build();
    auto* Return = NodeBuilder<IrOpcode::Return>(&G, Val2)
                   .Build();
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Return)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestPreLoweringArithmetic1.dot");
      G.dumpGraphviz(OF);
    }

    GraphReducer::RunWithEditor<PreMachineLowering>(G);
    {
      std::ofstream OF("TestPreLoweringArithmetic1.after.dot");
      G.dumpGraphviz(OF);
    }
    NodeProperties<IrOpcode::Return> RNP(Return);
    auto* RetVal = RNP.ReturnVal();
    EXPECT_TRUE(NodeProperties<IrOpcode::DLXAdd>(RetVal));
    auto* ParentVal = NodeProperties<IrOpcode::VirtDLXBinOps>(RetVal)
                      .RHS();
    EXPECT_TRUE(NodeProperties<IrOpcode::DLXAddI>(ParentVal));
  }
  {
    // optimize certain multiplication into left shift
    Graph G;
    auto* Arg1 = NodeBuilder<IrOpcode::Argument>(&G, "a").Build();
    auto* Arg2 = NodeBuilder<IrOpcode::Argument>(&G, "b").Build();
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_arithmetic_lowering2")
                 .AddParameter(Arg1).AddParameter(Arg2)
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 16)
                   .Build();
    auto* Val1 = NodeBuilder<IrOpcode::BinMul>(&G)
                 .LHS(Arg1).RHS(Const1)
                 .Build();
    auto* Return = NodeBuilder<IrOpcode::Return>(&G, Val1)
                   .Build();
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Return)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestPreLoweringArithmetic2.dot");
      G.dumpGraphviz(OF);
    }

    GraphReducer::RunWithEditor<PreMachineLowering>(G);
    {
      std::ofstream OF("TestPreLoweringArithmetic2.after.dot");
      G.dumpGraphviz(OF);
    }
    NodeProperties<IrOpcode::Return> RNP(Return);
    auto* RetVal = RNP.ReturnVal();
    EXPECT_EQ(RetVal->getOp(), IrOpcode::DLXLshI);
    auto* ImmOffset = NodeProperties<IrOpcode::VirtDLXBinOps>(RetVal)
                      .ImmRHS();
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(ImmOffset)
              .as<int32_t>(G), 4);
  }
}

TEST(CodeGenUnitTest, PreLoweringMemoryOps) {
  {
    Graph G;
    auto* Arg1 = NodeBuilder<IrOpcode::Argument>(&G, "a").Build();
    auto* Arg2 = NodeBuilder<IrOpcode::Argument>(&G, "b").Build();
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_memory_lowering1")
                 .AddParameter(Arg1).AddParameter(Arg2)
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 87)
                   .Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5)
                   .Build();
    auto* Const3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 6)
                   .Build();
    auto* Alloca = NodeBuilder<IrOpcode::Alloca>(&G).Size(Const1)
                   .Build();
    auto* Store1 = NodeBuilder<IrOpcode::MemStore>(&G)
                   .BaseAddr(Alloca).Offset(Const2)
                   .Src(Const1).Build();
    auto* Load1 = NodeBuilder<IrOpcode::MemLoad>(&G)
                  .BaseAddr(Alloca).Offset(Const3)
                  .Build();
    Load1->appendEffectInput(Store1);
    auto* Store2 = NodeBuilder<IrOpcode::MemStore>(&G)
                   .BaseAddr(Alloca).Offset(Arg1)
                   .Src(Arg2).Build();
    Store2->appendEffectInput(Load1);
    auto* Return = NodeBuilder<IrOpcode::Return>(&G, Load1)
                   .Build();
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddEffectDep(Store2)
                .AddTerminator(Return)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestPreLoweringMemory1.dot");
      G.dumpGraphviz(OF);
    }

    GraphReducer::RunWithEditor<PreMachineLowering>(G);
    {
      std::ofstream OF("TestPreLoweringMemory1.after.dot");
      G.dumpGraphviz(OF);
    }
    NodeProperties<IrOpcode::Return> RNP(Return);
    auto* RetVal = RNP.ReturnVal();
    EXPECT_EQ(RetVal->getOp(), IrOpcode::DLXLdW);
    ASSERT_EQ(std::distance(RetVal->effect_users().begin(),
                            RetVal->effect_users().end()), 1);
    EXPECT_EQ((*RetVal->effect_users().begin())->getOp(),
              IrOpcode::DLXStX);
    ASSERT_EQ(RetVal->getNumEffectInput(), 1);
    EXPECT_EQ(RetVal->getEffectInput(0)->getOp(), IrOpcode::DLXStW);
  }
}
