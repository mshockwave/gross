#ifndef GROSS_GRAPH_REDUCTIONS_CSE_H
#define GROSS_GRAPH_REDUCTIONS_CSE_H
#include "gross/Graph/GraphReducer.h"
#include "gross/Graph/Node.h"
#include <unordered_map>
#include <set>

namespace gross {
class CSEReducer : public GraphEditor {
  Graph& G;

  using node_hash_type = size_t;
  node_hash_type GetNodeHash(Node* N);

  // IrOpcode::ID -> Nodes w/ corresponding opcode
  std::unordered_map<unsigned, std::set<Node*>> NodeOpMap;
  NodeBiMap<node_hash_type> NodeHashMap;

  void RevisitNodes(unsigned OC, Node* Except);

  GraphReduction ReduceArithmetic(Node* N);
  GraphReduction ReduceMemoryLoad(Node* N);

public:
  static constexpr
  const char* name() { return "cse"; }

  CSEReducer(GraphEditor::Interface* editor);

  GraphReduction Reduce(Node* N);
};
} // end namespace gross
#endif
