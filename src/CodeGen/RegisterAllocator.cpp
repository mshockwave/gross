#include "RegisterAllocator.h"
#include "DLXNodeUtils.h"
#include "gross/Graph/NodeUtils.h"
#include <algorithm>

using namespace gross;

LinearScanRegisterAllocator::
LinearScanRegisterAllocator(GraphSchedule& schedule)
  : Schedule(schedule),
    G(Schedule.getGraph()) {
  RegUsages.fill(nullptr);

  // Reserved registers
  // placeholder
  auto* PH = NodeBuilder<IrOpcode::ConstantInt>(&G, 0)
             .Build();
  const std::array<size_t, 12> ReservedRegs = {
    0, // constant zero
    1, // return value
    26, 27, // scratch registers
    28, // frame pointer
    29, // stack pointer
    30, // global vars pointer
    31, // link register
  };
  for(size_t R : ReservedRegs)
    RegUsages[R] = PH;
  // TODO: function parameters
}

std::vector<Node*>& LinearScanRegisterAllocator::getOrderedUsers(Node* N) {
  struct InstrOrderFunctor {
    explicit InstrOrderFunctor(GraphSchedule& schedule)
      : Schedule(schedule) {}

    bool operator()(const Node* Val1, const Node* Val2) const noexcept {
      auto* N1 = const_cast<Node*>(Val1);
      auto* N2 = const_cast<Node*>(Val2);
      auto* BB1 = Schedule.MapBlock(N1);
      auto* BB2 = Schedule.MapBlock(N2);
      assert(BB1 && BB2 && "Node not in any block?");

      auto BBId1 = BB1->getId().get<uint32_t>(),
           BBId2 = BB2->getId().get<uint32_t>();
      if(BBId1 != BBId2) {
        return BBId1 < BBId2;
      } else {
        // in the same BB
        auto NodeIdx1 = BB1->getNodeIndex(N1),
             NodeIdx2 = BB1->getNodeIndex(N2);
        assert(NodeIdx1 != NodeIdx2 && "has the same node index?");
        return NodeIdx1 < NodeIdx2;
      }
    }

private:
    GraphSchedule& Schedule;
  };

  if(!OrderedUsers.count(N)){
    // cache miss
    std::vector<Node*> ValUsrs(N->value_users().begin(),
                               N->value_users().end());
    std::sort(ValUsrs.begin(), ValUsrs.end(),
              InstrOrderFunctor(Schedule));
    OrderedUsers.insert({N, std::move(ValUsrs)});
  }
  return OrderedUsers.at(N);
}

Node* LinearScanRegisterAllocator::CreateSpillSlotRead(size_t SlotNum) {
  return nullptr;
}

Node* LinearScanRegisterAllocator::CreateSpillSlotWrite(size_t SlotNum) {
  return nullptr;
}

bool LinearScanRegisterAllocator::AssignPHIRegister(Node* N) {
  assert(N->getNumValueInput() == 2);
  const std::array<Node*, 2> ValInputs
    = { N->getValueInput(0), N->getValueInput(1) };
  assert(Assignment.count(ValInputs[0]) || Assignment.count(ValInputs[1]) &&
         "none of the input has assigned?");

  size_t TargetReg = 0U;
  uint8_t ReuseFrom = 0;
  if(LiveRangeEnd(ValInputs[0]) == N ||
     LiveRangeEnd(ValInputs[1]) == N) {
    // if PHI is the last user for either of the input value,
    // assign to that register
    if(LiveRangeEnd(ValInputs[0]) == N &&
       Assignment.count(ValInputs[0]) &&
       Assignment.at(ValInputs[0]).IsRegister) {
      TargetReg = Assignment.at(ValInputs[0]).Index;
      ReuseFrom = 1;
    } else if(LiveRangeEnd(ValInputs[1]) == N &&
              Assignment.count(ValInputs[1]) &&
              Assignment.at(ValInputs[1]).IsRegister) {
      TargetReg = Assignment.at(ValInputs[0]).Index;
      ReuseFrom = 2;
    }
  }
  if(!TargetReg) {
    // we really need a register
    TargetReg = FindGeneralRegister();
  }
  if(!TargetReg) return false;

  auto* CurBB = Schedule.MapBlock(N);
  assert(CurBB);
  auto FindLastInsertPos = [&](BasicBlock* BB) -> Node* {
    for(auto NI = BB->node_rbegin(), NE = BB->node_rend();
        NI != NE; ++NI) {
      if(NodeProperties<IrOpcode::VirtDLXTerminate>(*NI))
        continue;
      return *(--NI);
    }
    gross_unreachable("can't find insert position?");
    return nullptr;
  };
  for(auto i = 0; i < 2; ++i) {
    if(Assignment.count(ValInputs[i])) {
      auto& Loc = Assignment.at(ValInputs[i]);
      auto* BB = *CurBB->pred_begin();
      if(i) {
        auto BI = CurBB->pred_begin();
        BB = *(++BI);
      }
      if(Loc.IsRegister && Loc.Index != TargetReg) {
        // insert register copy
        // find the last insert position in that block
        CreateRegisterCopy(FindLastInsertPos(BB),
                           Loc.Index, TargetReg);
      } else if(!Loc.IsRegister) {
        CreateSpill2Register(FindLastInsertPos(BB),
                             Loc.Index, TargetReg);
      }
    }
  }

  RegUsages[TargetReg] = N;
  assert(!Assignment.count(N));
  Assignment[N] = Location::Register(TargetReg);
  return true;
}

bool LinearScanRegisterAllocator::AssignRegister(Node* N) {
  if(N->getOp() == IrOpcode::Phi)
    return AssignPHIRegister(N);

  if(auto Reg = FindGeneralRegister()) {
    RegUsages[Reg] = N;
    assert(!Assignment.count(N));
    Assignment[N] = Location::Register(Reg);
    return true;
  }
  return false;
}

void LinearScanRegisterAllocator::SpillPHI(Node* N) {
}

void LinearScanRegisterAllocator::Spill(Node* N) {
  if(N->getOp() == IrOpcode::Phi)
    return SpillPHI(N);

  bool Found = false;
  size_t Idx = 0U;
  for(size_t Size = SpillSlots.size(); Idx < Size; ++Idx) {
    if(!SpillSlots[Idx]) {
      Found = true;
      break;
    }
  }
  if(!Found) {
    // create new spill stack slot
    SpillSlots.push_back(N);
    Idx = SpillSlots.size() - 1;
  }
  assert(!Assignment.count(N));
  Assignment[N] = Location::StackSlot(Idx);
}

void LinearScanRegisterAllocator::Recycle(Node* N) {
  // recycle register
  for(size_t I = 0U, Size = RegUsages.size(); I < Size; ++I) {
    auto* RegUsr = RegUsages[I];
    if(!RegUsr) continue;
    if(IsReserved(RegUsr)) continue;

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
  // TODO
}

void LinearScanRegisterAllocator::InsertRegisterCopies() {
  // TODO
}

void LinearScanRegisterAllocator::CommitRegisterNodes() {
  // TODO
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
    // TODO: handle callsite

    if(hasValueUsers(CurNode)) {
      // need a register to store value
      if(!Assignment.count(CurNode)) {
        if(!AssignRegister(CurNode)) {
          // no register, spill
          Spill(CurNode);
        }
      }
    }

    // recycle expired register and/or spill slot
    Recycle(CurNode);
  }

  InsertRegisterCopies();

  InsertSpillCodes();

  CommitRegisterNodes();
}
