#ifndef GROSS_CODEGEN_REGISTERALLOCATOR_H
#define GROSS_CODEGEN_REGISTERALLOCATOR_H
#include "gross/CodeGen/GraphScheduling.h"
#include <array>
#include <unordered_map>
#include <map>
#include <vector>

namespace gross {
class LinearScanRegisterAllocator {
  // register profiles
  static constexpr size_t NumRegister = 32;
  static constexpr size_t FirstCallerSaved = 10;
  static constexpr size_t LastCallerSaved = 25;
  static constexpr size_t FirstCalleeSaved = 6;
  static constexpr size_t LastCalleeSaved = 9;
  static constexpr size_t FirstParameter = 2;
  static constexpr size_t LastParameter = 5;

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
  // otherwise.
  // If the register user is IrOpcode::Constant,
  // then it's reserved register.
  std::array<Node*, NumRegister> RegUsages;
  std::vector<Node*> SpillSlots;

  size_t NumAvailableRegs() const {
    size_t Sum = 0U;
    for(auto* N : RegUsages) {
      if(!N) Sum++;
    }
    return Sum;
  }
  bool IsReserved(Node* N) const {
    return N && N->getOp() == IrOpcode::ConstantInt;
  }
  bool IsReserved(size_t RegNum) const {
    assert(RegNum < NumRegister);
    return IsReserved(RegUsages[RegNum]);
  }

  struct RegCopy {
    size_t From;
    bool IsFromReg;

    size_t ToReg;
    Node* InsertPos;
  };
  std::vector<RegCopy> RegisterCopies;
  void CreateRegisterCopy(Node* Before, size_t From, size_t To) {
    RegisterCopies.push_back({From, true, To, Before});
  }
  void CreateSpill2Register(Node* Before, size_t Slot, size_t Reg) {
    RegisterCopies.push_back({Slot, false, Reg, Before});
  }

  Node* CreateSpillSlotRead(size_t SlotNum);
  Node* CreateSpillSlotWrite(size_t SlotNum);

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

  size_t FindGeneralRegister() {
    // pick from caller-saved registers first
    auto check = [this](size_t Idx) -> bool {
      auto* RegSlot = RegUsages[Idx];
      if(!RegSlot) {
        return true;
      }
      return false;
    };
    for(auto I = FirstCallerSaved;
        I <= LastCallerSaved; ++I) {
      if(check(I)) return I;
    }
    // callee-saved
    for(auto I = FirstCalleeSaved;
        I <= LastCalleeSaved; ++I) {
      if(check(I)) return I;
    }
    // parameter
    for(auto I = FirstParameter;
        I <= LastParameter; ++I) {
      if(check(I)) return I;
    }
    return 0;
  }

  bool AssignRegister(Node* N);
  bool AssignPHIRegister(Node* N);
  void Spill(Node* N);
  void SpillPHI(Node* N);
  void Recycle(Node* N);

  void InsertSpillCodes();
  void InsertRegisterCopies();
  // apply register nodes to Graph
  void CommitRegisterNodes();

public:
  LinearScanRegisterAllocator(GraphSchedule&);

  void Allocate();
};
} // end namespace gross
#endif
