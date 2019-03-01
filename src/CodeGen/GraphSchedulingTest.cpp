#include "BGL.h"
#include "boost/concept/assert.hpp"
#include "boost/graph/graph_concepts.hpp"
#include "gross/Graph/NodeUtils.h"
#include "gross/CodeGen/GraphScheduling.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(CodeGenUnitTest, TestGraphScheduleConcepts) {
  // requirement for DFS
  BOOST_CONCEPT_ASSERT(( boost::VertexListGraphConcept<gross::GraphSchedule> ));
  BOOST_CONCEPT_ASSERT(( boost::IncidenceGraphConcept<gross::GraphSchedule> ));
  // additional requirement for Graphviz
  BOOST_CONCEPT_ASSERT(( boost::EdgeListGraphConcept<gross::GraphSchedule> ));
  BOOST_CONCEPT_ASSERT(( boost::ReadablePropertyMapConcept<
                          gross::graph_id_map<gross::GraphSchedule,
                                              boost::vertex_index_t>,
                          gross::BasicBlock* > ));
}

TEST(CodeGenUnitTest, GraphScheduleCFGSimpleCtrl) {
  Graph G;
  auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
               .FuncName("func_schedule_cfg_simple_ctrl")
               .Build();
  auto* Const = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
  auto* Branch = NodeBuilder<IrOpcode::If>(&G)
                 .Condition(Const).Build();
  Branch->appendControlInput(Func);
  auto* TrueBr = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                 .IfStmt(Branch).Build();
  auto* FalseBr = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                  .IfStmt(Branch).Build();
  auto* Branch2 = NodeBuilder<IrOpcode::If>(&G)
                  .Condition(Const).Build();
  Branch2->appendControlInput(TrueBr);
  auto* TrueBr2 = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                  .IfStmt(Branch2).Build();
  auto* FalseBr2 = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                   .IfStmt(Branch2).Build();
  auto* MergeInner = NodeBuilder<IrOpcode::Merge>(&G)
                     .AddCtrlInput(TrueBr2).AddCtrlInput(FalseBr2)
                     .Build();
  auto* MergeOuter = NodeBuilder<IrOpcode::Merge>(&G)
                     .AddCtrlInput(FalseBr).AddCtrlInput(MergeInner)
                     .Build();
  auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
              .AddTerminator(MergeOuter)
              .Build();
  SubGraph FuncSG(End);
  G.AddSubRegion(FuncSG);
  {
    std::ofstream OF("TestGraphScheduleCFGSimpleCtrl.dot");
    G.dumpGraphviz(OF);
  }

  GraphScheduler Scheduler(G);
  Scheduler.ComputeScheduledGraph();
  EXPECT_EQ(Scheduler.schedule_size(), 1);
  auto* FuncSchedule = *Scheduler.schedule_begin();
  {
    std::ofstream OF("TestGraphScheduleCFGSimpleCtrl.after.dot");
    FuncSchedule->dumpGraphviz(OF);
  }
  {
    std::ofstream OF("TestGraphScheduleCFGSimpleCtrl.after.dom.dot");
    FuncSchedule->dumpDomTreeGraphviz(OF);
  }
}

TEST(CodeGenUnitTest, GraphScheduleCFGSimpleLoop) {
  Graph G;
  auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
               .FuncName("func_schedule_cfg_simple_loop")
               .Build();
  auto* Const = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
  auto* Loop = NodeBuilder<IrOpcode::Loop>(&G, Func)
               .Condition(Const).Build();
  auto* Br = NodeProperties<IrOpcode::Loop>(Loop).Branch();
  auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
              .AddTerminator(NodeProperties<IrOpcode::If>(Br).FalseBranch())
              .Build();
  SubGraph FuncSG(End);
  G.AddSubRegion(FuncSG);
  {
    std::ofstream OF("TestGraphScheduleCFGSimpleLoop.dot");
    G.dumpGraphviz(OF);
  }

  GraphScheduler Scheduler(G);
  Scheduler.ComputeScheduledGraph();
  EXPECT_EQ(Scheduler.schedule_size(), 1);
  auto* FuncSchedule = *Scheduler.schedule_begin();
  {
    std::ofstream OF("TestGraphScheduleCFGSimpleLoop.after.dot");
    FuncSchedule->dumpGraphviz(OF);
  }
  {
    std::ofstream OF("TestGraphScheduleCFGSimpleLoop.after.dom.dot");
    FuncSchedule->dumpDomTreeGraphviz(OF);
  }
}

TEST(CodeGenTest, GraphScheduleValueNodePlacement) {
  {
    Graph G;
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_schedule_value_node_placement")
                 .Build();
    auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
    auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Const3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* Const4 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
    auto* Const5 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5).Build();
    auto* Sum1 = NodeBuilder<IrOpcode::BinAdd>(&G)
                 .LHS(Const1).RHS(Const2).Build();
    auto* Mul1 = NodeBuilder<IrOpcode::BinMul>(&G)
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
      std::ofstream OF("TestGraphScheduleValueNodePlacement1.dot");
      G.dumpGraphviz(OF);
    }

    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    EXPECT_EQ(Scheduler.schedule_size(), 1);
    auto* FuncSchedule = *Scheduler.schedule_begin();
    {
      std::ofstream OF("TestGraphScheduleValueNodePlacement1.after.dot");
      FuncSchedule->dumpGraphviz(OF);
    }
    {
      std::ofstream OF("TestGraphScheduleValueNodePlacement1.after.dom.dot");
      FuncSchedule->dumpDomTreeGraphviz(OF);
    }
  }
}
