#include "Frontend/Parser.h"
#include "gross/Graph/NodeUtils.h"
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

static std::string MakeName(size_t Idx,
                            const char* Base, const char* Ext) {
  std::stringstream SS;
  SS << Base << Idx << "." << Ext;
  return SS.str();
}

static std::string MakeName(size_t Idx, size_t Idx2,
                            const char* Base, const char* Ext) {
  std::stringstream SS;
  SS << Base << Idx << "-" << Idx2 << "." << Ext;
  return SS.str();
}

TEST(FullPipelineIntegrateTest, TestBasicCtrlStructures) {
  for(auto Idx = 1; Idx <= 2; ++Idx) {
    std::ifstream IF(MakeName(Idx, "full_pipeline", "txt"));

    Graph G;
    Parser P(IF, G);
    (void) P.getLexer().getNextToken();
    P.NewSymScope(); // global scope

    ASSERT_TRUE(P.ParseFuncDecl());
    {
      std::ofstream OF(MakeName(Idx,
                                "TestFullBasicCtrlStructure", "parsed.dot"));
      G.dumpGraphviz(OF);
    }

    // Middle-end
    GraphReducer::RunWithEditor<ValuePromotion>(G);
    GraphReducer::RunWithEditor<MemoryLegalize>(G);
    GraphReducer::RunWithEditor<PeepholeReducer>(G);
    {
      std::ofstream OF(MakeName(Idx,
                                "TestFullBasicCtrlStructure", "peephole.dot"));
      G.dumpGraphviz(OF);
    }
    GraphReducer::RunWithEditor<CSEReducer>(G);

    // Preparation
    DLXMemoryLegalize DLXMemLegalize(G);
    DLXMemLegalize.Run();
    GraphReducer::RunWithEditor<PeepholeReducer>(G);
    {
      std::ofstream OF(MakeName(Idx,
                                "TestFullBasicCtrlStructure",
                                "dlxmemlegalize.dot"));
      G.dumpGraphviz(OF);
    }
    GraphReducer::RunWithEditor<PreMachineLowering>(G);
    // Lower to CFG
    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    size_t Counter = 1;
    for(auto* FuncSchedule : Scheduler.schedules()) {
      std::ofstream OFSchedule(MakeName(Idx, Counter,
                                        "TestFullBasicCtrlStructure",
                                        "scheduled.dot"));
      FuncSchedule->dumpGraphviz(OFSchedule);

      PostMachineLowering PostLowering(*FuncSchedule);
      PostLowering.Run();
      std::ofstream OFPostLower(MakeName(Idx, Counter,
                                         "TestFullBasicCtrlStructure",
                                         "postlower.dot"));
      FuncSchedule->dumpGraphviz(OFPostLower);

      LinearScanRegisterAllocator<CompactDLXTargetTraits> RA(*FuncSchedule);
      RA.Allocate();
      std::ofstream OFRA(MakeName(Idx, Counter,
                                  "TestFullBasicCtrlStructure",
                                  "ra.dot"));
      FuncSchedule->dumpGraphviz(OFRA);

      PostRALowering PostRA(*FuncSchedule);
      PostRA.Run();
      std::ofstream OFPostRA(MakeName(Idx, Counter++,
                                      "TestFullBasicCtrlStructure",
                                      "postra.dot"));
      FuncSchedule->dumpGraphviz(OFPostRA);
    }
  }
}

TEST(FullPipelineIntegrateTest, TestComplexCtrlStructures) {
  for(auto Idx = 1; Idx <= 1; ++Idx) {
    std::ifstream IF(MakeName(Idx + 2, "full_pipeline", "txt"));

    Graph G;
    Parser P(IF, G);
    (void) P.getLexer().getNextToken();
    P.NewSymScope(); // global scope

    ASSERT_TRUE(P.ParseFuncDecl());
    {
      std::ofstream OF(MakeName(Idx,
                                "TestFullComplexCtrlStructure", "parsed.dot"));
      G.dumpGraphviz(OF);
    }

    // Middle-end
    GraphReducer::RunWithEditor<ValuePromotion>(G);
    GraphReducer::RunWithEditor<MemoryLegalize>(G);
    {
      std::ofstream OF(MakeName(Idx,
                                "TestFullComplexCtrlStructure", "mem2reg.dot"));
      G.dumpGraphviz(OF);
    }
    GraphReducer::RunWithEditor<PeepholeReducer>(G);
    {
      std::ofstream OF(MakeName(Idx,
                                "TestFullComplexCtrlStructure", "peephole.dot"));
      G.dumpGraphviz(OF);
    }
    GraphReducer::RunWithEditor<CSEReducer>(G);

    // Preparation
    DLXMemoryLegalize DLXMemLegalize(G);
    DLXMemLegalize.Run();
    GraphReducer::RunWithEditor<PeepholeReducer>(G);
    {
      std::ofstream OF(MakeName(Idx,
                                "TestFullComplexCtrlStructure",
                                "dlxmemlegalize.dot"));
      G.dumpGraphviz(OF);
    }
    GraphReducer::RunWithEditor<PreMachineLowering>(G);
    // Lower to CFG
    GraphScheduler Scheduler(G);
    Scheduler.ComputeScheduledGraph();
    size_t Counter = 1;
    for(auto* FuncSchedule : Scheduler.schedules()) {
      std::ofstream OFSchedule(MakeName(Idx, Counter,
                                        "TestFullComplexCtrlStructure",
                                        "scheduled.dot"));
      FuncSchedule->dumpGraphviz(OFSchedule);

      PostMachineLowering PostLowering(*FuncSchedule);
      PostLowering.Run();
      std::ofstream OFPostLower(MakeName(Idx, Counter,
                                         "TestFullComplexCtrlStructure",
                                         "postlower.dot"));
      FuncSchedule->dumpGraphviz(OFPostLower);

      LinearScanRegisterAllocator<CompactDLXTargetTraits> RA(*FuncSchedule);
      RA.Allocate();
      std::ofstream OFRA(MakeName(Idx, Counter,
                                  "TestFullComplexCtrlStructure",
                                  "ra.dot"));
      FuncSchedule->dumpGraphviz(OFRA);

      PostRALowering PostRA(*FuncSchedule);
      PostRA.Run();
      std::ofstream OFPostRA(MakeName(Idx, Counter++,
                                      "TestFullComplexCtrlStructure",
                                      "postra.dot"));
      FuncSchedule->dumpGraphviz(OFPostRA);
    }
  }
}
