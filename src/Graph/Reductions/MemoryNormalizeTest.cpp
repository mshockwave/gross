#include "gross/Graph/Reductions/MemoryNormalize.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(GRMemoryNormalizeTest, LinearMemOps) {
  Graph G;
  auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
               .FuncName("func_mem_normalize1")
               .Build();
  auto* Const10 = NodeBuilder<IrOpcode::ConstantInt>(&G, 10).Build();
  auto* Const5 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5).Build();
  auto* Const9 = NodeBuilder<IrOpcode::ConstantInt>(&G, 9).Build();
  auto* Alloca = NodeBuilder<IrOpcode::Alloca>(&G)
                 .Size(Const10).Build();
  // Store1
  // Load1
  // Load2
  // Store2
  // Store3
  auto* Store1 = NodeBuilder<IrOpcode::MemStore>(&G)
                 .BaseAddr(Alloca).Offset(Const5)
                 .Src(Const9).Build();
  Store1->appendControlInput(Func);
  auto* Load1 = NodeBuilder<IrOpcode::MemLoad>(&G)
                .BaseAddr(Alloca).Offset(Const5).Build();
  Load1->appendEffectInput(Store1);
  auto* Load2 = NodeBuilder<IrOpcode::MemLoad>(&G)
                .BaseAddr(Alloca).Offset(Const5).Build();
  Load2->appendEffectInput(Store1);
  auto* Store2 = NodeBuilder<IrOpcode::MemStore>(&G)
                 .BaseAddr(Alloca).Offset(Const5)
                 .Src(Const9).Build();
  Store2->appendEffectInput(Store1);
  auto* Store3 = NodeBuilder<IrOpcode::MemStore>(&G)
                 .BaseAddr(Alloca).Offset(Const5)
                 .Src(Const9).Build();
  Store3->appendEffectInput(Store2);

  auto* Sum = NodeBuilder<IrOpcode::BinAdd>(&G)
              .LHS(Load1).RHS(Load2)
              .Build();
  auto* Return = NodeBuilder<IrOpcode::Return>(&G, Sum)
                 .Build();
  auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
              .AddEffectDep(Store3)
              .AddTerminator(Return).Build();
  SubGraph FuncSG(End);
  G.AddSubRegion(FuncSG);
  {
    std::ofstream OF("TestMemNormalizeLinear.dot");
    G.dumpGraphviz(OF);
  }

  GraphReducer::RunWithEditor<MemoryNormalize>(G);
  {
    std::ofstream OF("TestMemNormalizeLinear.after.dot");
    G.dumpGraphviz(OF);
  }
}

TEST(GRMemoryNormalizeTest, ComplexCtrlMemOps) {
}
