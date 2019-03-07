#ifndef GROSS_GRAPH_OPCODES_H
#define GROSS_GRAPH_OPCODES_H
#include <iostream>

namespace gross {
// Forward declarations
class Graph;
class Node;

namespace IrOpcode {
enum ID : unsigned {
  None = 0,
  /// Frontend and middle-end opcodes
#define COMMON_OP(OC) OC,
#define CONST_OP(OC) OC,
#define CONTROL_OP(OC) OC,
#define MEMORY_OP(OC) OC,
#define INTERPROC_OP(OC)  OC,
#define SRC_OP(OC)  Src##OC,
#include "Opcodes.def"

  /// Machine level opcodes
#define DLX_ARITH_OP(OC)  \
  DLX##OC,  \
  DLX##OC##I,
#define DLX_COMMON(OC) DLX##OC,
#define DLX_CONST(OC) DLX_COMMON(OC)
#include "DLXOpcodes.def"

  /// Virtual opcodes: abtraction nodes for several
  /// opcodes with similar properties
#define VIRT_OP(OC) Virt##OC,
#include "Opcodes.def"
#define VIRT_OP(OC) Virt##OC,
#include "DLXOpcodes.def"
};

std::ostream& Print(const Graph& G, std::ostream& OS, Node* N);
} // end namespace IrOpcode
} // end namespace gross
#endif
