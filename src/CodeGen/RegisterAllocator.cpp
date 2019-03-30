#include "RegisterAllocator.h"
#include "Targets.h"
#include "gross/Graph/NodeUtils.h"
#include <algorithm>

using namespace gross;


template<class T> LinearScanRegisterAllocator<T>::
LinearScanRegisterAllocator(GraphSchedule& schedule)
  : Schedule(schedule),
    G(Schedule.getGraph()),
    SUtils(Schedule),
    RegNodes({
#define DLX_REG(OC)  \
      NodeBuilder<IrOpcode::DLX##OC>(&G).Build(),
#include "gross/Graph/DLXOpcodes.def"
    nullptr}) {
  RegUsages.fill(nullptr);

  // Reserved registers
  // placeholder
  auto* PH = NodeBuilder<IrOpcode::ConstantInt>(&G, 0)
             .Build();
  const std::array<size_t, 12> ReservedRegs = {
    0, // constant zero
    T::ReturnStorage, // return value
    26, 27, // scratch registers
    28, // frame pointer
    29, // stack pointer
    30, // global vars pointer
    31, // link register
  };
  for(size_t R : ReservedRegs)
    RegUsages[R] = PH;
}

template<class T>
Node* LinearScanRegisterAllocator<T>::CreateMove(Node* From) {
  assert(From);
  auto* LHSVal = From;
  auto* RHSVal = NodeBuilder<IrOpcode::ConstantInt>(&G, 0).Build();
  if(LHSVal->getOp() == IrOpcode::ConstantInt) {
    RHSVal = LHSVal;
    LHSVal = RegNodes[0]; // zero register
  }
  auto* Move
    = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI, true)
      .LHS(LHSVal).RHS(RHSVal).Build();
  return Move;
}

// 'move' every input values to a new value
template<class T>
void LinearScanRegisterAllocator<T>::LegalizePhiInputs(Node* PN) {
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
    auto* Move = CreateMove(Val);
    PN->ReplaceUseOfWith(Val, Move, Use::K_VALUE);
    // insert at the end of BB
    Node* PosAfter = nullptr;
    for(auto* N : BB->reverse_nodes()) {
      if(NodeProperties<IrOpcode::VirtDLXTerminate>(N)) {
        continue;
      }
      PosAfter = N;
      break;
    }
    if(PosAfter)
      Schedule.AddNodeAfter(BB, PosAfter, Move);
    else
      Schedule.AddNode(BB, BB->node_begin(), Move);
  }
}

template<class T>
void LinearScanRegisterAllocator<T>::ParametersLowering() {
  auto* FuncStart = Schedule.getStartNode();
  assert(FuncStart);
  constexpr auto FirstParamReg = T::RegisterFile::FirstParameter;
  const auto RegParamQuota
    = (T::RegisterFile::LastParameter - FirstParamReg + 1U);
  for(auto ArgIdx = 0U, ArgSize = FuncStart->getNumEffectInput();
      ArgIdx < ArgSize; ++ArgIdx) {
    auto* ArgNode = FuncStart->getEffectInput(ArgIdx);
    if(ArgIdx < RegParamQuota) {
      // read from register
      Assignment[ArgNode] = Location::Register(FirstParamReg + ArgIdx);
      RegUsages[FirstParamReg + ArgIdx] = ArgNode;
    } else {
      // read from stack
      Assignment[ArgNode] = Location::SpilledParam(ArgIdx - RegParamQuota);
      SpillParams.push_back(ArgNode);
    }
  }
}

