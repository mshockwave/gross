#include "gross/Graph/Reductions/CSE.h"
#include "gross/Graph/Node.h"
#include "gross/Graph/NodeUtils.h"
#include <vector>

using namespace gross;

GraphReduction CSEReducer::Reduce(Node* N) {
  return ReduceTrivialValues(N);
}

static bool onlyGlobalValDeps(Node* N) {
  // recusive search using simple BFS
  std::vector<Node*> BFSQueue;
  BFSQueue.push_back(N);
  while(!BFSQueue.empty()) {
    auto* Top = BFSQueue.front();
    BFSQueue.erase(BFSQueue.cbegin());
    // can't have control/effect dep
    if(Top->getNumControlInput() ||
       Top->getNumEffectInput()) return false;
    // global values, stop
    if(NodeProperties<IrOpcode::VirtGlobalValues>(Top))
      continue;
    // push value inputs
    for(auto* V : Top->value_inputs())
      BFSQueue.push_back(V);
  }
  return true;
}

GraphReduction CSEReducer::ReduceTrivialValues(Node* N) {
  if(NodeProperties<IrOpcode::VirtGlobalValues>(N))
    return NoChange();

  // Case 1. has only value dependency on global value(e.g. ConstInt/Str)
  auto TI = TrivialVals.find(NodeHandle{N});
  if(TI != TrivialVals.end()) {
    auto* ReplaceNode = TI->NodePtr;
    if(N == ReplaceNode) return NoChange();

    return Replace(ReplaceNode);
  } else if(onlyGlobalValDeps(N)) {
    TrivialVals.insert(NodeHandle{N});
  }
  return NoChange();
}
