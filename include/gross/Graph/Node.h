#ifndef GROSS_GRAPH_NODE_H
#define GROSS_GRAPH_NODE_H
#include "gross/Graph/Opcodes.h"
#include "gross/Support/Log.h"
#include "boost/container_hash/hash.hpp"
#include <unordered_map>
#include <vector>
#include <utility>

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

  void setNodeInput(unsigned Index, unsigned Size, unsigned Offset,
                    Node* NewNode);
  void appendNodeInput(unsigned& Size, unsigned Offset,
                       Node* NewNode);

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

  // modifiers
  void setValueInput(unsigned Index, Node* NewNode);
  void appendValueInput(Node* NewNode);
  void setControlInput(unsigned Index, Node* NewNode);
  void appendControlInput(Node* NewNode);
  void setEffectInput(unsigned Index, Node* NewNode);
  void appendEffectInput(Node* NewNode);

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
  ValueT* find_value(Node* N) const {
    return Node2Value.count(N)?
            const_cast<ValueT*>(&Node2Value.at(N)) :
            nullptr;
  }

  Node* find_node(const ValueT& V) const {
    return Value2Node.count(V)?
            const_cast<Node*>(Value2Node.at(V)) :
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

/// A really lightweight representation of Node use edge
struct Use {
  enum Kind : uint8_t {
    K_VALUE = 0,
    K_CONTROL,
    K_EFFECT
  };

  Node* Source;
  Node* Dest;
  // Category of dependency
  Kind DepKind;

  Use() :
    Source(nullptr),
    Dest(nullptr),
    DepKind(K_VALUE) {}

  Use(Node* S, Node* D, Kind DK = K_VALUE) :
    Source(S),
    Dest(D),
    DepKind(DK) {}

  bool operator==(const Use& RHS) const {
    return Source == RHS.Source &&
           Dest == RHS.Dest &&
           DepKind == RHS.DepKind;
  }
  bool operator!=(const Use& RHS) const {
    return !(RHS == *this);
  }
};
} // end namespace gross

namespace std {
template<>
struct hash<gross::Use> {
  using argument_type = gross::Use;
  using result_type = size_t;
  result_type operator()(argument_type const& U) const noexcept {
    result_type seed = 0;
    boost::hash_combine(seed, U.Source);
    boost::hash_combine(seed, U.Dest);
    boost::hash_combine(seed, U.DepKind);
    return seed;
  }
};
} // end namespace std
#endif
