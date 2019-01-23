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
  Region,
  Start,
  End,
  Phi,
  Return,
  Call,
  Arguments,
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
  VirtBinOps,         // Bin*
  VirtSrcDesigAccess  // SrcVarAccess | SrcArrayAccess
};
} // end namespace IrOpcode
} // end namespace gross
#endif
