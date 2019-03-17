#include "gross/CodeGen/GraphScheduling.h"
#include "gross/CodeGen/PostMachineLowering.h"
#include "gross/Graph/NodeUtils.h"
#include "RegisterAllocator.h"
#include "Targets.h"
#include "DLXNodeUtils.h"
#include "PostRALowering.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(CodeGenUnitTest, BasicValueAllocationRATest) {
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
  {
    // simple control flow
    Graph G;
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_ra_simple_ctrl")
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Const3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* Const4 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
    auto* Const5 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5).Build();
    auto* Sum1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                 .LHS(Const1).RHS(Const2).Build();
    auto* Mul1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXMulI)
                 .LHS(Sum1).RHS(Const3).Build();

    auto* Branch = NodeBuilder<IrOpcode::If>(&G)
                   .Condition(Const1).Build();
    Branch->appendControlInput(Func);
    auto* TrueBr = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                   .IfStmt(Branch).Build();
    auto* FalseBr = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                    .IfStmt(Branch).Build();
    auto* Return1 = NodeBuilder<IrOpcode::Return>(&G, Sum1).Build();
    Return1->appendControlInput(FalseBr);

    auto* Branch2 = NodeBuilder<IrOpcode::If>(&G)
                    .Condition(Const1).Build();
    Branch2->appendControlInput(TrueBr);
    auto* TrueBr2 = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                    .IfStmt(Branch2).Build();
    auto* FalseBr2 = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                     .IfStmt(Branch2).Build();
    auto* Sum2_1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                   .LHS(Mul1).RHS(Const4).Build();
    auto* Sum2_2 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
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
      std::ofstream OF("TestRASimpleCtrl.dot");
      G.dumpGraphviz(OF);
    }

    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    EXPECT_EQ(Scheduler.schedule_size(), 1);
    auto* FuncSchedule = *Scheduler.schedule_begin();
    {
      std::ofstream OF("TestRASimpleCtrl.cfg.dot");
      FuncSchedule->dumpGraphviz(OF);
    }

    LinearScanRegisterAllocator<CompactDLXTargetTraits> RA(*FuncSchedule);
    RA.Allocate();
    {
      std::ofstream OF("TestRASimpleCtrl.cfg.ra.dot");
      FuncSchedule->dumpGraphviz(OF);
    }
  }
  {
    Graph G;
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_ra_simple_loop")
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Const3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* Const4 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
    auto* Sum1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                 .LHS(Const3).RHS(Const4).Build();
    auto* Sum2 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                 .LHS(Sum1).RHS(Const2).Build();

    auto* Loop = NodeBuilder<IrOpcode::Loop>(&G, Func)
                 .Condition(Const1).Build();
    auto* PHINode = NodeBuilder<IrOpcode::Phi>(&G)
                    .AddValueInput(Sum1).AddValueInput(Sum2)
                    .SetCtrlMerge(Loop)
                    .Build();
    Sum2->ReplaceUseOfWith(Sum1, PHINode, Use::K_VALUE);
    auto* Br = NodeProperties<IrOpcode::Loop>(Loop).Branch();
    auto* Return1 = NodeBuilder<IrOpcode::Return>(&G, PHINode).Build();
    Return1->appendControlInput(NodeProperties<IrOpcode::If>(Br).FalseBranch());
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Return1)
                .Build();

    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestRASimpleLoop.dot");
      G.dumpGraphviz(OF);
    }

    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    EXPECT_EQ(Scheduler.schedule_size(), 1);
    auto* FuncSchedule = *Scheduler.schedule_begin();
    {
      std::ofstream OF("TestRASimpleLoop.cfg.dot");
      FuncSchedule->dumpGraphviz(OF);
    }

    LinearScanRegisterAllocator<CompactDLXTargetTraits> RA(*FuncSchedule);
    RA.Allocate();
    {
      std::ofstream OF("TestRASimpleLoop.cfg.ra.dot");
      FuncSchedule->dumpGraphviz(OF);
    }
  }
}

