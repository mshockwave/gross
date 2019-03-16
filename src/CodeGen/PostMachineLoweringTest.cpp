#include "BGL.h"
#include "boost/concept/assert.hpp"
#include "boost/graph/graph_concepts.hpp"
#include "gross/Graph/NodeUtils.h"
#include "gross/CodeGen/GraphScheduling.h"
#include "gross/CodeGen/PostMachineLowering.h"
#include "gtest/gtest.h"
#include <fstream>
#include <sstream>

using namespace gross;

TEST(CodeGenUnitTest, PostLoweringNormalCtrlStructures) {
  {
    Graph G;
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_post_lowering_normal_ctrl")
                 .Build();
    auto* Zero = NodeBuilder<IrOpcode::ConstantInt>(&G, 0).Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Const3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* Const4 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
    auto* Const5 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5).Build();
    auto* Sum1 = NodeBuilder<IrOpcode::BinAdd>(&G)
                 .LHS(Const1).RHS(Const2).Build();
    auto* Mul1 = NodeBuilder<IrOpcode::BinMul>(&G)
                 .LHS(Sum1).RHS(Const3).Build();
    auto* Cond1 = NodeBuilder<IrOpcode::BinLt>(&G)
                  .LHS(Const1).RHS(Zero).Build();
    auto* Cond2 = NodeBuilder<IrOpcode::BinGt>(&G)
                  .LHS(Const1).RHS(Zero).Build();

    auto* Branch = NodeBuilder<IrOpcode::If>(&G)
                   .Condition(Cond1).Build();
    Branch->appendControlInput(Func);
    auto* TrueBr = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                   .IfStmt(Branch).Build();
    auto* FalseBr = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                    .IfStmt(Branch).Build();
    auto* Return1 = NodeBuilder<IrOpcode::Return>(&G, Sum1).Build();
    Return1->appendControlInput(FalseBr);

    auto* Branch2 = NodeBuilder<IrOpcode::If>(&G)
                    .Condition(Cond2).Build();
    Branch2->appendControlInput(TrueBr);
    auto* TrueBr2 = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                    .IfStmt(Branch2).Build();
    auto* FalseBr2 = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                     .IfStmt(Branch2).Build();
    auto* Sum2_1 = NodeBuilder<IrOpcode::BinAdd>(&G)
                   .LHS(Mul1).RHS(Const4).Build();
    auto* Sum2_2 = NodeBuilder<IrOpcode::BinAdd>(&G)
                   .LHS(Mul1).RHS(Const5).Build();
    auto* MergeInner = NodeBuilder<IrOpcode::Merge>(&G)
                       .AddCtrlInput(TrueBr2).AddCtrlInput(FalseBr2)
                       .Build();
    auto* PHINode = NodeBuilder<IrOpcode::Phi>(&G)
                    .AddValueInput(Sum2_1).AddValueInput(Sum2_2)
                    .SetCtrlMerge(MergeInner)
                    .Build();

    auto* MergeOuter = NodeBuilder<IrOpcode::Merge>(&G)
                       .AddCtrlInput(Return1).AddCtrlInput(MergeInner)
                       .Build();
    auto* Return2 = NodeBuilder<IrOpcode::Return>(&G, PHINode).Build();
    Return2->appendControlInput(MergeOuter);
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Return2)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestPostLoweringNormalCtrl1.dot");
      G.dumpGraphviz(OF);
    }

    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    EXPECT_EQ(Scheduler.schedule_size(), 1);
    auto* FuncSchedule = *Scheduler.schedule_begin();
    {
      std::ofstream OF("TestPostLoweringNormalCtrl1.schedule.dot");
      FuncSchedule->dumpGraphviz(OF);
    }

    PostMachineLowering PostLowering(*FuncSchedule);
    PostLowering.Run();
    {
      std::ofstream OF("TestPostLoweringNormalCtrl1.schedule.postlower.dot");
      FuncSchedule->dumpGraphviz(OF);
    }
  }
  {
    Graph G;
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_post_lower_normal_ctrl2")
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Const3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* Const4 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
    auto* Sum1 = NodeBuilder<IrOpcode::BinMul>(&G)
                 .LHS(Const3).RHS(Const4).Build();
    auto* Sum2 = NodeBuilder<IrOpcode::BinAdd>(&G)
                 .LHS(Sum1).RHS(Const2).Build();

    auto* Loop = NodeBuilder<IrOpcode::Loop>(&G, Func)
                 .Condition(Const1).Build();
    auto* PHINode = NodeBuilder<IrOpcode::Phi>(&G)
                    .AddValueInput(Const2).AddValueInput(Sum2)
                    .SetCtrlMerge(Loop)
                    .Build();
    Sum2->ReplaceUseOfWith(Const2, PHINode, Use::K_VALUE);
    auto* Br = NodeProperties<IrOpcode::Loop>(Loop).Branch();
    auto* Return1 = NodeBuilder<IrOpcode::Return>(&G, PHINode).Build();
    Return1->appendControlInput(NodeProperties<IrOpcode::If>(Br).FalseBranch());
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Return1)
                .Build();

    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestPostLoweringNormalCtrl2.dot");
      G.dumpGraphviz(OF);
    }

    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    EXPECT_EQ(Scheduler.schedule_size(), 1);
    auto* FuncSchedule = *Scheduler.schedule_begin();
    {
      std::ofstream OF("TestPostLoweringNormalCtrl2.schedule.dot");
      FuncSchedule->dumpGraphviz(OF);
    }

    PostMachineLowering PostLowering(*FuncSchedule);
    PostLowering.Run();
    {
      std::ofstream OF("TestPostLoweringNormalCtrl2.schedule.postlower.dot");
      FuncSchedule->dumpGraphviz(OF);
    }
  }
}