template<class T>
void LinearScanRegisterAllocator<T>::FunctionReturnLowering() {
  std::vector<Node*> Returns;
  for(auto* BB : Schedule.rpo_blocks()) {
    for(auto* N : BB->nodes()) {
      if(N->getOp() == IrOpcode::Return)
        Returns.push_back(N);
    }
  }

  if(Returns.empty()) {
    // insert RET <link register> at the end block
    auto* EndNode = Schedule.getEndNode();
    auto* EndBlock = Schedule.MapBlock(EndNode);
    assert(EndBlock);
    auto* NewRet
      = NodeBuilder<IrOpcode::DLXRet>(&G, RegNodes[T::LinkRegister])
        .Build();
    Schedule.AddNodeBefore(EndBlock, EndNode, NewRet);
  } else {
    for(auto* Return : Returns) {
      // replace with move to return storage register.
      // And RET <link register>
      auto* RetBB = Schedule.MapBlock(Return);
      assert(RetBB);
      auto* RetVal = NodeProperties<IrOpcode::Return>(Return).ReturnVal();
      auto* Move = CreateMove(RetVal);
      Assignment[Move] = Location::Register(T::ReturnStorage);
      Schedule.AddNodeBefore(RetBB, Return, Move);
      auto* NewRet
        = NodeBuilder<IrOpcode::DLXRet>(&G, RegNodes[T::LinkRegister])
          .Build();
      Schedule.ReplaceNode(RetBB, Return, NewRet);
      Return->ReplaceUseOfWith(RetVal,
                               NodeBuilder<IrOpcode::ConstantInt>(&G, 0)
                               .Build(), Use::K_VALUE);
    }
  }
}

template<class T>
void LinearScanRegisterAllocator<T>::CallsiteLowering() {
  // - replace VirtDLXPassParam into either register
  //   saving or pushing to stack
  // - insert stack recovering code before CallsiteEnd
  //   if we push any argument into stack

  std::vector<Node*> Callsites;
  for(auto* BB : Schedule.rpo_blocks())
    for(auto* N : BB->nodes()) {
      if(N->getOp() == IrOpcode::VirtDLXCallsiteBegin)
        Callsites.push_back(N);
    }

  std::list<Node*> CSParams;
  for(auto* CS : Callsites) {
    NodeProperties<IrOpcode::VirtDLXCallsiteBegin> CSNP(CS);
    auto* CSEnd = CSNP.getCallsiteEnd();
    auto* CSBB = Schedule.MapBlock(CS);
    assert(CSBB);
    CSParams.assign(CSNP.param_begin(), CSNP.param_end());
    for(auto Reg = T::RegisterFile::FirstParameter;
        Reg <= T::RegisterFile::LastParameter && !CSParams.empty();
        ++Reg) {
      auto* Param = CSParams.front();
      assert(Param->getNumValueInput() > 0);
      auto* ActualParam = Param->getValueInput(0);
      auto* Move = CreateMove(ActualParam);
      Assignment[Move] = Location::Register(Reg);
      Schedule.ReplaceNode(CSBB, Param, Move);
      CSParams.pop_front();
    }

    // stil have some parameters need to push to stack
    // (in reverse order)
    for(auto PI = CSParams.rbegin(), PE = CSParams.rend();
        PI != PE; ++PI) {
      auto* Param = *PI;
      assert(Param->getNumValueInput() > 0);
      auto* ActualParam = Param->getValueInput(0);
      auto* Push = SUtils.ReserveSlots(1, ActualParam);
      Schedule.AddNodeBefore(CSBB, CSParams.front(), Push);
    }
    // restore stack before CallsiteEnd if there are
    // stack parameter
    if(!CSParams.empty()) {
      auto ParamSize = CSParams.size();
      auto* OffsetNode
        = NodeBuilder<IrOpcode::ConstantInt>(&G, ParamSize * 4).Build();
      auto* Restore
        = NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI, true)
          .LHS(RegNodes[T::StackPointer])
          .RHS(OffsetNode)
          .Build();
      Assignment[Restore] = Location::Register(T::StackPointer);
      Schedule.AddNodeBefore(CSBB, CSEnd, Restore);
    }

    // remove virtual callsite markers
    for(auto* N : CSParams) {
      Schedule.RemoveNode(CSBB, N);
    }
    Schedule.RemoveNode(CSBB, CS);
    Schedule.RemoveNode(CSBB, CSEnd);
  }
}

