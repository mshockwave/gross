#ifndef GROSS_CODEGEN_BASICBLOCK_H
#define GROSS_CODEGEN_BASICBLOCK_H
#include "gross/Graph/Node.h"
#include "gross/Support/iterator_range.h"
#include <list>
#include <utility>
#include <unordered_map>
#include <iostream>

namespace gross {
struct BasicBlock {
  friend class GraphSchedule;

  template<class DerivedT>
  struct IdBase {
    bool operator==(const DerivedT& Other) const {
      return Other.IdVal == IdVal;
    }
    bool operator!=(const DerivedT& Other) const {
      return !(Other == *this);
    }

    template<class T = uint32_t>
    T get() const {
      return static_cast<T>(IdVal);
    }

    template<class IntegralT>
    static DerivedT Create(IntegralT V) {
      DerivedT DV;
      DV.IdVal = static_cast<uint32_t>(V);
      return std::move(DV);
    }

    static DerivedT AdvanceFrom(const DerivedT& Prev) {
      DerivedT DV;
      DV.IdVal = Prev.IdVal + 1;
      return std::move(DV);
    }

    struct hash {
      using argument_type = DerivedT;
      using result_type = size_t;

      result_type operator()(argument_type const& V) const noexcept {
        return std::hash<uint32_t>{}(V.IdVal);
      }
    };

  protected:
    uint32_t IdVal;
  };

  // BasicBlock Id
  struct Id : public IdBase<Id> {
    std::ostream& print(std::ostream& OS) const {
      return OS << "$" << IdVal;
    }
  };

  // Node Id used only in linearlized Nodes
  struct SeqNodeId : public IdBase<SeqNodeId> {
    std::ostream& print(std::ostream& OS) const {
      return OS << "(" << IdVal << ")";
    }
  };

private:
  Id BlockId;

  // first Node is always the control op of this block
  std::list<Node*> NodeSequence;
  std::unordered_map<Node*, SeqNodeId> NodeIds;
  SeqNodeId LastNodeId;

  std::vector<BasicBlock*> Predecessors;
  std::vector<BasicBlock*> Successors;

public:
  BasicBlock(const Id& BBId = Id::Create(0U))
    : BlockId(BBId),
      LastNodeId(SeqNodeId::Create(0U)) {}

  const Id& getId() const { return BlockId; }
  // Id is RPO order index
  size_t getRPOIndex() const { return getId().get<size_t>(); }

  Node* getCtrlNode() const { return NodeSequence.empty()?
                                     nullptr : NodeSequence.front(); }

  // block iterators
  using pred_iterator = typename decltype(Predecessors)::iterator;
  using succ_iterator = typename decltype(Successors)::iterator;
  pred_iterator pred_begin() { return Predecessors.begin(); }
  pred_iterator pred_end() { return Predecessors.end(); }
  llvm::iterator_range<pred_iterator> preds() {
    return llvm::make_range(pred_begin(), pred_end());
  }
  size_t pred_size() const { return Predecessors.size(); }
  succ_iterator succ_begin() { return Successors.begin(); }
  succ_iterator succ_end() { return Successors.end(); }
  llvm::iterator_range<succ_iterator> succs() {
    return llvm::make_range(succ_begin(), succ_end());
  }
  size_t succ_size() const { return Successors.size(); }

  bool HasPredBlock(BasicBlock* BB);
  void AddPredBlock(BasicBlock* BB) { Predecessors.push_back(BB); }
  bool RemovePredBlock(BasicBlock* BB);

  bool HasSuccBlock(BasicBlock* BB);
  void AddSuccBlock(BasicBlock* BB) { Successors.push_back(BB); }
  bool RemoveSuccBlock(BasicBlock* BB);

  // build a std::pair<BasicBlock*,BasicBlock*> that represents
  // { source BB, dest BB }
  struct EdgeBuilder;

  // Node iterators
  using node_iterator = typename decltype(NodeSequence)::iterator;
  using const_node_iterator = typename decltype(NodeSequence)::const_iterator;
  using reverse_node_iterator
    = typename decltype(NodeSequence)::reverse_iterator;
  node_iterator node_begin() { return NodeSequence.begin(); }
  node_iterator node_end() { return NodeSequence.end(); }
  const_node_iterator node_cbegin() { return NodeSequence.cbegin(); }
  const_node_iterator node_cend() { return NodeSequence.cend(); }
  reverse_node_iterator node_rbegin() { return NodeSequence.rbegin(); }
  reverse_node_iterator node_rend() { return NodeSequence.rend(); }
  llvm::iterator_range<node_iterator> nodes() {
    return llvm::make_range(node_begin(), node_end());
  }
  llvm::iterator_range<const_node_iterator> const_nodes() {
    return llvm::make_range(node_cbegin(), node_cend());
  }
  llvm::iterator_range<reverse_node_iterator> reverse_nodes() {
    return llvm::make_range(node_rbegin(), node_rend());
  }

  SeqNodeId* getNodeId(Node* N) const;

  size_t getNodeIndex(Node* N) const;

private:
  // Since GraphSchedule is the one managing other BB properties
  // (e.g. Node <-> BB mapping) we want GraphSchedule be the only
  // interface to manage nodes in BB. Thus putting the rest APIs
  // as private.
  void AddNode(node_iterator Pos, Node* N);
  bool AddNodeBefore(Node* Before, Node* N);
  bool AddNodeAfter(Node* After, Node* N);
  void AddNode(Node* N) {
    AddNode(node_end(), N);
  }

  std::pair<bool, node_iterator> RemoveNode(Node* N);

  void ReplaceNode(Node* Old, Node* New);
};

struct BasicBlock::EdgeBuilder {
  using value_type = std::pair<BasicBlock*,BasicBlock*>;
  BasicBlock* FromBB;

  EdgeBuilder() : FromBB(nullptr) {}
  explicit EdgeBuilder(BasicBlock* From) : FromBB(From) {}

  value_type operator()(BasicBlock* To) const {
    return std::make_pair(FromBB, To);
  }
};
} // end namespace gross
#endif