TEST(CodeGenUnitTest, AdvancedValueAllocationRATest) {
  {
    // w/ less than five function parameters
    Graph G;
    auto* Arg1 = NodeBuilder<IrOpcode::Argument>(&G, "a").Build();
    auto* Arg2 = NodeBuilder<IrOpcode::Argument>(&G, "b").Build();
    auto* Arg3 = NodeBuilder<IrOpcode::Argument>(&G, "c").Build();
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_ra_small_args")
                 .AddParameter(Arg1).AddParameter(Arg2).AddParameter(Arg3)
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Const3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* Const4 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
    auto* Const5 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5).Build();
    auto* Sum1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                 .LHS(Const1).RHS(Const2).Build();
    auto* Mul1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXMulI)
                 .LHS(Sum1).RHS(Arg1).Build();

    auto* Branch = NodeBuilder<IrOpcode::If>(&G)
                   .Condition(Const1).Build();
    Branch->appendControlInput(Func);
    auto* TrueBr = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                   .IfStmt(Branch).Build();
    auto* FalseBr = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                    .IfStmt(Branch).Build();
    auto* Return1 = NodeBuilder<IrOpcode::Return>(&G, Sum1).Build();
    Return1->appendControlInput(FalseBr);

    auto* Branch2 = NodeBuilder<IrOpcode::If>(&G)
                    .Condition(Const1).Build();
    Branch2->appendControlInput(TrueBr);
    auto* TrueBr2 = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                    .IfStmt(Branch2).Build();
    auto* FalseBr2 = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                     .IfStmt(Branch2).Build();
    auto* Sum2_1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                   .LHS(Mul1).RHS(Arg2).Build();
    auto* Sum2_2 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                   .LHS(Mul1).RHS(Arg3).Build();
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
      std::ofstream OF("TestRASmallParams.dot");
      G.dumpGraphviz(OF);
    }

    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    EXPECT_EQ(Scheduler.schedule_size(), 1);
    auto* FuncSchedule = *Scheduler.schedule_begin();
    {
      std::ofstream OF("TestRASmallParams.cfg.dot");
      FuncSchedule->dumpGraphviz(OF);
    }

    LinearScanRegisterAllocator<CompactDLXTargetTraits> RA(*FuncSchedule);
    RA.Allocate();
    {
      std::ofstream OF("TestRASmallParams.cfg.ra.dot");
      FuncSchedule->dumpGraphviz(OF);
    }

    PostRALowering PostRA(*FuncSchedule);
    PostRA.Run();
    {
      std::ofstream OF("TestRASmallParams.cfg.ra.postra.dot");
      FuncSchedule->dumpGraphviz(OF);
    }
  }
  {
    // w/ more than four function parameters
    Graph G;
    auto* Arg1 = NodeBuilder<IrOpcode::Argument>(&G, "a").Build();
    auto* Arg2 = NodeBuilder<IrOpcode::Argument>(&G, "b").Build();
    auto* Arg3 = NodeBuilder<IrOpcode::Argument>(&G, "c").Build();
    auto* Arg4 = NodeBuilder<IrOpcode::Argument>(&G, "d").Build();
    auto* Arg5 = NodeBuilder<IrOpcode::Argument>(&G, "e").Build();
    auto* Arg6 = NodeBuilder<IrOpcode::Argument>(&G, "f").Build();
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_ra_many_args")
                 .AddParameter(Arg1).AddParameter(Arg2).AddParameter(Arg3)
                 .AddParameter(Arg4).AddParameter(Arg5).AddParameter(Arg6)
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Const3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* Const4 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
    auto* Const5 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5).Build();
    auto* Sum1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                 .LHS(Const1).RHS(Arg4).Build();
    auto* Mul1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXMulI)
                 .LHS(Sum1).RHS(Arg1).Build();

    auto* Branch = NodeBuilder<IrOpcode::If>(&G)
                   .Condition(Const1).Build();
    Branch->appendControlInput(Func);
    auto* TrueBr = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                   .IfStmt(Branch).Build();
    auto* FalseBr = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                    .IfStmt(Branch).Build();
    auto* Return1 = NodeBuilder<IrOpcode::Return>(&G, Sum1).Build();
    Return1->appendControlInput(FalseBr);

    auto* Branch2 = NodeBuilder<IrOpcode::If>(&G)
                    .Condition(Arg5).Build();
    Branch2->appendControlInput(TrueBr);
    auto* TrueBr2 = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                    .IfStmt(Branch2).Build();
    auto* FalseBr2 = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                     .IfStmt(Branch2).Build();
    auto* Sum2_1 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                   .LHS(Arg6).RHS(Arg2).Build();
    auto* Sum2_2 = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI)
                   .LHS(Mul1).RHS(Arg3).Build();
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
      std::ofstream OF("TestRAManyParams.dot");
      G.dumpGraphviz(OF);
    }

    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    EXPECT_EQ(Scheduler.schedule_size(), 1);
    auto* FuncSchedule = *Scheduler.schedule_begin();
    {
      std::ofstream OF("TestRAManyParams.cfg.dot");
      FuncSchedule->dumpGraphviz(OF);
    }

    LinearScanRegisterAllocator<CompactDLXTargetTraits> RA(*FuncSchedule);
    RA.Allocate();
    {
      std::ofstream OF("TestRAManyParams.cfg.ra.dot");
      FuncSchedule->dumpGraphviz(OF);
    }

    PostRALowering PostRA(*FuncSchedule);
    PostRA.Run();
    {
      std::ofstream OF("TestRAManyParams.cfg.ra.postra.dot");
      FuncSchedule->dumpGraphviz(OF);
    }
  }
}

