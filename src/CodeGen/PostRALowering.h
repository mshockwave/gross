#ifndef GROSS_CODEGEN_POSTRALOWERING_H
#define GROSS_CODEGEN_POSTRALOWERING_H
#include "gross/CodeGen/GraphScheduling.h"
#include "RegisterAllocator.h"
#include <functional>

namespace gross {
struct PostRALowering {
  using RAQueryFunctorTy
    = std::function<const RegisterAllocator::Location&(Node*)>;

  PostRALowering(GraphSchedule&, RAQueryFunctorTy = nullptr);

  void Run();

private:
  GraphSchedule& Schedule;
  Graph& G;
  RAQueryFunctorTy RAQuery;

  static constexpr size_t PeepholeMaxIterations = 10;
  void RunPeepholes();
  // peephole callbacks. Return true if changes
  bool VisitBlock(BasicBlock*);
  bool TrimNode(Node*);
  bool VisitDLXAdd(Node*);
  bool VisitDLXLoad(Node*);
};
} // end namespace gross
#endif
