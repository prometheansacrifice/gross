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
  Return,
  Call,
  Arguments,
  MemLoad,
  MemStore,
  // High-level primitives
  SrcVarDecl,
  SrcArrayDecl,
  SrcArrayAccess,
  SrcWhileStmt,
  SrcAssignStmt,
  // Low(machine)-level opcodes
  DLXJmp
};
} // end namespace IrOpcode
} // end namespace gross
#endif