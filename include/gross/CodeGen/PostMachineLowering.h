#ifndef GROSS_CODEGEN_POSTMACHINELOWERING_H
#define GROSS_CODEGEN_POSTMACHINELOWERING_H
#include "gross/Graph/GraphReducer.h"

namespace gross {
class PostMachineLowering : public GraphEditor {
  Graph &G;

public:
  PostMachineLowering(GraphEditor::Interface* editor);

  static constexpr
  const char* name() { return "post-machine-lowering"; }

  GraphReduction Reduce(Node* N);
};
} // end namespace gross
#endif
