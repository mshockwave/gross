#include "gross/Graph/Reductions/ValuePromotion.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Graph/Graph.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(GRValuePromotionTest, SimpleValuePromotionTest) {
  Graph G;
  auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
               .FuncName("func_mem2reg1")
               .Build();
  auto* VarDecl = NodeBuilder<IrOpcode::SrcVarDecl>(&G)
                  .SetSymbolName("foo")
                  .Build();
  auto* RHSVal = NodeBuilder<IrOpcode::ConstantInt>(&G, 87)
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
  G.AddSubRegion(SubGraph(End));
  {
    std::ofstream OF("TestMem2regSimple.dot");
    G.dumpGraphviz(OF);
  }

  RunReducer<ValuePromotion>(G, G);
  {
    std::ofstream OF("TestMem2regSimple.after.dot");
    G.dumpGraphviz(OF);
  }
}
