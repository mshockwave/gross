#include "gross/Graph/Reductions/CSE.h"
//#include "gross/Graph/Reductions/ValuePromotion.h"
//#include "gross/Graph/Reductions/DeadCodeElimination.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(GRCSEUnitTest, TrivialValDepsTest) {
  Graph G;
  auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
               .FuncName("func_cse1")
               .Build();
  auto* VarDecl1 = NodeBuilder<IrOpcode::SrcVarDecl>(&G)
                   .SetSymbolName("foo")
                   .Build();
  auto* VarDecl2 = NodeBuilder<IrOpcode::SrcVarDecl>(&G)
                   .SetSymbolName("bar")
                   .Build();
  auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 87)
                 .Build();
  auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 94)
                 .Build();
  auto* RHSVal1 = NodeBuilder<IrOpcode::BinAdd>(&G)
                  .LHS(Const1).RHS(Const2)
                  .Build();
  auto* VarAccess1 = NodeBuilder<IrOpcode::SrcVarAccess>(&G)
                     .Decl(VarDecl1)
                     .Build();
  auto* Assign1 = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                  .Dest(VarAccess1).Src(RHSVal1)
                  .Build();
  Assign1->appendControlInput(Func);

  auto* RHSVal2 = NodeBuilder<IrOpcode::BinAdd>(&G)
                  .LHS(Const1).RHS(Const2)
                  .Build();
  auto* VarAccess2 = NodeBuilder<IrOpcode::SrcVarAccess>(&G)
                    .Decl(VarDecl2)
                    .Effect(Assign1)
                    .Build();
  auto* Assign2 = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                  .Dest(VarAccess2).Src(RHSVal2)
                  .Build();
  auto* VarAccess3 = NodeBuilder<IrOpcode::SrcVarAccess>(&G)
                    .Decl(VarDecl2)
                    .Effect(Assign2)
                    .Build();
  auto* Return = NodeBuilder<IrOpcode::Return>(&G, VarAccess3)
                 .Build();
  auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
              .AddTerminator(Return)
              .Build();
  SubGraph FuncSG(End);
  G.AddSubRegion(FuncSG);
  {
    std::ofstream OF("TestTrivialValCSE.dot");
    G.dumpGraphviz(OF);
  }

  RunReducer<CSEReducer>(G, G);
  {
    std::ofstream OF("TestTrivialValCSE.after.dot");
    G.dumpGraphviz(OF);
  }

  /*
  RunReducer<ValuePromotion>(G, G);
  {
    std::ofstream OF("TestTrivialValCSE.vp.dot");
    G.dumpGraphviz(OF);
  }
  RunGlobalReducer<DCEReducer>(G);
  {
    std::ofstream OF("TestTrivialValCSE.dce.dot");
    G.dumpGraphviz(OF);
  }
  */
}
