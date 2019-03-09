#ifndef GROSS_CODEGEN_POSTMACHINELOWERING_H
#define GROSS_CODEGEN_POSTMACHINELOWERING_H
#include "gross/CodeGen/GraphScheduling.h"

namespace gross {
class PostMachineLowering {
  GraphSchedule& Schedule;
  Graph& G;

  void ControlFlowLowering();

  void FunctionCallLowering();

  void Trimming();

public:
  PostMachineLowering(GraphSchedule& schedule);

  void Run();
};
} // end namespace gross
#endif