TEST(CodeGenUnitTest, InterprocedureRATest) {
  {
    // test callsite lowering - w/ # of parameters > 4
    Graph G;
    auto* Arg1 = NodeBuilder<IrOpcode::Argument>(&G, "a").Build();
    auto* Arg2 = NodeBuilder<IrOpcode::Argument>(&G, "b").Build();
    auto* Arg3 = NodeBuilder<IrOpcode::Argument>(&G, "c").Build();
    auto* Arg4 = NodeBuilder<IrOpcode::Argument>(&G, "d").Build();
    auto* Arg5 = NodeBuilder<IrOpcode::Argument>(&G, "e").Build();
    auto* Arg6 = NodeBuilder<IrOpcode::Argument>(&G, "f").Build();
    auto* Arg7 = NodeBuilder<IrOpcode::Argument>(&G, "g").Build();
    auto* Arg8 = NodeBuilder<IrOpcode::Argument>(&G, "h").Build();
    auto* Func1 = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                  .FuncName("func_ra_callsite_lowering_callee")
                  .AddParameter(Arg1).AddParameter(Arg2).AddParameter(Arg3)
                  .AddParameter(Arg4).AddParameter(Arg5).AddParameter(Arg6)
                  .AddParameter(Arg7).AddParameter(Arg8)
                  .Build();
    auto* Sum = NodeBuilder<IrOpcode::BinAdd>(&G)
                .LHS(Arg8).RHS(Arg2).Build();
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
                  .FuncName("func_ra_callsite_lowering_caller")
                  .Build();
    Node* Consts[8];
    for(auto i = 0; i < 8; ++i)
      Consts[i] = NodeBuilder<IrOpcode::ConstantInt>(&G, i + 1).Build();
    NodeBuilder<IrOpcode::Call> CallBuilder(&G, CalleeStub);
    for(auto i = 0; i < 8; ++i)
      CallBuilder.AddParam(Consts[i]);
    auto* Call = CallBuilder.Build();
    auto* Return2 = NodeBuilder<IrOpcode::Return>(&G, Call).Build();
    Return2->appendControlInput(Func2);
    auto* End2 = NodeBuilder<IrOpcode::End>(&G, Func2)
                 .AddTerminator(Return2)
                 .Build();
    SubGraph SGCaller(End2);
    G.AddSubRegion(SGCaller);
    //{
      //std::ofstream OF("TestRACallsiteLowering.dot");
      //G.dumpGraphviz(OF);
    //}

    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    EXPECT_EQ(Scheduler.schedule_size(), 2);
    size_t Counter = 1;
    for(auto* FuncSchedule : Scheduler.schedules()) {
      std::stringstream SS;
      //SS << "TestRACallsiteLowering-"
         //<< Counter << ".schedule.dot";
      //std::ofstream OFSchedule(SS.str());
      //FuncSchedule->dumpGraphviz(OFSchedule);
      //SS.str("");

      SS << "TestRACallsiteLowering-"
         << Counter << ".schedule.postlower.dot";
      PostMachineLowering PostLowering(*FuncSchedule);
      PostLowering.Run();
      std::ofstream OFPostLower(SS.str());
      FuncSchedule->dumpGraphviz(OFPostLower);
      SS.str("");

      SS << "TestRACallsiteLowering-"
         << Counter++ << ".schedule.postlower.ra.dot";
      LinearScanRegisterAllocator<CompactDLXTargetTraits> RA(*FuncSchedule);
      RA.Allocate();
      std::ofstream OFRA(SS.str());
      FuncSchedule->dumpGraphviz(OFRA);
    }
  }
}
