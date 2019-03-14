#include "gross/CodeGen/GraphScheduling.h"
#include "gross/Graph/NodeUtils.h"
#include "RegisterAllocator.h"
#include "Targets.h"
#include "DLXNodeUtils.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(CodeGenUnitTest, ValueAllocationRATest) {
  {
    // simple straight line code
    Graph G;
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_ra_simple_straightline")
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Const3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* Const4 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
    auto* Const5 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5).Build();

    auto* Sum1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                 .LHS(Const1).RHS(Const2).Build();
    auto* Sum2 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                 .LHS(Sum1).RHS(Const2).Build();
    auto* Mul1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXMulI)
                 .LHS(Sum1).RHS(Const4).Build();
    auto* Sum3 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAdd)
                 .LHS(Sum1).RHS(Mul1).Build();
    auto* Sum4 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAdd)
                 .LHS(Sum2).RHS(Sum3).Build();
    auto* Mul2 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXMul)
                 .LHS(Sum1).RHS(Sum4).Build();
    auto* Return = NodeBuilder<IrOpcode::Return>(&G, Mul2)
                   .Build();
    Return->appendControlInput(Func);
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Return)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestRASimpleStraightline.dot");
      G.dumpGraphviz(OF);
    }

    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    EXPECT_EQ(Scheduler.schedule_size(), 1);
    auto* FuncSchedule = *Scheduler.schedule_begin();
    {
      std::ofstream OF("TestRASimpleStraightline.cfg.dot");
      FuncSchedule->dumpGraphviz(OF);
    }

    LinearScanRegisterAllocator<CompactDLXTargetTraits> RA(*FuncSchedule);
    RA.Allocate();
    {
      std::ofstream OF("TestRASimpleStraightline.cfg.ra.dot");
      FuncSchedule->dumpGraphviz(OF);
    }
  }
}
