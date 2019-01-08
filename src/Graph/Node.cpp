#include "gross/Graph/Node.h"

using namespace gross;

Node::Node(IrOpcode::ID OC,
           const std::initializer_list<Node*>& ValueInputs,
           const std::initializer_list<Node*>& ControlInputs,
           const std::initializer_list<Node*>& EffectInputs)
  : Op(OC),
    NumValueInput(ValueInputs.size()),
    NumControlInput(ControlInputs.size()),
    NumEffectInput(EffectInputs.size()) {
  if(NumEffectInput > 0)
    Inputs.insert(Inputs.begin(),
                  EffectInputs.begin(), EffectInputs.end());
  if(NumControlInput > 0)
    Inputs.insert(Inputs.begin(),
                  ControlInputs.begin(), ControlInputs.end());
  if(NumValueInput > 0)
    Inputs.insert(Inputs.begin(),
                  ValueInputs.begin(), ValueInputs.end());
}
