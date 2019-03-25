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
#include "gross/Support/Log.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cxxopts.hpp>

using namespace gross;

static std::string MakeName(const std::string& Base, const char* Ext) {
  std::stringstream SS;
  SS << Base << "." << Ext;
  return SS.str();
}
static std::string MakeName(size_t Idx,
                            const std::string& Base, const char* Ext) {
  std::stringstream SS;
  SS << Base << "." << Idx << "." << Ext;
  return SS.str();
}

static void InitializeCLIOptions(cxxopts::Options& Opts) {
  Opts.add_options()
    ("v,version", "Show version")
    ("h,help", "Show this message")
    ("i,input", "Input file", cxxopts::value<std::string>())
    ("dump-hl", "Dump high-level graph (looks really like AST)")
    ("dump-mem2reg", "Result after ValuePromotion")
    ("dump-peephole", "Result after peephole optimization")
    ("dump-cse", "Result after CSE")
    ("dump-pre-lowering", "Result after PreMachineLowering")
    ("dump-scheduled", "Scheduled graph")
    ("dump-post-lowering", "Result after PostMachineLowering")
    ("dump-ra", "Register allocated graph")
    ("dump-post-ra", "Result after PostRALowering");
  Opts.parse_positional({"input"});
}

int main(int argc, char** argv) {
  cxxopts::Options CLIOpts("gross", "GRaph Optimizer Soley for cS241");
  InitializeCLIOptions(CLIOpts);
  auto GrossOpts = CLIOpts.parse(argc, argv);

  if(GrossOpts.count("help")) {
    std::cout << CLIOpts.help() << "\n";
    return 0;
  } else if(GrossOpts.count("version")) {
    std::cout << "v0.94.87\n";
    return 0;
  }

  if(!GrossOpts.count("input")) {
    Log::E() << "Required input file\n";
    std::cout << CLIOpts.help() << "\n";
    return 1;
  }
  auto InputFileName = GrossOpts["input"].as<std::string>();
  std::ifstream IF(InputFileName);
  if(!IF) {
    Log::E() << "Failed to open file " << InputFileName << "\n";
    return 1;
  }

  Graph G;
  Parser P(IF, G);

  if(!P.Parse(true)) {
    Log::E() << "Failed to parse\n";
    return 1;
  }
  if(GrossOpts.count("dump-hl")) {
    std::ofstream OF(MakeName(InputFileName, "parsed.dot"));
    G.dumpGraphviz(OF);
  }

  // Middle-end
  GraphReducer::RunWithEditor<ValuePromotion>(G);
  GraphReducer::RunWithEditor<MemoryLegalize>(G);
  if(GrossOpts.count("dump-mem2reg")) {
    std::ofstream OF(MakeName(InputFileName, "mem2reg.dot"));
    G.dumpGraphviz(OF);
  }
  GraphReducer::RunWithEditor<PeepholeReducer>(G);
  if(GrossOpts.count("dump-peephole")) {
    std::ofstream OF(MakeName(InputFileName, "peephole.dot"));
    G.dumpGraphviz(OF);
  }
  GraphReducer::RunWithEditor<CSEReducer>(G);
  if(GrossOpts.count("dump-cse")) {
    std::ofstream OF(MakeName(InputFileName, "cse.dot"));
    G.dumpGraphviz(OF);
  }

  // Preparation
  DLXMemoryLegalize DLXMemLegalize(G);
  DLXMemLegalize.Run();
  if(GrossOpts.count("dump-pre-lowering")) {
    std::ofstream OF(MakeName(InputFileName, "prelower1.dot"));
    G.dumpGraphviz(OF);
  }
  GraphReducer::RunWithEditor<PeepholeReducer>(G);
  if(GrossOpts.count("dump-pre-lowering")) {
    std::ofstream OF(MakeName(InputFileName, "prelower2.dot"));
    G.dumpGraphviz(OF);
  }
  GraphReducer::RunWithEditor<PreMachineLowering>(G);
  if(GrossOpts.count("dump-pre-lowering")) {
    std::ofstream OF(MakeName(InputFileName, "prelower3.dot"));
    G.dumpGraphviz(OF);
  }
  // Lower to CFG
  GraphScheduler Scheduler(G);
  Scheduler.ComputeScheduledGraph();
  size_t Counter = 1;
  for(auto* FuncSchedule : Scheduler.schedules()) {
    if(GrossOpts.count("dump-scheduled")) {
      std::ofstream OF(MakeName(Counter,
                                InputFileName, "scheduled.dot"));
      FuncSchedule->dumpGraphviz(OF);
    }

    PostMachineLowering PostLowering(*FuncSchedule);
    PostLowering.Run();
    if(GrossOpts.count("dump-post-lowering")) {
      std::ofstream OF(MakeName(Counter,
                                InputFileName, "postlower.dot"));
      FuncSchedule->dumpGraphviz(OF);
    }

    LinearScanRegisterAllocator<CompactDLXTargetTraits> RA(*FuncSchedule);
    RA.Allocate();
    if(GrossOpts.count("dump-ra")) {
      std::ofstream OF(MakeName(Counter,
                                InputFileName, "ra.dot"));
      FuncSchedule->dumpGraphviz(OF);
    }

    PostRALowering PostRA(*FuncSchedule);
    PostRA.Run();
    if(GrossOpts.count("dump-post-ra")) {
      std::ofstream OF(MakeName(Counter,
                                InputFileName, "postra.dot"));
      FuncSchedule->dumpGraphviz(OF);
    }
    Counter++;
  }
  return 0;
}
