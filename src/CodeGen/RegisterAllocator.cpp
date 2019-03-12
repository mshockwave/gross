#include "RegisterAllocator.h"

using namespace gross;

LinearScanRegisterAllocator::
LinearScanRegisterAllocator(GraphSchedule& schedule)
  : Schedule(schedule),
    G(Schedule.getGraph()) {
  RegUsages.fill(nullptr);
  // TODO: reserve special registers here
}

std::vector<Node*>& LinearScanRegisterAllocator::getOrderedUsers(Node* N) {
  // TODO
  return OrderedUsers.at(N);
}

void LinearScanRegisterAllocator::AssignRegister(Node* N) {
}

void LinearScanRegisterAllocator::Spill(Node* N) {
}

void LinearScanRegisterAllocator::Recycle(Node* N) {
  // recycle register
  for(size_t I = 0U, Size = RegUsages.size(); I < Size; ++I) {
    auto* RegUsr = RegUsages[I];
    if(!RegUsr) continue;
    // TODO: exclude special register
    if(LiveRangeEnd(RegUsr) == N) {
      RegUsages[I] = nullptr;
    }
  }

  // recycle spill slots
  for(auto I = SpillSlots.begin(), E = SpillSlots.end();
      I != E; ++I) {
    auto* SlotUsr = *I;
    if(!SlotUsr) continue;
    if(LiveRangeEnd(SlotUsr) == N) {
      *I = nullptr;
    }
  }
}

void LinearScanRegisterAllocator::InsertSpillCodes() {
}

void LinearScanRegisterAllocator::CommitRegisterNodes() {
}

inline bool hasValueUsers(Node* N) {
  auto ValUsrs = N->value_users();
  return ValUsrs.begin() != ValUsrs.end();
}

void LinearScanRegisterAllocator::Allocate() {
  for(auto* CurNode : Schedule.rpo_nodes()) {
    // 1. if no register, spill
    // 2. assign register otherwise
    // 3. recycle any expired register

    if(hasValueUsers(CurNode)) {
      // need a register to store value
      if(!NumAvailableRegs()) {
        // no register, spill
        Spill(CurNode);
      } else {
        // TODO: handle PHI node
        AssignRegister(CurNode);
      }
    }

    // recycle expired register and/or spill slot
    Recycle(CurNode);
  }

  InsertSpillCodes();
  CommitRegisterNodes();
}
