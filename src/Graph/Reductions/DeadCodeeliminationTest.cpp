#include "gross/Graph/Reductions/DeadCodeElimination.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/Node.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(GRDCETest, DeadEliminationTest) {
  Graph G;
  auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
               .FuncName("func_dce1")
               .Build();
  auto* Dead = NodeBuilder<IrOpcode::Dead>(&G).Build();
  // value dep on dead
  auto* Return = NodeBuilder<IrOpcode::Return>(&G, Dead)
                 .Build();
  Return->appendControlInput(Func);
  // control dep on dead
  auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
              .AddTerminator(Return)
              .AddTerminator(Dead)
              .Build();
  G.AddSubRegion(SubGraph(End));
  {
    std::ofstream OF("DCEDeadTest1.dot");
    G.dumpGraphviz(OF);
  }

  RunReducer<DCEReducer>(G);
  {
    std::ofstream OF("DCEDeadTest1.after.dot");
    G.dumpGraphviz(OF);
  }
}
