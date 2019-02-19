#ifndef GROSS_GRAPH_OPCODES_H
#define GROSS_GRAPH_OPCODES_H
namespace gross {
namespace IrOpcode {
enum ID : unsigned {
  None = 0,
  Dead,
  // Common opcodes
  ConstantInt,
  ConstantStr,
  BinAdd,
  BinSub,
  BinMul,
  BinDiv,
  BinLe,
  BinLt,
  BinGe,
  BinGt,
  BinEq,
  BinNe,
  // Control opcodes
  If,
  IfTrue,
  IfFalse,
  Region,
  Start, // start of a function
  End,   // end of a function
  Merge, // control flow merge
  Phi,
  Loop,
  Return,
  Call,
  Argument,
  FunctionStub, // Represent as function callee. Also allow additional metadata
                // to be attached on it (e.g. no_mem).
                // Since we use some lazy node traversal to represent Function
                // subgraph, pointing to real function body at callsite will be
                // a bad idea(i.e. causing incorrect subgraph covering), thus we
                // use function stub instead. Also, a stub instance is a single-
                // ton, that is, global values in our framework.
  Attribute,
  MemLoad,
  MemStore,
  // High-level primitives
  SrcVarDecl,
  SrcVarAccess,
  SrcArrayDecl,
  SrcArrayAccess,
  SrcWhileStmt,
  SrcAssignStmt,
  // Low(machine)-level opcodes
#define DLX_ARITH_OP(OC)  \
  DLX##OC,  \
  DLX##OC##I,
#define DLX_COMMON(OC) DLX##OC,
#define DLX_MEM_OP(OC) DLX_COMMON(OC)
#define DLX_CTRL_OP(OC) DLX_COMMON(OC)
#define DLX_BUILTIN(OC) DLX_COMMON(OC)
#include "DLXOpcodes.def"
#undef DLX_BUILTIN
#undef DLX_CTRL_OP
#undef DLX_MEM_OP
#undef DLX_COMMON
#undef DLX_ARITH_OP
  // Virtual opcodes: abtraction nodes for several
  // opcodes with similar properties
  VirtSrcDecl,        // SrcVarDecl | SrcArrayDecl
  VirtBinOps,         // Bin*
  VirtSrcDesigAccess, // SrcVarAccess | SrcArrayAccess
  VirtIfBranches,     // IfTrue | IfFalse
  VirtFuncPrototype,  // Start + Argument(s)
  VirtConstantValues, // Dead | ConstantInt | ConstantStr
  VirtGlobalValues,   // Dead | ConstantInt | ConstantStr | Start | End
                      // | FunctionStub
  VirtCtrlPoints,     // If | IfTrue | IfFalse | Merge | Start | End
                      // | Return | Loop
  VirtMemOps          // MemLoad | MemStore
};
} // end namespace IrOpcode
} // end namespace gross
#endif
