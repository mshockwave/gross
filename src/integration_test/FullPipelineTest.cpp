#include "Frontend/Parser.h"
#include "gross/Graph/Reductions/CSE.h"
#include "gross/Graph/Reductions/Peephole.h"
#include "gross/Graph/Reductions/ValuePromotion.h"
#include "gross/Graph/Reductions/MemoryLegalize.h"
#include "CodeGen/PreMachineLowering.h"
#include "gross/CodeGen/GraphScheduling.h"
#include "CodeGen/PostMachineLowering.h"
#include "CodeGen/RegisterAllocator.h"
#include "CodeGen/Targets.h"
#include "CodeGen/PostRALowering.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(FullPipelineIntegrateTest, TestBasicCtrlStructure) {
  std::ifstream IF("full_pipeline1.txt");

  Graph G;
  Parser P(IF, G);
  (void) P.getLexer().getNextToken();
  P.NewSymScope(); // global scope

  ASSERT_TRUE(P.ParseFuncDecl());
  {
    std::ofstream OF("TestBasicCtrlStructure1.parsed.dot");
    G.dumpGraphviz(OF);
  }

  // Middle-end
  GraphReducer::RunWithEditor<ValuePromotion>(G);
  GraphReducer::RunWithEditor<MemoryLegalize>(G);
  GraphReducer::RunWithEditor<PeepholeReducer>(G);
  {
    std::ofstream OF("TestBasicCtrlStructure1.peephole.dot");
    G.dumpGraphviz(OF);
  }
  GraphReducer::RunWithEditor<CSEReducer>(G);

  // Preparation
  DLXMemoryLegalize DLXMemLegalize(G);
  DLXMemLegalize.Run();
  GraphReducer::RunWithEditor<PeepholeReducer>(G);
  {
    std::ofstream OF("TestBasicCtrlStructure1.dlxmemlegalize.dot");
    G.dumpGraphviz(OF);
  }
  GraphReducer::RunWithEditor<PreMachineLowering>(G);
  // Lower to CFG
  GraphScheduler Scheduler(G);
  Scheduler.ComputeScheduledGraph();
  size_t Counter = 1;
  for(auto* FuncSchedule : Scheduler.schedules()) {
    std::stringstream SS;
    PostMachineLowering PostLowering(*FuncSchedule);
    PostLowering.Run();
    SS << "TestBasicCtrlStructure1-"
       << Counter << ".postlowering.dot";
    std::ofstream OFPostLower(SS.str());
    FuncSchedule->dumpGraphviz(OFPostLower);
    SS.str("");

    SS << "TestBasicCtrlStructure1-"
       << Counter << ".ra.dot";
    LinearScanRegisterAllocator<CompactDLXTargetTraits> RA(*FuncSchedule);
    RA.Allocate();
    std::ofstream OFRA(SS.str());
    FuncSchedule->dumpGraphviz(OFRA);
    SS.str("");

    SS << "TestBasicCtrlStructure1-"
       << Counter++ << ".postra.dot";
    PostRALowering PostRA(*FuncSchedule);
    PostRA.Run();
    std::ofstream OFPostRA(SS.str());
    FuncSchedule->dumpGraphviz(OFPostRA);
  }
}
