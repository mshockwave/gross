#include "PostRALowering.h"
#include "gross/Graph/NodeUtils.h"
#include "DLXNodeUtils.h"

using namespace gross;

PostRALowering::PostRALowering(GraphSchedule& schedule,
                               RAQueryFunctorTy RACallback)
  : Schedule(schedule),
    G(Schedule.getGraph()),
    RAQuery(RACallback) {}

void PostRALowering::Run() {
  // - Peephole optimizations
  //   - Remove PHI nodes
  //   - Remove redundant move
  //   - Remove redundnat load/store
  RunPeepholes();
}

void PostRALowering::RunPeepholes() {
  for(auto* BB : Schedule.rpo_blocks()) {
    bool Changed = false;
    size_t Counter = 0U;
    do {
      // run until a fixpoint
      Changed = VisitBlock(BB);
    } while(Changed &&
            (++Counter) < PeepholeMaxIterations);
  }
}

bool PostRALowering::VisitBlock(BasicBlock* BB) {
  // collect nodes first instead of visiting nodes
  // during iteration
  std::vector<Node*> BlockNodes;
  for(auto* N : BB->nodes())
    BlockNodes.push_back(N);

  bool Changed = false;
  for(auto* N : BlockNodes) {
    switch(N->getOp()) {
    case IrOpcode::DLXAddI:
    case IrOpcode::DLXAdd:
      Changed |= VisitDLXAdd(N);
      break;
    case IrOpcode::Phi:
    case IrOpcode::Merge:
    case IrOpcode::VirtDLXCallsiteBegin:
    case IrOpcode::VirtDLXCallsiteEnd:
      Changed |= TrimNode(N);
      break;
    default: break;
    }
  }
  return Changed;
}

bool PostRALowering::TrimNode(Node* N) {
  auto* BB = Schedule.MapBlock(N);
  if(BB) {
    Schedule.RemoveNode(BB, N);
    return true;
  }
  return false;
}

// constant zero or zero register
inline bool IsZero(Node* N, const Graph& G) {
  if(N->getOp() == IrOpcode::DLXr0) return true;
  return (N->getOp() == IrOpcode::ConstantInt &&
          NodeProperties<IrOpcode::ConstantInt>(N)
          .as<uint32_t>(G) == 0U);
}

bool PostRALowering::VisitDLXAdd(Node* N) {
  auto* BB = Schedule.MapBlock(N);
  if(!BB) return false;

  // remove redundant move
  assert(N->getNumValueInput() == 3);
  // registers have been commited, RHS is the
  // third operand
  if(IsZero(N->getValueInput(2), G) &&
     N->getValueInput(0) == N->getValueInput(1)) {
    // remove
    Schedule.RemoveNode(BB, N);
    return true;
  }
  return false;
}

bool PostRALowering::VisitDLXLoad(Node* N) {
  // TODO
  return false;
}
