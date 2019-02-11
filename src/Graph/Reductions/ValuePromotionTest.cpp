#include "gross/Graph/Reductions/ValuePromotion.h"
#include "gross/Graph/Reductions/DeadCodeElimination.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Graph/Graph.h"
#include "gross/Support/STLExtras.h"
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
    std::ofstream OF("TestMem2RegSimple.dot");
    G.dumpGraphviz(OF);
  }

  RunReducer<ValuePromotion>(G, G);
  // return, end, start, function name string, bin add
  EXPECT_EQ(FuncSG.node_size(), 7);
  EXPECT_EQ(End->getNumControlInput(), 2);
  EXPECT_NE(gross::find(End->control_inputs(), Return),
            End->control_inputs().end());
  ASSERT_EQ(Return->getNumValueInput(), 1);
  EXPECT_EQ(Return->getValueInput(0), RHSVal);
  {
    std::ofstream OF("TestMem2RegSimple.after.dot");
    G.dumpGraphviz(OF);
  }

  //RunGlobalReducer<DCEReducer>(G);
  //{
    //std::ofstream OF("TestMem2regSimple.dce.dot");
    //G.dumpGraphviz(OF);
  //}
}

TEST(GRValuePromotionTest, MultipleAssignTest) {
  Graph G;
  auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
               .FuncName("func_mem2reg2")
               .Build();
  auto* VarDecl = NodeBuilder<IrOpcode::SrcVarDecl>(&G)
                  .SetSymbolName("foo")
                  .Build();
  auto* Const1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 87)
                 .Build();
  auto* Const2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 94)
                 .Build();
  auto* VarAccessDest1 = NodeBuilder<IrOpcode::SrcVarAccess>(&G)
                         .Decl(VarDecl)
                         .Build();
  auto* Assign1 = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                  .Dest(VarAccessDest1).Src(Const1)
                  .Build();
  Assign1->appendControlInput(Func);

  auto* RHSVal = NodeBuilder<IrOpcode::BinAdd>(&G)
                 .LHS(Const1).RHS(Const2)
                 .Build();
  auto* VarAccess2 = NodeBuilder<IrOpcode::SrcVarAccess>(&G)
                     .Decl(VarDecl)
                     .Effect(Assign1)
                     .Build();
  auto* Assign2 = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                  .Dest(VarAccess2).Src(RHSVal)
                  .Build();

  auto* VarAccess3 = NodeBuilder<IrOpcode::SrcVarAccess>(&G)
                     .Decl(VarDecl)
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
    std::ofstream OF("TestMem2RegMultiAssign.dot");
    G.dumpGraphviz(OF);
  }

  RunReducer<ValuePromotion>(G, G);
  // same assertion as previous testcase
  // return, end, start, function name string, bin add
  EXPECT_EQ(FuncSG.node_size(), 7);
  EXPECT_EQ(End->getNumControlInput(), 2);
  EXPECT_NE(gross::find(End->control_inputs(), Return),
            End->control_inputs().end());
  ASSERT_EQ(Return->getNumValueInput(), 1);
  EXPECT_EQ(Return->getValueInput(0), RHSVal);
  {
    std::ofstream OF("TestMem2RegMultiAssign.after.dot");
    G.dumpGraphviz(OF);
  }

  //RunGlobalReducer<DCEReducer>(G);
  //{
    //std::ofstream OF("TestMem2RegMultiAssign.dce.dot");
    //G.dumpGraphviz(OF);
  //}
}

TEST(GRValuePromotionTest, ArrayReadTest) {
  {
    Graph G;
    // only read array element
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_mem2reg3")
                 .Build();
    auto* ArrayDecl = NodeBuilder<IrOpcode::SrcArrayDecl>(&G)
                      .SetSymbolName("barArray")
                      .AddConstDim(94)
                      .AddConstDim(87)
                      .Build();
    auto* Dim1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5).Build();
    auto* Dim2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 7).Build();
    auto* ArrayAccess = NodeBuilder<IrOpcode::SrcArrayAccess>(&G)
                        .Decl(ArrayDecl)
                        .AppendAccessDim(Dim1)
                        .AppendAccessDim(Dim2)
                        .Build();
    auto* Return = NodeBuilder<IrOpcode::Return>(&G, ArrayAccess).Build();
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Return)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestMem2RegArrayRead1.dot");
      G.dumpGraphviz(OF);
    }

    RunReducer<ValuePromotion>(G, G);
    {
      std::ofstream OF("TestMem2RegArrayRead1.after.dot");
      G.dumpGraphviz(OF);
    }
    RunGlobalReducer<DCEReducer>(G);
    {
      std::ofstream OF("TestMem2RegArrayRead1.dce.dot");
      G.dumpGraphviz(OF);
    }
  }
}

TEST(GRValuePromotionTest, ArrayWriteTest) {
  {
    Graph G;
    // single array element write
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_mem2reg4")
                 .Build();
    auto* ArrayDecl = NodeBuilder<IrOpcode::SrcArrayDecl>(&G)
                      .SetSymbolName("barArray")
                      .AddConstDim(94)
                      .AddConstDim(87)
                      .Build();
    auto* Dim1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5).Build();
    auto* Dim2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 7).Build();
    auto* ArrayAccess = NodeBuilder<IrOpcode::SrcArrayAccess>(&G)
                        .Decl(ArrayDecl)
                        .AppendAccessDim(Dim1)
                        .AppendAccessDim(Dim2)
                        .Build();
    auto* RHSVal = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Assign = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                   .Dest(ArrayAccess).Src(RHSVal)
                   .Build();
    Assign->appendControlInput(Func);
    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddEffectDep(Assign)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestMem2RegArrayWrite1.dot");
      G.dumpGraphviz(OF);
    }

    RunReducer<ValuePromotion>(G, G);
    {
      std::ofstream OF("TestMem2RegArrayWrite1.after.dot");
      G.dumpGraphviz(OF);
    }
    RunGlobalReducer<DCEReducer>(G);
    {
      std::ofstream OF("TestMem2RegArrayWrite1.dce.dot");
      G.dumpGraphviz(OF);
    }
  }
}
