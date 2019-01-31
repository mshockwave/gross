#ifndef GROSS_GRAPH_OPCODES_H
#define GROSS_GRAPH_OPCODES_H
namespace gross {
namespace IrOpcode {
enum ID : unsigned {
  None = 0,
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
  VirtGlobalValues,   // ConstantInt + ConstantStr + Start
  VirtCtrlPoints      // If + IfTrue + IfFalse + Merge + Start + End
                      // + Return
};
} // end namespace IrOpcode
} // end namespace gross
#endif
