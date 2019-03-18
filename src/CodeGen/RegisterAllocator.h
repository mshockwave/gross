#ifndef GROSS_CODEGEN_REGISTERALLOCATOR_H
#define GROSS_CODEGEN_REGISTERALLOCATOR_H
#include "DLXNodeUtils.h"
#include "gross/CodeGen/GraphScheduling.h"
#include <array>
#include <bitset>
#include <unordered_map>
#include <map>
#include <vector>

namespace gross {
// Forward declarations
struct StackUtils;

struct RegisterAllocator {
  struct Location {
    enum Kind {
      K_REG,
      K_SPILL_VAL,
      K_SPILL_PARAM
    };
    Kind LocKind;

    // K_REG: register number
    // K_SPILL_VAL: spilled stack slot
    // K_SPILL_PARAM: spilled function parameter slot
    size_t Index;

    bool IsRegister() const { return LocKind == K_REG; };
    bool IsSpilledVal() const {
      return LocKind == K_SPILL_VAL;
    };
    bool IsSpilledParam() const {
      return LocKind == K_SPILL_PARAM;
    };

    static Location Register(size_t Idx) {
      return Location{K_REG, Idx};
    }
    static Location SpilledVal(size_t Idx) {
      return Location{K_SPILL_VAL, Idx};
    }
    static Location SpilledParam(size_t Idx) {
      return Location{K_SPILL_PARAM, Idx};
    }
  };
};

template<class Target>
class LinearScanRegisterAllocator : public RegisterAllocator {
  static constexpr size_t NumRegister
    = Target::RegisterFile::size();
  static constexpr size_t FirstCallerSaved
    = Target::RegisterFile::FirstCallerSaved;
  static constexpr size_t LastCallerSaved
    = Target::RegisterFile::LastCallerSaved;
  static constexpr size_t FirstCalleeSaved
    = Target::RegisterFile::FirstCalleeSaved;
  static constexpr size_t LastCalleeSaved
    = Target::RegisterFile::LastCalleeSaved;
  static constexpr size_t FirstParameter
    = Target::RegisterFile::FirstParameter;
  static constexpr size_t LastParameter
    = Target::RegisterFile::LastParameter;

  GraphSchedule& Schedule;
  Graph& G;
  StackUtils SUtils;

  // one more entry for the (stupid) trailing nullptr
  const std::array<Node*, NumRegister + 1> RegNodes;

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
  std::vector<Node*> SpillParams;

  // VirtDLXCallsiteBegin node -> active registers at this moment
  std::unordered_map<Node*, std::bitset<NumRegister>> CallerSaved;
  // Callee-saved registers that have ever clobbered in this function
  std::bitset<NumRegister> CalleeSaved;

  bool IsReserved(Node* N) const {
    return N && N->getOp() == IrOpcode::ConstantInt;
  }
  bool IsReserved(size_t RegNum) const {
    assert(RegNum < NumRegister);
    return IsReserved(RegUsages[RegNum]);
  }

  // value node -> register number or stack slot
  std::map<Node*, Location> Assignment;

  Node* CreateMove(Node* From);

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

  void LegalizePhiInputs(Node* N);
  void ParametersLowering();

  bool AssignRegister(Node* N);
  void Spill(Node* N);
  void Recycle(Node* N);

  // do before inserting epilogue!
  void FunctionReturnLowering();

  // lower parameter passing
  void CallsiteLowering();

  // return the instruction right after spill slots
  Node* InsertSpillCodes();

  void InsertCalleeSavedCodes(Node* PosBefore);
  void InsertCallerSavedCodes();

  void MemAllocationLowering();

  void CommitRegisterNodes();

public:
  LinearScanRegisterAllocator(GraphSchedule&);

  void Allocate();

  const Location& GetAllocation(Node* N) const {
    assert(Assignment.count(N));
    return Assignment.at(N);
  }
};

// template specialize stub
void __SupportedLinearScanRATargets(GraphSchedule&);
} // end namespace gross
#endif
