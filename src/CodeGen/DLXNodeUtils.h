#ifndef GROSS_CODEGEN_DLXNODEUTILS_H
#define GROSS_CODEGEN_DLXNODEUTILS_H
#include "gross/Graph/NodeUtilsBase.h"

namespace gross {
NODE_PROPERTIES(VirtDLXOps) {
  NodeProperties(Node* N)
    : NODE_PROP_BASE(VirtDLXOps, N) {}

  operator bool() const {
    if(!NodePtr) return false;
    switch(NodePtr->getOp()) {
#define DLX_ARITH_OP(OC)  \
    case IrOpcode::DLX##OC:  \
    case IrOpcode::DLX##OC##I:
#define DLX_COMMON(OC)  \
    case IrOpcode::DLX##OC:
#include "gross/Graph/DLXOpcodes.def"
      return true;
    default:
      return false;
    }
  }
};
} // end namespace gross
#endif
