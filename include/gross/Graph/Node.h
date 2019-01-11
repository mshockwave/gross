#ifndef GROSS_GRAPH_NODE_H
#define GROSS_GRAPH_NODE_H
#include "gross/Graph/Opcodes.h"
#include "gross/Support/Log.h"
#include <unordered_map>
#include <vector>

namespace gross {
// Forward declarations
namespace _details {
template<IrOpcode::ID OC,class SubT>
struct BinOpNodeBuilder;
} // end namespace _details

class Node {
  template<IrOpcode::ID>
  friend class NodePropertiesBase;
  template<IrOpcode::ID>
  friend class NodeProperties;
  template<IrOpcode::ID>
  friend class NodeBuilder;
  template<IrOpcode::ID OC,class SubT>
  friend struct _details::BinOpNodeBuilder;

  IrOpcode::ID Op;

  unsigned NumValueInput;
  unsigned NumControlInput;
  unsigned NumEffectInput;

  std::vector<Node*> Inputs;

  std::vector<Node*> Users;

public:
  IrOpcode::ID getOp() const { return Op; }

  inline
  unsigned getNumValueInput() const { return NumValueInput; }
  inline
  unsigned getNumControlInput() const { return NumControlInput; }
  inline
  unsigned getNumEffectInput() const { return NumEffectInput; }

  Node* getValueInput(unsigned Index) const {
    assert(Index < NumValueInput);
    return Inputs.at(Index);
  }

  Node* getControlInput(unsigned Index) const {
    assert(Index < NumControlInput);
    return Inputs.at(NumValueInput + Index);
  }

  Node* getEffectInput(unsigned Index) const {
    assert(Index < NumEffectInput);
    return Inputs.at(NumValueInput + NumControlInput + Index);
  }

  Node()
    : Op(IrOpcode::None),
      NumValueInput(0),
      NumControlInput(0),
      NumEffectInput(0) {}

  Node(IrOpcode::ID OC)
    : Op(OC),
      NumValueInput(0),
      NumControlInput(0),
      NumEffectInput(0) {}

  Node(IrOpcode::ID OC,
       const std::vector<Node*>& Values,
       const std::vector<Node*>& Controls = {},
       const std::vector<Node*>& Effects = {});
};

template<typename ValueT>
class NodeBiMap {
  std::unordered_map<Node*, ValueT> Node2Value;
  using N2VPairTy
    = typename decltype(Node2Value)::value_type;
  std::unordered_map<ValueT, Node*> Value2Node;

public:
  ValueT* find_value(Node* N) {
    return Node2Value.count(N)?
            &Node2Value.at(N) :
            nullptr;
  }

  Node* find_node(const ValueT& V) {
    return Value2Node.count(V)?
            Value2Node.at(V) :
            nullptr;
  }

  bool insert(const N2VPairTy& Pair) {
    if(Node2Value.count(Pair.first) ||
       Value2Node.count(Pair.second))
      return false;
    Node2Value.insert(Pair);
    Value2Node.insert(std::make_pair(Pair.second, Pair.first));
    return true;
  }

  size_t size() const { return Value2Node.size(); }
};
} // end namespace gross
#endif
