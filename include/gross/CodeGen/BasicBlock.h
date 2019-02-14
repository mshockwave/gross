#ifndef GROSS_CODEGEN_BASICBLOCK_H
#define GROSS_CODEGEN_BASICBLOCK_H
#include "gross/Graph/Node.h"
#include <list>

namespace gross {
class BasicBlock {
  std::list<Node*> InstrSequence;

public:
  BasicBlock() = default;
};
} // end namespace gross
#endif
