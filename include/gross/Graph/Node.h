#ifndef GROSS_GRAPH_NODE_H
#define GROSS_GRAPH_NODE_H
#include "gross/Graph/Opcodes.h"
#include "gross/Support/Log.h"
#include "gross/Support/iterator_range.h"
#include "boost/container_hash/hash.hpp"
#include "boost/iterator/filter_iterator.hpp"
#include <unordered_map>
#include <vector>
#include <utility>

namespace gross {
// Forward declarations
class Node;
namespace _details {
template<IrOpcode::ID OC,class SubT>
struct BinOpNodeBuilder;
} // end namespace _details

/// A really lightweight representation of Node use edge
struct Use {
  enum Kind : uint8_t {
    K_NONE = 0,
    K_VALUE,
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
    DepKind(K_NONE) {}

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

  struct BuilderFunctor {
    Node* From;
    Use::Kind DepKind;

    // will have problem if one just
    // delcare edge iterator without initialize
    // BuilderFunctor() = delete;
    BuilderFunctor() = default;

    explicit
    BuilderFunctor(Node* F, Use::Kind K = K_NONE)
      : From(F), DepKind(K) {}

    Use operator()(Node* To) const {
      return Use(From, To, DepKind);
    }
  };
};

class Node {
  friend class Graph;
  template<class T>
  friend class lazy_edge_iterator;
  template<IrOpcode::ID>
  friend class NodePropertiesBase;
  template<IrOpcode::ID>
  friend class NodeProperties;
  template<class T>
  friend class NodeMarker;
  template<IrOpcode::ID>
  friend class NodeBuilder;
  template<IrOpcode::ID OC,class SubT>
  friend struct _details::BinOpNodeBuilder;

  IrOpcode::ID Op;

  // used by NodeMarker
  using MarkerTy = uint32_t;
  uint32_t MarkerData;

  unsigned NumValueInput;
  unsigned NumControlInput;
  unsigned NumEffectInput;

  std::vector<Node*> Inputs;
  inline Use::Kind inputUseKind(unsigned rawInputIdx) {
    assert(rawInputIdx < Inputs.size());
    if(rawInputIdx < NumValueInput) return Use::K_VALUE;
    else if(rawInputIdx < NumValueInput + NumControlInput)
      return Use::K_CONTROL;
    else if(rawInputIdx < NumValueInput + NumControlInput + NumEffectInput)
      return Use::K_EFFECT;
    return Use::K_NONE;
  }

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

  using input_iterator = typename decltype(Inputs)::iterator;
  llvm::iterator_range<input_iterator> inputs() {
    return llvm::make_range(Inputs.begin(), Inputs.end());
  }
  typename decltype(Inputs)::iterator input_begin() { return Inputs.begin(); }
  typename decltype(Inputs)::iterator input_end() { return Inputs.end(); }

  // iterators
  llvm::iterator_range<typename decltype(Inputs)::iterator>
  value_inputs() {
    return llvm::make_range(Inputs.begin(),
                            Inputs.begin() + NumValueInput);
  }
  llvm::iterator_range<typename decltype(Inputs)::iterator>
  control_inputs() {
    return llvm::make_range(Inputs.begin() + NumValueInput,
                            Inputs.begin() + NumValueInput + NumControlInput);
  }
  llvm::iterator_range<typename decltype(Inputs)::iterator>
  effect_inputs() {
    auto IB = Inputs.begin();
    return llvm::make_range(IB + NumValueInput + NumControlInput,
                            IB + NumValueInput + NumControlInput
                            + NumEffectInput);
  }

  struct is_value_use {
    Node* SrcNode;
    is_value_use(Node* Src) : SrcNode(Src) {}

    bool operator()(Node* Sink) {
      for(Node* Dep : Sink->value_inputs()) {
        if(Dep == SrcNode) return true;
      }
      return false;
    }
  };
  struct is_control_use {
    Node* SrcNode;
    is_control_use(Node* Src) : SrcNode(Src) {}

    bool operator()(Node* Sink) {
      for(Node* Dep : Sink->control_inputs()) {
        if(Dep == SrcNode) return true;
      }
      return false;
    }
  };
  struct is_effect_use {
    Node* SrcNode;
    is_effect_use(Node* Src) : SrcNode(Src) {}

    bool operator()(Node* Sink) {
      for(Node* Dep : Sink->effect_inputs()) {
        if(Dep == SrcNode) return true;
      }
      return false;
    }
  };
  using user_iterator = typename decltype(Users)::iterator;
  using value_user_iterator
    = boost::filter_iterator<is_value_use, user_iterator>;
  using control_user_iterator
    = boost::filter_iterator<is_control_use, user_iterator>;
  using effect_user_iterator
    = boost::filter_iterator<is_effect_use, user_iterator>;

  llvm::iterator_range<value_user_iterator>
  value_users();
  llvm::iterator_range<control_user_iterator>
  control_users();
  llvm::iterator_range<effect_user_iterator>
  effect_users();

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

  bool ReplaceUseOfWith(Node* From, Node* To, Use::Kind UseKind);
  // replace this node with Replacement in all its users
  void ReplaceWith(Node* Replacement, Use::Kind UseKind);
};

/// scratch data inside Node that is fast to access
/// note that there should be only one kind of NodeMarker
/// active at a given time
template<class T>
struct NodeMarker {
  static_assert(sizeof(T) <= sizeof(typename Node::MarkerTy),
                "can't fit into marker scratch");

  static T Get(Node* N) {
    return static_cast<T>(N->MarkerData);
  }
  static void Set(Node* N, T val) {
    N->MarkerData
      = static_cast<typename Node::MarkerTy>(val);
  }
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
