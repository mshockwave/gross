#ifndef GROSS_FRONTEND_PARSERTEST_H
#define GROSS_FRONTEND_PARSERTEST_H
/// Some utilities only for parser unittests
#include "Parser.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"

namespace gross {
inline void SetMockContext(Parser& P, Graph& G) {
  P.NewSymScope();
  auto* DummyFunc = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                    .FuncName("dummy_func")
                    .Build();
  P.setLastCtrlPoint(DummyFunc);
}
} // end namespace gross
#endif