template<class T>
std::vector<Node*>& LinearScanRegisterAllocator<T>::getOrderedUsers(Node* N) {
  struct InstrOrderFunctor {
    explicit InstrOrderFunctor(GraphSchedule& schedule)
      : Schedule(schedule) {}

    bool operator()(const Node* Val1, const Node* Val2) const noexcept {
      if(Val1 == Val2) return true;
      auto* N1 = const_cast<Node*>(Val1);
      auto* N2 = const_cast<Node*>(Val2);
      auto* BB1 = Schedule.MapBlock(N1);
      auto* BB2 = Schedule.MapBlock(N2);
      assert(BB1 && BB2 && "Node not in any block?");

      auto BBId1 = BB1->getId().template get<uint32_t>(),
           BBId2 = BB2->getId().template get<uint32_t>();
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
    // only those reside in Blocks
    for(auto NI = ValUsrs.begin(); NI != ValUsrs.end();) {
      if(!Schedule.MapBlock(*NI))
        NI = ValUsrs.erase(NI);
      else
        ++NI;
    }
    std::sort(ValUsrs.begin(), ValUsrs.end(),
              InstrOrderFunctor(Schedule));
    OrderedUsers.insert({N, std::move(ValUsrs)});
  }
  return OrderedUsers.at(N);
}

template<class T>
bool LinearScanRegisterAllocator<T>::AssignRegister(Node* N) {
  Node* PHIUsr = nullptr;
  for(auto* VU : N->value_users()) {
    if(VU->getOp() == IrOpcode::Phi) {
      PHIUsr = VU;
      break;
    }
  }
  if(PHIUsr && Assignment.count(PHIUsr)) {
    auto& Loc = Assignment[PHIUsr];
    if(!Loc.IsRegister()) return false;
    RegUsages[Loc.Index] = N;
    assert(!Assignment.count(N));
    Assignment[N] = Loc;
    return true;
  }

  if(auto Reg = FindGeneralRegister()) {
    RegUsages[Reg] = N;
    assert(!Assignment.count(N));
    Assignment[N] = Location::Register(Reg);
    if(Reg >= FirstCalleeSaved && Reg <= LastCalleeSaved)
      CalleeSaved[Reg] = true;

    if(PHIUsr) {
      assert(!Assignment.count(PHIUsr));
      Assignment[PHIUsr] = Location::Register(Reg);
    }
    return true;
  }
  return false;
}

template<class T>
void LinearScanRegisterAllocator<T>::Spill(Node* N) {
  Node* PHIUsr = nullptr;
  for(auto* VU : N->value_users()) {
    if(VU->getOp() == IrOpcode::Phi) {
      PHIUsr = VU;
      break;
    }
  }
  // also handle spilled stack parameter here!
  if(PHIUsr && Assignment.count(PHIUsr)) {
    auto& Loc = Assignment[PHIUsr];
    assert(!Loc.IsRegister());
    if(Loc.IsSpilledVal()) {
      SpillSlots[Loc.Index] = N;
    }
    assert(!Assignment.count(N));
    Assignment[N] = Loc;
    return;
  }

  if(Assignment.count(N) &&
     Assignment.at(N).IsSpilledParam()) {
    // propagate the spilled param location
    if(PHIUsr) {
      assert(!Assignment.count(PHIUsr));
      Assignment[PHIUsr] = Assignment.at(N);
    }
  } else {
    // spilled value
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
    Assignment[N] = Location::SpilledVal(Idx);
    SpillSlots[Idx] = N;

    if(PHIUsr) {
      assert(!Assignment.count(PHIUsr));
      Assignment[PHIUsr] = Location::SpilledVal(Idx);
    }
  }
}

template<class T>
void LinearScanRegisterAllocator<T>::Recycle(Node* N) {
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

template<class T>
Node* LinearScanRegisterAllocator<T>::InsertSpillCodes() {
  // find the place next to local var stack slots
  auto* EntryBlock = Schedule.getEntryBlock();
  assert(EntryBlock);
  Node* PosBefore = nullptr;
  for(auto* N : EntryBlock->nodes()) {
    if(N->getOp() == IrOpcode::Start ||
       N->getOp() == IrOpcode::Alloca) continue;
    PosBefore = N;
    break;
  }
  if(!PosBefore) {
    PosBefore = Schedule.getEndNode();
  }

  if(SpillSlots.empty() &&
     SpillParams.empty()) return PosBefore;

  if(!SpillSlots.empty()) {
    // reserve spill slots
    auto* Reservation = SUtils.ReserveSlots(SpillSlots.size());
    Schedule.AddNodeBefore(EntryBlock, PosBefore, Reservation);
  }

  auto* Fp = SUtils.FramePointer();
  // nodes that will assigned to R27
  std::vector<Node*> ScratchCandidates;
  std::vector<Node*> ValUsrs;
  for(auto& AS : Assignment) {
    auto& Loc = AS.second;
    if(Loc.IsRegister()) continue;

    auto* DefNode = AS.first;
    // collect before used by Store!
    ValUsrs.assign(DefNode->value_users().begin(),
                   DefNode->value_users().end());

    Node* SlotOffset = nullptr;
    if(Loc.IsSpilledVal()) {
      SlotOffset = SUtils.NonLocalSlotOffset(Loc.Index);
    } else {
      // (spilled) parameter slot
      SlotOffset = SUtils.SpilledParamOffset(Loc.Index);
    }
    // no need to generate store for PHI or spilled parameter
    if(DefNode->getOp() != IrOpcode::Phi &&
       DefNode->getOp() != IrOpcode::Argument) {
      auto* DefBB = Schedule.MapBlock(DefNode);
      assert(DefBB);
      auto* Store = NodeBuilder<IrOpcode::DLXStW>(&G)
                    .BaseAddr(Fp).Offset(SlotOffset)
                    .Src(DefNode).Build();
      Schedule.AddNodeAfter(DefBB, DefNode, Store);
      // also store to scratch
      ScratchCandidates.push_back(DefNode);
    }

    // FIXME: there are chances that all the operands
    // are loaded from (different) spill slots. Using
    // only one scratch register will clobber each others.
    for(auto* VU : ValUsrs) {
      // no need to re-load on PHI nodes
      if(VU->getOp() == IrOpcode::Phi) continue;
      auto* BB = Schedule.MapBlock(VU);
      // not in any block
      if(!BB) continue;
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

  return PosBefore;
}

template<class T>
void LinearScanRegisterAllocator<T>::InsertCalleeSavedCodes(Node* PosBefore) {
  // insert callee-save/restore code (i.e. pro/epilogue)
  auto* EntryBlock = Schedule.getEntryBlock();
  assert(EntryBlock);

  // save current stack position as frame pointer
  // right after Start
  auto* MoveToFP = CreateMove(RegNodes[T::StackPointer]);
  Assignment[MoveToFP] = Location::Register(T::FramePointer);
  auto* StartNode = Schedule.getStartNode();
  Schedule.AddNodeAfter(EntryBlock, StartNode, MoveToFP);

  // insert prologue
  for(int i = CalleeSaved.size() - 1; i >= 0; --i) {
    if(CalleeSaved.test(i)) {
      auto* Push = SUtils.ReserveSlots(1, RegNodes[i]);
      // insert instructions in 'reverse' order
      Schedule.AddNodeBefore(EntryBlock, PosBefore, Push);
    }
  }

  // epilogue creator
  auto createEpilogue = [&](std::vector<Node*>& Epilogue) {
    for(auto i = 0U; i < CalleeSaved.size(); ++i) {
      if(CalleeSaved.test(i)) {
        auto* Pop = SUtils.RestoreSlot(RegNodes[i]);
        Epilogue.insert(Epilogue.begin(), Pop);
      }
    }
    // restore frame pointer to stack pointer
    Epilogue.insert(
      Epilogue.begin(),
      NodeBuilder<IrOpcode::VirtDLXBinOps>(&G, IrOpcode::DLXAddI, true)
      .LHS(SUtils.FramePointer())
      .RHS(NodeBuilder<IrOpcode::ConstantInt>(&G, 0).Build())
      .Build()
    );
    Assignment[Epilogue.front()] = Location::Register(T::StackPointer);
  };
  // insert epilogue
  // terminate BB -> insert after point
  std::map<BasicBlock*, Node*> ExitPoints;
  for(auto* BB : Schedule.rpo_blocks()) {
    bool FoundTerminate = false;
    for(auto* N : BB->reverse_nodes()) {
      if(N->getOp() == IrOpcode::End ||
         N->getOp() == IrOpcode::Return ||
         N->getOp() == IrOpcode::DLXRet) {
        FoundTerminate = true;
      } else if(FoundTerminate) {
        // found insertion point
        ExitPoints.insert({BB, N});
        break;
      }
    }
  }
  std::vector<Node*> Epilogue;
  for(auto& Point : ExitPoints) {
    auto* BB = Point.first;
    auto* PosAfter = Point.second;
    Epilogue.clear();
    createEpilogue(Epilogue);
    for(auto* EPN : Epilogue) {
      Schedule.AddNodeAfter(BB, PosAfter, EPN);
    }
  }
}

template<class T>
void LinearScanRegisterAllocator<T>::InsertCallerSavedCodes() {
  for(auto& Pair : CallerSaved) {
    auto* CS = Pair.first;
    NodeProperties<IrOpcode::VirtDLXCallsiteBegin> CSNP(CS);
    assert(CSNP);
    auto* CSEnd = CSNP.getCallsiteEnd();
    auto* CSBB = Schedule.MapBlock(CS);
    assert(CSBB);
    auto& State = Pair.second;

    for(auto i = 0U; i < State.size(); ++i) {
      if(State.test(i)) {
        // save
        auto* Push = SUtils.ReserveSlots(1, RegNodes[i]);
        Schedule.AddNodeBefore(CSBB, CS, Push);
        // restore
        auto* Pop = SUtils.RestoreSlot(RegNodes[i]);
        Schedule.AddNodeAfter(CSBB, CSEnd, Pop);
      }
    }
  }
}

template<class T>
void LinearScanRegisterAllocator<T>::MemAllocationLowering() {
  // replace user of Alloca with frame pointer. And replace Alloca
  // instruction with local var reservation.
  // For global variables, replace it with global pointer.

  // there should be only one Alloca (after merged by DLXMemoryLegalize)
  auto* EntryBB = Schedule.getEntryBlock();
  Node* Alloca = nullptr;
  for(auto* N : EntryBB->nodes()) {
    if(N->getOp() == IrOpcode::Alloca) {
      Alloca = N;
      break;
    }
  }
  if(Alloca) {
    Assignment[Alloca] = Location::Register(T::FramePointer);
    size_t NumSlots = Schedule.getWordAllocaSize();
    assert(NumSlots);
    auto* Reservation = SUtils.ReserveSlots(NumSlots);
    Schedule.ReplaceNode(EntryBB, Alloca, Reservation);
  }

  for(auto* GV : G.global_vars()) {
    if(GV->getOp() == IrOpcode::Alloca)
      Assignment[GV] = Location::Register(T::GlobalPointer);
  }
}

template<class T>
void LinearScanRegisterAllocator<T>::CommitRegisterNodes() {
  // Transform to three-address instructions and replace
  // inputs with assigned registers
  auto skipInput = [](Node* VI) -> bool {
    return (VI->getOp() == IrOpcode::ConstantInt ||
            VI->getOp() == IrOpcode::DLXOffset ||
            NodeProperties<IrOpcode::VirtDLXRegisters>(VI));
  };

  for(auto* CurBB : Schedule.rpo_blocks()) {
    for(auto* CurNode : CurBB->nodes()) {
      switch(CurNode->getOp()) {
#define DLX_ARITH_OP(OC)  \
      case IrOpcode::DLX##OC: \
      case IrOpcode::DLX##OC##I:
#include "gross/Graph/DLXOpcodes.def"
      case IrOpcode::DLXLdW:
      case IrOpcode::DLXLdX:
      {
        assert(CurNode->getNumValueInput() == 2);
        std::array<Node*, 3> NewOperands;
        // result
        assert(Assignment.count(CurNode));
        auto& ResultLoc = Assignment[CurNode];
        assert(ResultLoc.IsRegister() && "still not in register?");
        NewOperands[0] = RegNodes[ResultLoc.Index];
        // inputs
        for(auto i = 0; i < 2; ++i) {
          auto* VI = CurNode->getValueInput(i);
          if(skipInput(VI)) {
            NewOperands[i + 1] = VI;
          } else {
            assert(Assignment.count(VI));
            auto& Loc = Assignment[VI];
            assert(Loc.IsRegister() && "still not in register?");
            NewOperands[i + 1] = RegNodes[Loc.Index];
          }
        }

        CurNode->setValueInput(0, NewOperands[0]);
        CurNode->setValueInput(1, NewOperands[1]);
        CurNode->appendValueInput(NewOperands[2]);
        break;
      }
      case IrOpcode::DLXStW:
      case IrOpcode::DLXStX:
      case IrOpcode::DLXRet:
      case IrOpcode::Return: {
        std::vector<Node*> ValInputs(CurNode->value_input_begin(),
                                     CurNode->value_input_end());
        for(auto* VI : ValInputs) {
          if(!skipInput(VI)) {
            assert(Assignment.count(VI));
            auto& Loc = Assignment[VI];
            assert(Loc.IsRegister() && "still not in register?");
            CurNode->ReplaceUseOfWith(VI, RegNodes[Loc.Index], Use::K_VALUE);
          }
        }
        break;
      }
      default: break;
      }
    }
  }
}

inline bool hasValueUsers(Node* N) {
  auto ValUsrs = N->value_users();
  return ValUsrs.begin() != ValUsrs.end();
}

template<class T>
void LinearScanRegisterAllocator<T>::Allocate() {
  // 1. insert 'move' for every PHI input values
  // 2. lowering function parameters
  // 3. if no register, spill
  // 4. assign register otherwise
  // 5. recycle any expired register

  // skip builtin functions
  auto* Stub
    = NodeBuilder<IrOpcode::FunctionStub>(&G, Schedule.getSubGraph())
      .Build();
  if(NodeProperties<IrOpcode::FunctionStub>(Stub)
     .hasAttribute<Attr::IsBuiltin>(G)) {
    return;
  }

  std::vector<Node*> PHINodes;
  for(auto* BB : Schedule.rpo_blocks()) {
    for(auto* N : BB->nodes()) {
      if(N->getOp() == IrOpcode::Phi &&
         N->getNumValueInput() && !N->getNumEffectInput()) {
        PHINodes.push_back(N);
      }
    }
  }
  for(auto* PN : PHINodes)
    LegalizePhiInputs(PN);

  ParametersLowering();

  for(auto* BB : Schedule.rpo_blocks()) {
    for(auto* CurNode : BB->nodes()) {
      // recycle expired register and/or spill slot
      Recycle(CurNode);

      if(CurNode->getOp() == IrOpcode::VirtDLXCallsiteBegin) {
        // record the current active registers needs to be saved
        std::bitset<NumRegister> ActiveRegs;
        // frame pointer and link register always need to be saved
        ActiveRegs[T::FramePointer] = true;
        ActiveRegs[T::LinkRegister] = true;
        for(auto i = FirstCallerSaved; i <= LastCallerSaved; ++i) {
          if(RegUsages[i]) ActiveRegs[i] = true;
        }
        for(auto i = FirstParameter; i <= LastParameter; ++i) {
          if(RegUsages[i]) ActiveRegs[i] = true;
        }
        CallerSaved[CurNode] = std::move(ActiveRegs);
        continue;
      }

      if(hasValueUsers(CurNode) &&
         !NodeProperties<IrOpcode::VirtGlobalValues>(CurNode) &&
         CurNode->getOp() != IrOpcode::DLXOffset) {
        // PHI should be handled after either of its
        // inputs values
        if(CurNode->getOp() == IrOpcode::Phi) {
          assert(Assignment.count(CurNode));
          auto& Loc = Assignment.at(CurNode);
          if(Loc.IsRegister()) {
            RegUsages[Loc.Index] = CurNode;
          } else if(Loc.IsSpilledVal()) {
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
    }
  }

  FunctionReturnLowering();

  auto* Pos = InsertSpillCodes();

  InsertCalleeSavedCodes(Pos);
  InsertCallerSavedCodes();

  CallsiteLowering();

  MemAllocationLowering();

  CommitRegisterNodes();
}

namespace gross {
void __SupportedLinearScanRATargets(GraphSchedule& Schedule) {
  LinearScanRegisterAllocator<DLXTargetTraits> DLX(Schedule);
  DLX.Allocate();
  (void) DLX.GetAllocation(nullptr);

  LinearScanRegisterAllocator<CompactDLXTargetTraits> DLXLite(Schedule);
  DLXLite.Allocate();
  (void) DLXLite.GetAllocation(nullptr);
}
} // end namespace gross
