#include "DLXNodeUtils.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/CodeGen/GraphScheduling.h"

using namespace gross;

StackUtils::StackUtils(GraphSchedule& schedule)
  : Schedule(schedule),
    G(Schedule.getGraph()),
    Fp(NodeBuilder<FpReg>(&G).Build()),
    Sp(NodeBuilder<SpReg>(&G).Build()){}

void StackUtils::ReserveSlots(size_t Num, std::vector<Node*>& Result) {
  // stack grow from high to low
  auto* OffsetNode
    = NodeBuilder<IrOpcode::ConstantInt>(&G, 4 * -1)
      .Build();
  auto* R0 = NodeBuilder<IrOpcode::DLXr0>(&G).Build();

  for(auto i = 0U; i < Num; ++i) {
    auto* N = NodeBuilder<IrOpcode::VirtDLXTriOps>(&G, IrOpcode::DLXPush)
              .SetVal<0>(R0) // pushed value
              .SetVal<1>(Sp) // base address
              .SetVal<2>(OffsetNode) // offset
              .Build();
    Result.push_back(N);
  }
}

Node* StackUtils::ReserveSlots(size_t Num, Node* Val) {
  auto SignNum = static_cast<int32_t>(Num);
  // stack grow from high to low
  auto* OffsetNode
    = NodeBuilder<IrOpcode::ConstantInt>(&G, SignNum * 4 * -1)
      .Build();
  if(!Val)
    Val = NodeBuilder<IrOpcode::DLXr0>(&G).Build();

  auto* N = NodeBuilder<IrOpcode::VirtDLXTriOps>(&G, IrOpcode::DLXPush)
            .SetVal<0>(Val) // pushed value
            .SetVal<1>(Sp) // base address
            .SetVal<2>(OffsetNode) // offset
            .Build();
  return N;
}

Node* StackUtils::RestoreSlot(Node* Dest) {
  auto* OffsetNode
    = NodeBuilder<IrOpcode::ConstantInt>(&G, 4)
      .Build();
  auto* N = NodeBuilder<IrOpcode::VirtDLXTriOps>(&G, IrOpcode::DLXPop)
            .SetVal<0>(Dest) // poped destination
            .SetVal<1>(Sp) // base address
            .SetVal<2>(OffsetNode) // offset
            .Build();
  return N;
}

Node* StackUtils::NonLocalSlotOffset(size_t Idx) {
  size_t AllocaSlots = Schedule.getWordAllocaSize();
  auto Offset = static_cast<int32_t>(AllocaSlots + Idx + 1) * 4 * -1;
  auto* OffsetNode
    = NodeBuilder<IrOpcode::ConstantInt>(&G, Offset).Build();
  return OffsetNode;
}

Node* StackUtils::SpilledParamOffset(size_t Idx) {
  auto Offset = static_cast<int32_t>(Idx) * 4;
  auto* OffsetNode
    = NodeBuilder<IrOpcode::ConstantInt>(&G, Offset).Build();
  return OffsetNode;
}
