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

TEST(CodeGenUnitTest, TestGraphScheduleSimpleCtrl) {
  Graph G;
  auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
               .FuncName("func_schedule_simple_ctrl")
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
    std::ofstream OF("TestGraphScheduleSimpleCtrl.dot");
    G.dumpGraphviz(OF);
  }

  GraphScheduler Scheduler(G);
  Scheduler.ComputeScheduledGraph();
  EXPECT_EQ(Scheduler.schedule_size(), 1);
  auto* FuncSchedule = *Scheduler.schedule_begin();
  {
    std::ofstream OF("TestGraphScheduleSimpleCtrl.after.dot");
    FuncSchedule->dumpGraphviz(OF);
  }
  {
    std::ofstream OF("TestGraphScheduleSimpleCtrl.after.dom.dot");
    FuncSchedule->dumpDomTreeGraphviz(OF);
  }
}

TEST(CodeGenUnitTest, TestGraphScheduleSimpleLoop) {
  Graph G;
  auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
               .FuncName("func_schedule_simple_loop")
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
    std::ofstream OF("TestGraphScheduleSimpleLoop.dot");
    G.dumpGraphviz(OF);
  }

  GraphScheduler Scheduler(G);
  Scheduler.ComputeScheduledGraph();
  EXPECT_EQ(Scheduler.schedule_size(), 1);
  auto* FuncSchedule = *Scheduler.schedule_begin();
  {
    std::ofstream OF("TestGraphScheduleSimpleLoop.after.dot");
    FuncSchedule->dumpGraphviz(OF);
  }
  {
    std::ofstream OF("TestGraphScheduleSimpleLoop.after.dom.dot");
    FuncSchedule->dumpDomTreeGraphviz(OF);
  }
}
