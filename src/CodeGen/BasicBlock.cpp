#include "gross/CodeGen/BasicBlock.h"
#include "gross/Support/STLExtras.h"

using namespace gross;

BasicBlock::SeqNodeId* BasicBlock::getNodeId(Node* N) const {
  if(NodeIds.count(N))
    return const_cast<SeqNodeId*>(&NodeIds.at(N));
  else
    return nullptr;
}

size_t BasicBlock::getNodeIndex(Node* N) const {
  auto Idx = 0U;
  for(auto I = NodeSequence.begin(), E = NodeSequence.end(); I != E;
      ++I, ++Idx) {
    if(N == *I) return Idx;
  }
  gross_unreachable("Node is not in BB?");
  return 0;
}

bool BasicBlock::HasPredBlock(BasicBlock* BB) {
  return gross::find(Predecessors, BB) != Predecessors.end();
}
bool BasicBlock::RemovePredBlock(BasicBlock* BB) {
  auto Pos = gross::find(Predecessors, BB);
  if(Pos != Predecessors.end()) {
    Predecessors.erase(Pos);
    return true;
  } else {
    return false;
  }
}

bool BasicBlock::HasSuccBlock(BasicBlock* BB) {
  return gross::find(Successors, BB) != Successors.end();
}
bool BasicBlock::RemoveSuccBlock(BasicBlock* BB) {
  auto Pos = gross::find(Successors, BB);
  if(Pos != Successors.end()) {
    Successors.erase(Pos);
    return true;
  } else {
    return false;
  }
}

void BasicBlock::AddNode(typename BasicBlock::node_iterator Pos,
                         Node* N) {
  LastNodeId = SeqNodeId::AdvanceFrom(LastNodeId);
  NodeIds.insert({N, LastNodeId});
  NodeSequence.insert(Pos, N);
}

bool BasicBlock::AddNodeBefore(Node* Before, Node* N) {
  auto NodeIt = gross::find_if(nodes(),
                               [=](Node* Target) -> bool {
                                 return Target == Before;
                               });
  if(NodeIt == node_end()) return false;
  AddNode(NodeIt, N);
  return true;
}

bool BasicBlock::AddNodeAfter(Node* After, Node* N) {
  auto NodeIt = gross::find_if(nodes(),
                               [=](Node* Target) -> bool {
                                 return Target == After;
                               });
  if(NodeIt == node_end()) return false;
  AddNode(++NodeIt, N);
  return true;
}

std::pair<bool, typename BasicBlock::node_iterator>
BasicBlock::RemoveNode(Node* N) {
  auto NodeIt = gross::find_if(nodes(),
                               [=](Node* Target) -> bool {
                                 return Target == N;
                               });
  if(NodeIt == node_end())
    return std::make_pair(false, node_end());
  NodeIds.erase(N);
  return std::make_pair(true, NodeSequence.erase(NodeIt));
}

void BasicBlock::ReplaceNode(Node* Old, Node* New) {
  node_iterator Pos;
  bool Exist;
  std::tie(Exist, Pos) = RemoveNode(Old);
  if(Exist) {
    AddNode(Pos, New);
  }
}
