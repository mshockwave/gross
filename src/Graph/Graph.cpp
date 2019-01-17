#include "gross/Graph/Graph.h"

using namespace gross;

void Graph::InsertNode(Node* N) {
  Nodes.emplace_back(N);

  // insert edges
  for(size_t i = 0, Num = N->getNumValueInput(); i < Num; ++i) {
    Node* Dest = N->getValueInput(i);
    Edges.insert(Use(N, Dest, Use::K_VALUE));
  }
  for(size_t i = 0, Num = N->getNumControlInput(); i < Num; ++i) {
    Node* Dest = N->getControlInput(i);
    Edges.insert(Use(N, Dest, Use::K_CONTROL));
  }
  for(size_t i = 0, Num = N->getNumEffectInput(); i < Num; ++i) {
    Node* Dest = N->getEffectInput(i);
    Edges.insert(Use(N, Dest, Use::K_EFFECT));
  }
}
