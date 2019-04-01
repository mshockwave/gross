#ifndef GROSS_GRAPH_NODE_H
#define GROSS_GRAPH_NODE_H
#include "gross/Graph/Opcodes.h"
#include "gross/Support/Log.h"
#include "gross/Support/iterator_range.h"
#include "boost/container_hash/hash.hpp"
#include "boost/iterator/filter_iterator.hpp"
#include <functional>
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

  struct BuilderFunctor;
};

struct Use::BuilderFunctor {
  Node* From;
  Use::Kind DepKind;

  using PatcherTy = std::function<Use(const Use&)>;
  PatcherTy PatchFunctor;

  // will have problem if one just
  // delcare edge iterator without initialize
  // BuilderFunctor() = delete;
  BuilderFunctor() = default;

  explicit
  BuilderFunctor(Node* F, Use::Kind K = K_NONE,
                 PatcherTy Patcher = nullptr)
    : From(F), DepKind(K),
      PatchFunctor(Patcher) {}

  Use operator()(Node* To) const {
    Use E(From, To, DepKind);
    if(PatchFunctor)
      return PatchFunctor(E);
    else
      return std::move(E);
  }
};

class Node {
  friend class Graph;
  template<class T>
  friend class lazy_edge_iterator;
  template<IrOpcode::ID>
  friend class NodePropertiesBase;
  template<IrOpcode::ID>
  friend class NodeProperties;
  friend class NodeMarkerBase;
  template<IrOpcode::ID>
  friend class NodeBuilder;
  template<IrOpcode::ID OC,class SubT>
  friend struct _details::BinOpNodeBuilder;

  IrOpcode::ID Op;

  // used by NodeMarker
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
  void removeNodeInput(unsigned Index, unsigned& Size, unsigned Offset);
  void removeNodeInputAll(Node* N, unsigned& Size, unsigned Offset);
  void cleanupRemoveNodeInput(Node* OldInput);

  bool IsKilled;

public:
  using MarkerTy = uint32_t;

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
  void removeValueInput(unsigned Index);
  void removeValueInputAll(Node* N);

  void setControlInput(unsigned Index, Node* NewNode);
  void appendControlInput(Node* NewNode);
  void removeControlInput(unsigned Index);
  void removeControlInputAll(Node* N);

  void setEffectInput(unsigned Index, Node* NewNode);
  void appendEffectInput(Node* NewNode);
  void removeEffectInput(unsigned Index);
  void removeEffectInputAll(Node* N);

  // remove all the inputs, and replace all
  // (remaining) users with Dead Node
  void Kill(Node* DeadNode);
  bool IsDead() const { return IsKilled; }

  using input_iterator = typename decltype(Inputs)::iterator;
  using const_input_iterator = typename decltype(Inputs)::const_iterator;
  llvm::iterator_range<input_iterator> inputs() {
    return llvm::make_range(Inputs.begin(), Inputs.end());
  }
  llvm::iterator_range<const_input_iterator> inputs() const {
    return llvm::make_range(Inputs.cbegin(), Inputs.cend());
  }
  input_iterator input_begin() { return Inputs.begin(); }
  input_iterator input_end() { return Inputs.end(); }
  size_t input_size() const { return Inputs.size(); }

  // iterators
  llvm::iterator_range<input_iterator>
  value_inputs() {
    return llvm::make_range(Inputs.begin(),
                            Inputs.begin() + NumValueInput);
  }
  input_iterator value_input_begin() { return value_inputs().begin(); }
  input_iterator value_input_end() { return value_inputs().end(); }

  llvm::iterator_range<input_iterator>
  control_inputs() {
    return llvm::make_range(Inputs.begin() + NumValueInput,
                            Inputs.begin() + NumValueInput + NumControlInput);
  }
  input_iterator control_input_begin() { return control_inputs().begin(); }
  input_iterator control_input_end() { return control_inputs().end(); }

  llvm::iterator_range<input_iterator>
  effect_inputs() {
    auto IB = Inputs.begin();
    return llvm::make_range(IB + NumValueInput + NumControlInput,
                            IB + NumValueInput + NumControlInput
                            + NumEffectInput);
  }
  input_iterator effect_input_begin() { return effect_inputs().begin(); }
  input_iterator effect_input_end() { return effect_inputs().end(); }

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

  llvm::iterator_range<user_iterator>
  users() {
    return llvm::make_range(Users.begin(), Users.end());
  }
  llvm::iterator_range<value_user_iterator>
  value_users();
  llvm::iterator_range<control_user_iterator>
  control_users();
  llvm::iterator_range<effect_user_iterator>
  effect_users();

  size_t user_size() const { return Users.size(); }

  Node()
    : Op(IrOpcode::None),
      MarkerData(0U),
      NumValueInput(0),
      NumControlInput(0),
      NumEffectInput(0),
      IsKilled(false) {}

  Node(IrOpcode::ID OC)
    : Op(OC),
      MarkerData(0U),
      NumValueInput(0),
      NumControlInput(0),
      NumEffectInput(0),
      IsKilled(false) {}

  Node(IrOpcode::ID OC,
       const std::vector<Node*>& Values,
       const std::vector<Node*>& Controls = {},
       const std::vector<Node*>& Effects = {});

  bool ReplaceUseOfWith(Node* From, Node* To, Use::Kind UseKind);
  // replace this node with Replacement in all its users
  void ReplaceWith(Node* Replacement, Use::Kind UseKind = Use::K_NONE);
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

  bool insert(const N2VPairTy& Pair, bool overwrite = false) {
    if(overwrite) {
      Node2Value[Pair.first] = Pair.second;
      Value2Node[Pair.second] = Pair.first;
    } else {
      if(Node2Value.count(Pair.first) ||
         Value2Node.count(Pair.second)) {
        return false;
      }
      Node2Value.insert(Pair);
      Value2Node.insert(std::make_pair(Pair.second, Pair.first));
    }
    return true;
  }

  void erase(Node* N) {
    if(Node2Value.count(N)) {
      auto Val = Node2Value.at(N);
      Node2Value.erase(N);
      Value2Node.erase(Val);
    }
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
