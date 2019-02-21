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
  template<class DerivedT>
  struct IdBase {
    bool operator==(const DerivedT& Other) const {
      return Other.IdVal == IdVal;
    }
    bool operator!=(const DerivedT& Other) const {
      return !(Other == *this);
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

  std::list<Node*> NodeSequence;
  std::unordered_map<Node*, SeqNodeId> NodeIds;

  std::vector<BasicBlock*> Predecessors;
  std::vector<BasicBlock*> Successors;

public:
  BasicBlock(const Id& BBId = Id::Create(0U))
    : BlockId(BBId) {}

  const Id& getId() const { return BlockId; }

  SeqNodeId* getNodeId(Node* N) const {
    if(NodeIds.count(N))
      return const_cast<SeqNodeId*>(&NodeIds.at(N));
    else
      return nullptr;
  }

  void AddNode(Node* N) {
    if(NodeSequence.empty()) {
      NodeIds.insert({N, SeqNodeId::Create(0U)});
    } else {
      auto* PrevNodeId = getNodeId(NodeSequence.back());
      assert(PrevNodeId);
      NodeIds.insert({N, SeqNodeId::AdvanceFrom(*PrevNodeId)});
    }
    NodeSequence.push_back(N);
  }

  // iterators
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

  void AddPredBlock(BasicBlock* BB) { Predecessors.push_back(BB); }
  void AddSuccBlock(BasicBlock* BB) { Successors.push_back(BB); }

  // build a std::pair<BasicBlock*,BasicBlock*> that represents
  // { source BB, dest BB }
  struct EdgeBuilder;

  using node_iterator = typename decltype(NodeSequence)::iterator;
  node_iterator node_begin() { return NodeSequence.begin(); }
  node_iterator node_end() { return NodeSequence.end(); }
  llvm::iterator_range<node_iterator> nodes() {
    return llvm::make_range(node_begin(), node_end());
  }
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
