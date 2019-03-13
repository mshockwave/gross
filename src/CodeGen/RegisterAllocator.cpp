#include "RegisterAllocator.h"
#include "gross/Graph/NodeUtils.h"
#include <algorithm>

using namespace gross;

LinearScanRegisterAllocator::
LinearScanRegisterAllocator(GraphSchedule& schedule)
  : Schedule(schedule),
    G(Schedule.getGraph()),
    SUtils(Schedule) {
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

// 'move' every input values to a new value
void LinearScanRegisterAllocator::LegalizePhiInputs(Node* PN) {
  assert(PN->getNumValueInput() == 2);
  const std::array<Node*, 2> Inputs
    = { PN->getValueInput(0), PN->getValueInput(1) };
  auto* PNBB = Schedule.MapBlock(PN);
  assert(PNBB);
  auto PredBI = PNBB->pred_begin();
  const std::array<BasicBlock*, 2> InputBBs
    = { *PredBI, *(++PredBI) };
  auto* Zero = NodeBuilder<IrOpcode::ConstantInt>(&G, 0).Build();

  for(auto i = 0; i < 2; ++i) {
    auto* BB = InputBBs[i];
    auto* Val = Inputs[i];
    auto* Move
      = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI, true)
        .LHS(Val).RHS(Zero).Build();
    PN->ReplaceUseOfWith(Val, Move, Use::K_VALUE);
    // insert at the end of BB
    for(auto NI = BB->node_rbegin(), NE = BB->node_rend();
        NI != NE; ++NI) {
      if(NodeProperties<IrOpcode::VirtDLXTerminate>(*NI))
        continue;
      if(NI == BB->node_rbegin())
        Schedule.AddNode(BB, Move);
      else
        Schedule.AddNodeBefore(BB, *(--NI), Move);
    }
  }
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
    // extend the live range with PHI!
    std::vector<Node*> ValQueue(N->value_users().begin(),
                                N->value_users().end());
    std::vector<Node*> ValUsrs;
    while(!ValQueue.empty()) {
      auto* Top = ValQueue.front();
      ValQueue.erase(ValQueue.cbegin());
      ValUsrs.push_back(Top);
      if(Top->getOp() == IrOpcode::Phi) {
        for(auto* PU : Top->value_users())
          ValQueue.push_back(PU);
      }
    }

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

bool LinearScanRegisterAllocator::AssignRegister(Node* N) {
  Node* PHIUsr = nullptr;
  for(auto* VU : N->value_users()) {
    if(VU->getOp() == IrOpcode::Phi) {
      PHIUsr = VU;
      break;
    }
  }
  if(PHIUsr && Assignment.count(PHIUsr)) {
    auto& Loc = Assignment[PHIUsr];
    if(!Loc.IsRegister) return false;
    RegUsages[Loc.Index] = N;
    assert(!Assignment.count(N));
    Assignment[N] = Loc;
    return true;
  }

  if(auto Reg = FindGeneralRegister()) {
    RegUsages[Reg] = N;
    assert(!Assignment.count(N));
    Assignment[N] = Location::Register(Reg);

    if(PHIUsr) {
      assert(!Assignment.count(PHIUsr));
      Assignment[PHIUsr] = Location::Register(Reg);
    }
    return true;
  }
  return false;
}

void LinearScanRegisterAllocator::Spill(Node* N) {
  Node* PHIUsr = nullptr;
  for(auto* VU : N->value_users()) {
    if(VU->getOp() == IrOpcode::Phi) {
      PHIUsr = VU;
      break;
    }
  }
  if(PHIUsr && Assignment.count(PHIUsr)) {
    auto& Loc = Assignment[PHIUsr];
    assert(!Loc.IsRegister);
    SpillSlots[Loc.Index] = N;
    assert(!Assignment.count(N));
    Assignment[N] = Loc;
    return;
  }

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

  if(PHIUsr) {
    assert(!Assignment.count(PHIUsr));
    Assignment[PHIUsr] = Location::StackSlot(Idx);
  }
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
  if(SpillSlots.empty()) return;

  // reserve spill slots
  auto* Reservation = SUtils.ReserveSlots(SpillSlots.size());
  // TODO: insert it
  (void) Reservation;

  auto* Fp = SUtils.FramePointer();
  // nodes that will assigned to R27
  std::vector<Node*> ScratchCandidates;
  std::vector<Node*> ValUsrs;
  for(auto& AS : Assignment) {
    auto& Loc = AS.second;
    if(Loc.IsRegister) continue;

    auto* DefNode = AS.first;
    auto* DefBB = Schedule.MapBlock(DefNode);
    assert(DefBB);
    // collect before used by Store!
    ValUsrs.assign(DefNode->value_users().begin(),
                   DefNode->value_users().end());

    auto* SlotOffset = SUtils.NonLocalSlotOffset(Loc.Index);
    // no need to generate store for PHI
    if(DefNode->getOp() != IrOpcode::Phi) {
      auto* Store = NodeBuilder<IrOpcode::DLXStW>(&G)
                    .BaseAddr(Fp).Offset(SlotOffset)
                    .Src(DefNode).Build();
      Schedule.AddNodeAfter(DefBB, DefNode, Store);
    }

    for(auto* VU : ValUsrs) {
      // no need to re-load on PHI nodes
      if(VU->getOp() == IrOpcode::Phi) continue;
      auto* BB = Schedule.MapBlock(VU);
      assert(BB);
      auto* Load = NodeBuilder<IrOpcode::DLXLdW>(&G)
                   .BaseAddr(Fp).Offset(SlotOffset)
                   .Build();
      Schedule.AddNodeBefore(BB, VU, Load);
      VU->ReplaceUseOfWith(DefNode, Load, Use::K_VALUE);
      ScratchCandidates.push_back(Load);
    }
  }

  for(auto* SC : ScratchCandidates) {
    Assignment[SC] = Location::Register(27);
  }
}

void LinearScanRegisterAllocator::PostRALowering() {
  // TODO
  // 1. Remove PHI nodes
  // 2. Lowering function call virtual nodes
}

void LinearScanRegisterAllocator::CommitRegisterNodes() {
  // TODO
  // Transform to three-address instructions and replace
  // inputs with assigned registers
}

inline bool hasValueUsers(Node* N) {
  auto ValUsrs = N->value_users();
  return ValUsrs.begin() != ValUsrs.end();
}

void LinearScanRegisterAllocator::Allocate() {
  // 1. insert 'move' for every PHI input values
  // 2. if no register, spill
  // 3. assign register otherwise
  // 4. recycle any expired register
  // TODO: handle callsite

  std::vector<Node*> PHINodes;
  for(auto* N : Schedule.rpo_nodes()) {
    if(N->getOp() == IrOpcode::Phi &&
       N->getNumValueInput() && !N->getNumEffectInput())
      PHINodes.push_back(N);
  }
  for(auto* PN : PHINodes)
    LegalizePhiInputs(PN);

  for(auto* CurNode : Schedule.rpo_nodes()) {
    if(hasValueUsers(CurNode)) {
      // PHI should be handled after either of its
      // inputs values
      if(CurNode->getOp() == IrOpcode::Phi) {
        assert(Assignment.count(CurNode));
        auto& Loc = Assignment.at(CurNode);
        if(Loc.IsRegister) {
          RegUsages[Loc.Index] = CurNode;
        } else {
          SpillSlots[Loc.Index] = CurNode;
        }
      }
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

  InsertSpillCodes();

  CommitRegisterNodes();

  PostRALowering();
}
