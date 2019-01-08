#include "gross/Graph/Graph.h"

using namespace gross;

void Graph::InsertNode(Node* N) {
  Nodes.emplace_back(N);
}
