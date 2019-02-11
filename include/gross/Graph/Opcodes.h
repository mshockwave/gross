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
  DLXJmp,
  // Virtual opcodes: abtraction nodes for several
  // opcodes with similar properties
  VirtSrcDecl,        // SrcVarDecl | SrcArrayDecl
  VirtBinOps,         // Bin*
  VirtSrcDesigAccess, // SrcVarAccess | SrcArrayAccess
  VirtIfBranches,     // IfTrue | IfFalse
  VirtFuncPrototype,  // Start + Argument(s)
  VirtGlobalValues,   // Dead | ConstantInt | ConstantStr | Start | End
  VirtCtrlPoints,     // If | IfTrue | IfFalse | Merge | Start | End
                      // | Return | Loop
  VirtMemOps          // MemLoad | MemStore
};
} // end namespace IrOpcode
} // end namespace gross
#endif
