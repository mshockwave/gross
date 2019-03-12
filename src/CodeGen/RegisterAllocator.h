#ifndef GROSS_CODEGEN_REGISTERALLOCATOR_H
#define GROSS_CODEGEN_REGISTERALLOCATOR_H
#include "gross/CodeGen/GraphScheduling.h"
#include <array>
#include <unordered_map>
#include <map>
#include <vector>

namespace gross {
class LinearScanRegisterAllocator {
  static constexpr size_t NumRegister = 32;
  GraphSchedule& Schedule;
  Graph& G;

  // RPO ordered users
  std::unordered_map<Node*, std::vector<Node*>> OrderedUsers;
  // lazily compute
  std::vector<Node*>& getOrderedUsers(Node* N);
  Node* LiveRangeEnd(Node* N) {
    auto& Usrs = getOrderedUsers(N);
    if(Usrs.empty()) return nullptr;
    else return Usrs.back();
  }

  // (both RegUsages/SpillSlots)
  // nullptr if available, currently used Node
  // otherwise
  std::array<Node*, NumRegister> RegUsages;
  std::vector<Node*> SpillSlots;

  size_t NumAvailableRegs() const {
    size_t Sum = 0U;
    for(auto* N : RegUsages) {
      if(!N) Sum++;
    }
    return Sum;
  }

  struct Location {
    bool IsRegister;
    // register number if IsRegister is true
    // stack slot number otherwise
    size_t Index;

    static Location Register(size_t Idx) {
      return Location{true, Idx};
    }
    static Location StackSlot(size_t Idx) {
      return Location{false, Idx};
    }
  };
  // value node -> register number or stack slot
  std::map<Node*, Location> Assignment;

  void AssignRegister(Node* N);
  void Spill(Node* N);
  void Recycle(Node* N);

  void InsertSpillCodes();
  // apply register nodes to Graph
  void CommitRegisterNodes();

public:
  LinearScanRegisterAllocator(GraphSchedule&);

  void Allocate();
};
} // end namespace gross
#endif