TEST(CodeGenUnitTest, PostLoweringFuncCallsTest) {
  {
    // call w/o parameter
    Graph G;
    auto* Func1 = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                  .FuncName("func_post_lower_call1_callee")
                  .Build();
    auto* End1 = NodeBuilder<IrOpcode::End>(&G, Func1)
                .Build();
    SubGraph SGCallee(End1);
    G.AddSubRegion(SGCallee);
    auto* CalleeStub
      = NodeBuilder<IrOpcode::FunctionStub>(&G, SGCallee).Build();

    auto* Func2 = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                  .FuncName("func_post_lower_call1_caller")
                  .Build();
    auto* Call = NodeBuilder<IrOpcode::Call>(&G, CalleeStub)
                 .Build();
    auto* Return = NodeBuilder<IrOpcode::Return>(&G, Call).Build();
    Return->appendControlInput(Func2);
    auto* End2 = NodeBuilder<IrOpcode::End>(&G, Func2)
                 .AddTerminator(Return)
                 .Build();
    SubGraph SGCaller(End2);
    G.AddSubRegion(SGCaller);
    {
      std::ofstream OF("TestPostLoweringFuncCall1.dot");
      G.dumpGraphviz(OF);
    }

    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    EXPECT_EQ(Scheduler.schedule_size(), 2);
    size_t Counter = 1;
    for(auto* FuncSchedule : Scheduler.schedules()) {
      std::stringstream SS;
      SS << "TestPostLoweringFuncCall1-"
         << Counter << ".schedule.dot";
      std::ofstream OFSchedule(SS.str());
      FuncSchedule->dumpGraphviz(OFSchedule);
      SS.str("");

      SS << "TestPostLoweringFuncCall1-"
         << Counter++ << ".schedule.postlower.dot";
      PostMachineLowering PostLowering(*FuncSchedule);
      PostLowering.Run();
      std::ofstream OFPostLower(SS.str());
      FuncSchedule->dumpGraphviz(OFPostLower);
    }
  }
  {
    // call w/ parameters
    Graph G;
    auto* Arg1 = NodeBuilder<IrOpcode::Argument>(&G, "a").Build();
    auto* Arg2 = NodeBuilder<IrOpcode::Argument>(&G, "b").Build();
    auto* Func1 = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                  .FuncName("func_post_lower_call2_callee")
                  .AddParameter(Arg1).AddParameter(Arg2)
                  .Build();
    auto* Sum = NodeBuilder<IrOpcode::BinAdd>(&G)
                .LHS(Arg1).RHS(Arg2).Build();
    auto* Return1 = NodeBuilder<IrOpcode::Return>(&G, Sum).Build();
    Return1->appendControlInput(Func1);
    auto* End1 = NodeBuilder<IrOpcode::End>(&G, Func1)
                .AddTerminator(Return1)
                .Build();
    SubGraph SGCallee(End1);
    G.AddSubRegion(SGCallee);
    auto* CalleeStub
      = NodeBuilder<IrOpcode::FunctionStub>(&G, SGCallee).Build();

    auto* Func2 = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                  .FuncName("func_post_lower_call2_caller")
                  .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Call = NodeBuilder<IrOpcode::Call>(&G, CalleeStub)
                 .AddParam(Const1).AddParam(Const2)
                 .Build();
    auto* Return2 = NodeBuilder<IrOpcode::Return>(&G, Call).Build();
    Return2->appendControlInput(Func2);
    auto* End2 = NodeBuilder<IrOpcode::End>(&G, Func2)
                 .AddTerminator(Return2)
                 .Build();
    SubGraph SGCaller(End2);
    G.AddSubRegion(SGCaller);
    {
      std::ofstream OF("TestPostLoweringFuncCall2.dot");
      G.dumpGraphviz(OF);
    }

    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    EXPECT_EQ(Scheduler.schedule_size(), 2);
    size_t Counter = 1;
    for(auto* FuncSchedule : Scheduler.schedules()) {
      std::stringstream SS;
      SS << "TestPostLoweringFuncCall2-"
         << Counter << ".schedule.dot";
      std::ofstream OFSchedule(SS.str());
      FuncSchedule->dumpGraphviz(OFSchedule);
      SS.str("");

      SS << "TestPostLoweringFuncCall2-"
         << Counter++ << ".schedule.postlower.dot";
      PostMachineLowering PostLowering(*FuncSchedule);
      PostLowering.Run();
      std::ofstream OFPostLower(SS.str());
      FuncSchedule->dumpGraphviz(OFPostLower);
    }
  }
}
