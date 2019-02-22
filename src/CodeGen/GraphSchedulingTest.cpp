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

TEST(CodeGenUnitTest, TestGraphScheduleDump) {
  Graph G;
  auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
               .FuncName("func")
               .Build();
  auto* VarDecl = NodeBuilder<IrOpcode::SrcVarDecl>(&G)
                  .SetSymbolName("foo")
                  .Build();
  auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 87)
                 .Build();
  auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 94)
                 .Build();
  auto* RHSVal = NodeBuilder<IrOpcode::BinAdd>(&G)
                 .LHS(Const1).RHS(Const2)
                 .Build();
  auto* VarAccessDest = NodeBuilder<IrOpcode::SrcVarAccess>(&G)
                        .Decl(VarDecl)
                        .Build();
  auto* Assign = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                 .Dest(VarAccessDest).Src(RHSVal)
                 .Build();
  Assign->appendControlInput(Func);
  auto* VarAccess = NodeBuilder<IrOpcode::SrcVarAccess>(&G)
                    .Decl(VarDecl)
                    .Effect(Assign)
                    .Build();
  auto* Return = NodeBuilder<IrOpcode::Return>(&G, VarAccess)
                 .Build();
  auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
              .AddTerminator(Return)
              .Build();
  SubGraph FuncSG(End);
  G.AddSubRegion(FuncSG);
  {
    std::ofstream OF("TestGraphScheduleDump.dot");
    G.dumpGraphviz(OF);
  }

  GraphScheduler Scheduler(G);
  Scheduler.ComputeScheduledGraph();
  EXPECT_EQ(Scheduler.schedule_size(), 1);
  auto* FuncSchedule = *Scheduler.schedule_begin();
  {
    std::ofstream OF("TestGraphScheduleDump.func.dot");
    FuncSchedule->dumpGraphviz(OF);
  }
}
