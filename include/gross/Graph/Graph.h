#ifndef GROSS_GRAPH_GRAPH_H
#define GROSS_GRAPH_GRAPH_H
#include "gross/Graph/Node.h"
#include <vector>
#include <memory>

namespace gross {
// Owner of nodes
class Graph {
  std::vector<std::unique_ptr<Node>> Nodes;

public:
};
} // end namespace gross
#endif
