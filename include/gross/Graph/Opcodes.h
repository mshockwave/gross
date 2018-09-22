#ifndef GROSS_GRAPH_OPCODES_H
#define GROSS_GRAPH_OPCODES_H
namespace gross {
namespace IrOpcode {
enum ID : unsigned {
  None = 0,
  // Common opcodes
  ConstantInt,
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
  Start,
  End,
  Return,
  Call,
  // High-level primitives
  SrcSymbolName,
  SrcVarDecl,
  SrcArrayDecl,
  SrcArrayAccess,
  SrcIfStmt,
  SrcWhileStmt,
  SrcAssignStmt,
  // Low(machine)-level opcodes
  DLXJmp
};
} // end namespace IrOpcode
} // end namespace gross
#endif
