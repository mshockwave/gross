#ifndef GROSS_CODEGEN_INSTRUCTION_H
#define GROSS_CODEGEN_INSTRUCTION_H
#include "gross/Graph/Node.h"

namespace gross {
class Instruction {
  Node* NodePtr;

public:
  Instruction() = delete;
  Instruction(Node* N);
};
} // end namespace gross
#endif
