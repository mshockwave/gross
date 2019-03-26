#include "gross/Graph/Reductions/CSE.h"
#include "gross/Graph/Node.h"
#include "gross/Graph/NodeUtils.h"
#include <utility>
#include <vector>

using namespace gross;

typename CSEReducer::node_hash_type
CSEReducer::GetNodeHash(Node* N) {
  // only process arithmetic node here
  assert(!N->getNumEffectInput() &&
         !N->getNumControlInput());

  size_t seed = 0U;
  boost::hash_combine(
    seed, std::hash<unsigned>{}(static_cast<unsigned>(N->getOp()))
  );

  NodeProperties<IrOpcode::VirtBinOps> BNP(N);
  std::hash<Node*> Hasher;
  if(BNP && BNP.IsCommutative()) {
    // sort hash value first
    size_t Hashes[2] = { Hasher(BNP.LHS()), Hasher(BNP.RHS()) };
    if(Hashes[0] > Hashes[1]) {
      boost::hash_combine(seed, Hashes[1]);
      boost::hash_combine(seed, Hashes[0]);
    } else {
      boost::hash_combine(seed, Hashes[0]);
      boost::hash_combine(seed, Hashes[1]);
    }
    return seed;
  } else {
    // order of inputs matter
    for(auto* VI : N->value_inputs()) {
      auto HashVal = Hasher(VI);
      boost::hash_combine(seed, HashVal);
    }
    return seed;
  }
}

CSEReducer::CSEReducer(GraphEditor::Interface* editor)
  : GraphEditor(editor),
    G(Editor->GetGraph()) {}

void CSEReducer::RevisitNodes(unsigned OC, Node* Except) {
  auto& Nodes = NodeOpMap[OC];
  for(auto* N : Nodes) {
    if(N == Except) continue;
    Revisit(N);
  }
}

GraphReduction CSEReducer::ReduceArithmetic(Node* N) {
  if(N->getNumEffectInput() ||
     N->getNumControlInput())
    return NoChange();

  auto OC = static_cast<unsigned>(N->getOp());
  if(!NodeOpMap[OC].count(N))
    NodeOpMap[OC].insert(N);

  auto NewHash = GetNodeHash(N);
  if(auto* NewNode = NodeHashMap.find_node(NewHash)) {
    if(NewNode != N) {
      // replace with new node
      NodeHashMap.erase(N);
      NodeOpMap[OC].erase(N);
      RevisitNodes(OC, N);
      return Replace(NewNode);
    }
  }

  if(auto* OldHash = NodeHashMap.find_value(N)) {
    if(NewHash != *OldHash) {
      RevisitNodes(OC, N);
    } else {
      // no hash update
      return NoChange();
    }
  }
  NodeHashMap.insert({N, NewHash}, true);
  return NoChange();
}

// Scan all the MemLoad users from the last MemStore
// and merge if others have identical offset node
GraphReduction CSEReducer::ReduceMemoryLoad(Node* N) {
  if(N->getNumEffectInput() != 1) return NoChange();
  auto* LastStore = N->getEffectInput(0);
  NodeProperties<IrOpcode::VirtMemOps> RefNP(N);

  std::vector<Node*> Worklist;
  for(auto* EU : LastStore->effect_users()) {
    if(EU->getOp() != IrOpcode::MemLoad) continue;
    if(EU == N) continue;
    NodeProperties<IrOpcode::VirtMemOps> MNP(EU);
    assert(MNP.BaseAddr() == RefNP.BaseAddr());
    // just do pointer compare as other optimization
    // (e.g. peephole, arithmetic CSE) would take care
    // for us
    if(MNP.Offset() == RefNP.Offset())
      Worklist.push_back(EU);
  }
  for(auto* Orig : Worklist) {
    Replace(Orig, N);
  }
  return NoChange();
}

GraphReduction CSEReducer::Reduce(Node* N) {
  switch(N->getOp()) {
#define COMMON_OP(OC) \
  case IrOpcode::OC:
#include "gross/Graph/Opcodes.def"
    return ReduceArithmetic(N);
  case IrOpcode::MemLoad:
    return ReduceMemoryLoad(N);
  default:
    return NoChange();
  }
}

