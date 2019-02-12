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
  FunctionStub, // Represent as function callee. Also allow additional metadata
                // to be attached on it (e.g. no_mem).
                // Since we use some lazy node traversal to represent Function
                // subgraph, pointing to real function body at callsite will be
                // a bad idea(i.e. causing incorrect subgraph covering), thus we
                // use function stub instead. Also, a stub instance is a single-
                // ton, that is, global values in our framework.
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
                      // | FunctionStub
  VirtCtrlPoints,     // If | IfTrue | IfFalse | Merge | Start | End
                      // | Return | Loop
  VirtMemOps          // MemLoad | MemStore
};
} // end namespace IrOpcode
} // end namespace gross
#endif
