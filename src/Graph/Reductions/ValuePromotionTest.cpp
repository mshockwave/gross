#include "gross/Graph/Reductions/ValuePromotion.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Graph/Graph.h"
#include "gross/Support/STLExtras.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(GRValuePromotionUnitTest, SimpleValuePromotionTest) {
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

  GraphReducer::RunWithEditor<ValuePromotion>(G);
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
}

TEST(GRValuePromotionUnitTest, MultipleAssignTest) {
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

  GraphReducer::RunWithEditor<ValuePromotion>(G);
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

TEST(GRValuePromotionUnitTest, ArrayReadTest) {
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

    GraphReducer::RunWithEditor<ValuePromotion>(G);
    {
      std::ofstream OF("TestMem2RegArrayRead1.after.dot");
      G.dumpGraphviz(OF);
    }
  }
}

TEST(GRValuePromotionUnitTest, ArrayWriteTest) {
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

    GraphReducer::RunWithEditor<ValuePromotion>(G);
    {
      std::ofstream OF("TestMem2RegArrayWrite1.after.dot");
      G.dumpGraphviz(OF);
    }
  }
  {
    Graph G;
    // array write within control structure
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_mem2reg5")
                 .Build();
    auto* ArrayDecl = NodeBuilder<IrOpcode::SrcArrayDecl>(&G)
                      .SetSymbolName("barArray")
                      .AddConstDim(94)
                      .AddConstDim(87)
                      .Build();
    auto* Pred = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
    auto* IfBranch = NodeBuilder<IrOpcode::If>(&G)
                     .Condition(Pred).Build();
    auto* IfTrue = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                   .IfStmt(IfBranch)
                   .Build();
    auto* IfFalse = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                    .IfStmt(IfBranch)
                    .Build();

    auto* Dim1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 9).Build();
    auto* Dim2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
    auto* Dim3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 8).Build();
    auto* Dim4 = NodeBuilder<IrOpcode::ConstantInt>(&G, 7).Build();

    auto* ArrayAccess1 = NodeBuilder<IrOpcode::SrcArrayAccess>(&G)
                        .Decl(ArrayDecl)
                        .AppendAccessDim(Dim1)
                        .AppendAccessDim(Dim2)
                        .Build();
    auto* RHSVal1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Assign1 = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                   .Dest(ArrayAccess1).Src(RHSVal1)
                   .Build();
    Assign1->appendControlInput(IfTrue);

    auto* ArrayAccess2 = NodeBuilder<IrOpcode::SrcArrayAccess>(&G)
                        .Decl(ArrayDecl)
                        .AppendAccessDim(Dim3)
                        .AppendAccessDim(Dim4)
                        .Build();
    auto* RHSVal2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* Assign2 = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                   .Dest(ArrayAccess2).Src(RHSVal2)
                   .Build();
    Assign2->appendControlInput(IfFalse);

    auto* Merge = NodeBuilder<IrOpcode::Merge>(&G)
                  .AddCtrlInput(IfTrue).AddCtrlInput(IfFalse)
                  .Build();
    auto* PHINode = NodeBuilder<IrOpcode::Phi>(&G)
                    .SetCtrlMerge(Merge)
                    .AddEffectInput(Assign1)
                    .AddEffectInput(Assign2)
                    .Build();

    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Merge)
                .AddEffectDep(PHINode)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    {
      std::ofstream OF("TestMem2RegArrayWrite2.dot");
      G.dumpGraphviz(OF);
    }

    GraphReducer::RunWithEditor<ValuePromotion>(G);
    {
      std::ofstream OF("TestMem2RegArrayWrite2.after.dot");
      G.dumpGraphviz(OF);
    }
  }
}
