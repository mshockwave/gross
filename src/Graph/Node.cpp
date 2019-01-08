#include "gross/Graph/Node.h"

using namespace gross;

Node::Node(IrOpcode::ID OC,
           const std::vector<Node*>& ValueInputs,
           const std::vector<Node*>& ControlInputs,
           const std::vector<Node*>& EffectInputs)
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
