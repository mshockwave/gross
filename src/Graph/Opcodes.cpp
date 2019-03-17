#include "gross/Graph/Opcodes.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/Node.h"
#include "gross/Graph/NodeUtils.h"

using namespace gross;

std::ostream& IrOpcode::Print(const Graph& G, std::ostream& OS, Node* N) {
#define STR(V) #V
#define CASE(OC, S)  \
  case IrOpcode::OC:  \
    OS << S;  \
    break;
  switch(N->getOp()) {
  CASE(Dead, STR(Dead))
  case IrOpcode::ConstantInt: {
    OS << "ConstInt<"
       << NodeProperties<IrOpcode::ConstantInt>(N).as<int32_t>(G)
       << ">";
    break;
  }
  case IrOpcode::ConstantStr: {
    OS << "ConstStr<"
       << NodeProperties<IrOpcode::ConstantStr>(N).str(G)
       << ">";
    break;
  }
  case IrOpcode::FunctionStub: {
    auto* FuncStart = NodeProperties<IrOpcode::FunctionStub>(N)
                      .getFunctionStart(G);
    std::string Name = "N/A";
    if(FuncStart)
      Name = NodeProperties<IrOpcode::Start>(FuncStart).name(G);
    OS << "FunctionStub<" << Name << ">";
    break;
  }
  case IrOpcode::Call: {
    assert(N->getNumValueInput() > 0);
    auto* FuncStub = N->getValueInput(0);
    assert(FuncStub->getOp() == IrOpcode::FunctionStub);
    auto* FuncStart = NodeProperties<IrOpcode::FunctionStub>(FuncStub)
                      .getFunctionStart(G);
    std::string Name = "N/A";
    if(FuncStart)
      Name = NodeProperties<IrOpcode::Start>(FuncStart).name(G);
    OS << "Call<" << Name << ">";
    break;
  }
  case IrOpcode::Argument: {
    // print index of argument as well
    NodeProperties<IrOpcode::Argument> NP(N);
    auto* Func = NP.getFuncStart();
    size_t Idx = 0;
    for(auto* Arg : Func->effect_inputs()) {
      if(Arg == N) break;
      ++Idx;
    }
    OS << "Arg<" << Idx << ">";
    break;
  }
#define COMMON_OP(OC) CASE(OC, STR(OC))
#define CONTROL_OP(OC) CASE(OC, STR(OC))
#define MEMORY_OP(OC) CASE(OC, STR(OC))
#define SRC_OP(OC) CASE(Src##OC, STR(Src##OC))
#define VIRT_OP(OC) CASE(Virt##OC, STR(Virt##OC))
#include "gross/Graph/Opcodes.def"
#define DLX_ARITH_OP(OC)  \
  CASE(DLX##OC, STR(DLX##OC))  \
  CASE(DLX##OC##I, STR(DLX##OC##I))
#define DLX_COMMON(OC) CASE(DLX##OC, STR(DLX##OC))
// FIXME: we really want to print DLXOffset w/ BB or function it represents
#define DLX_CONST(OC) DLX_COMMON(OC)
#define VIRT_OP(OC) CASE(Virt##OC, STR(Virt##OC))
#include "gross/Graph/DLXOpcodes.def"
  default:
    gross_unreachable("Opcodes can't be printed");
  }
  return OS;
#undef CASE
#undef STR
}
