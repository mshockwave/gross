#include "gross/Graph/Reductions/Peephole.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(GRPeepholeUnitTest, ConstReductionTest) {
  {
    Graph G;
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_const_reduce1")
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 87)
                   .Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 94)
                   .Build();
    auto* RHSVal = NodeBuilder<IrOpcode::BinAdd>(&G)
                   .LHS(Const1).RHS(Const2)
                   .Build();
    auto* Return = NodeBuilder<IrOpcode::Return>(&G, RHSVal)
                   .Build();
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Return)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestPHConstReduce.dot");
      G.dumpGraphviz(OF);
    }

    RunReducer<PeepholeReducer>(G, G);
    {
      std::ofstream OF("TestPHConstReduce.after.dot");
      G.dumpGraphviz(OF);
    }
  }
  {
    // heirarchy expression
    Graph G;
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_const_reduce2")
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 1)
                   .Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2)
                   .Build();
    auto* Const3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3)
                   .Build();
    auto* Const4 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4)
                   .Build();
    auto* Const5 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5)
                   .Build();
    auto* RHSVal1 = NodeBuilder<IrOpcode::BinAdd>(&G)
                   .LHS(Const1).RHS(Const2)
                   .Build();
    auto* RHSVal2 = NodeBuilder<IrOpcode::BinMul>(&G)
                   .LHS(Const3).RHS(RHSVal1)
                   .Build();
    auto* RHSVal3 = NodeBuilder<IrOpcode::BinAdd>(&G)
                   .LHS(Const4).RHS(RHSVal2)
                   .Build();
    auto* RHSVal4 = NodeBuilder<IrOpcode::BinMul>(&G)
                   .LHS(Const5).RHS(RHSVal3)
                   .Build();
    auto* Return = NodeBuilder<IrOpcode::Return>(&G, RHSVal4)
                   .Build();
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Return)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestPHConstReduce2.dot");
      G.dumpGraphviz(OF);
    }

    RunReducer<PeepholeReducer>(G, G);
    {
      std::ofstream OF("TestPHConstReduce2.after.dot");
      G.dumpGraphviz(OF);
    }
  }
}
