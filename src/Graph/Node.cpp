#include "gross/Graph/Node.h"
#include "gross/Support/STLExtras.h"
#include <iterator>
#include <utility>

using namespace gross;

Node::Node(IrOpcode::ID OC,
           const std::vector<Node*>& ValueInputs,
           const std::vector<Node*>& ControlInputs,
           const std::vector<Node*>& EffectInputs)
  : Op(OC),
    MarkerData(0U),
    NumValueInput(ValueInputs.size()),
    NumControlInput(ControlInputs.size()),
    NumEffectInput(EffectInputs.size()),
    IsKilled(false) {
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

void Node::appendNodeInput(unsigned& Size, unsigned Offset,
                           Node* NewNode) {
  auto It = Inputs.begin();
  std::advance(It, Size + Offset);
  Inputs.insert(It, NewNode);
  Size += 1;
  NewNode->Users.push_back(this);
}

void Node::setNodeInput(unsigned Index, unsigned Size, unsigned Offset,
                        Node* NewNode) {
  Index += Offset;
  Size += Offset;
  assert(Index < Size);
  Node* OldNode = Inputs[Index];
  auto It = std::find(OldNode->Users.begin(), OldNode->Users.end(),
                      this);
  assert(It != OldNode->Users.end());
  OldNode->Users.erase(It);
  Inputs[Index] = NewNode;
  NewNode->Users.push_back(this);
}

void Node::cleanupRemoveNodeInput(Node* OldInput) {
  auto It = std::find(OldInput->Users.begin(), OldInput->Users.end(),
                      this);
  assert(It != OldInput->Users.end());
  OldInput->Users.erase(It);
}

void Node::removeNodeInput(unsigned Index, unsigned& Size, unsigned Offset) {
  unsigned S = Size;
  Index += Offset;
  S += Offset;
  assert(Index < S);
  Node* OldNode = Inputs[Index];
  cleanupRemoveNodeInput(OldNode);
  Inputs.erase(Inputs.begin() + Index);
  Size -= 1;
}

void Node::removeNodeInputAll(Node* Target, unsigned& Size, unsigned Offset) {
  auto I = Inputs.begin() + Offset;
  while(I != (Inputs.begin() + Size + Offset)) {
    auto* N = *I;
    if(N == Target) {
      // remove
      cleanupRemoveNodeInput(N);
      I = Inputs.erase(I);
      --Size;
    } else {
      ++I;
    }
  }
}

void Node::setValueInput(unsigned Index, Node* NewNode) {
  setNodeInput(Index, NumValueInput, 0, NewNode);
}
void Node::appendValueInput(Node* NewNode) {
  appendNodeInput(NumValueInput, 0, NewNode);
}
void Node::removeValueInput(unsigned Index) {
  removeNodeInput(Index, NumValueInput, 0);
}
void Node::removeValueInputAll(Node* N) {
  removeNodeInputAll(N, NumValueInput, 0);
}

void Node::setControlInput(unsigned Index, Node* NewNode) {
  setNodeInput(Index, NumControlInput, NumValueInput, NewNode);
}
void Node::appendControlInput(Node* NewNode) {
  appendNodeInput(NumControlInput, NumValueInput, NewNode);
}
void Node::removeControlInput(unsigned Index) {
  removeNodeInput(Index, NumControlInput, NumValueInput);
}
void Node::removeControlInputAll(Node* N) {
  removeNodeInputAll(N, NumControlInput, NumValueInput);
}

void Node::setEffectInput(unsigned Index, Node* NewNode) {
  setNodeInput(Index, NumEffectInput, NumValueInput + NumControlInput, NewNode);
}
void Node::appendEffectInput(Node* NewNode) {
  appendNodeInput(NumEffectInput, NumValueInput + NumControlInput,
                  NewNode);
}
void Node::removeEffectInput(unsigned Index) {
  removeNodeInput(Index, NumEffectInput, NumValueInput + NumControlInput);
}
void Node::removeEffectInputAll(Node* N) {
  removeNodeInputAll(N, NumEffectInput, NumValueInput + NumControlInput);
}

void Node::Kill(Node* DeadNode) {
  // remove inputs
  for(auto i = 0U, N = NumValueInput; i < N; ++i)
    setValueInput(i, DeadNode);
  for(auto i = 0U, N = NumControlInput; i < N; ++i)
    setControlInput(i, DeadNode);
  for(auto i = 0U, N = NumEffectInput; i < N; ++i)
    setEffectInput(i, DeadNode);

  // replace all uses with Dead node
  ReplaceWith(DeadNode, Use::K_VALUE);
  ReplaceWith(DeadNode, Use::K_CONTROL);
  ReplaceWith(DeadNode, Use::K_EFFECT);

  IsKilled = true;
}

llvm::iterator_range<Node::value_user_iterator>
Node::value_users() {
  is_value_use Pred(this);
  value_user_iterator it_begin(Pred, Users.begin(), Users.end()),
                      it_end(Pred, Users.end(), Users.end());
  return llvm::make_range(it_begin, it_end);
}
llvm::iterator_range<Node::control_user_iterator>
Node::control_users() {
  is_control_use Pred(this);
  control_user_iterator it_begin(Pred, Users.begin(), Users.end()),
                        it_end(Pred, Users.end(), Users.end());
  return llvm::make_range(it_begin, it_end);
}
llvm::iterator_range<Node::effect_user_iterator>
Node::effect_users() {
  is_effect_use Pred(this);
  effect_user_iterator it_begin(Pred, Users.begin(), Users.end()),
                       it_end(Pred, Users.end(), Users.end());
  return llvm::make_range(it_begin, it_end);
}

bool Node::ReplaceUseOfWith(Node* From, Node* To, Use::Kind UseKind) {
  switch(UseKind) {
  case Use::K_NONE:
    gross_unreachable("Invalid Use Kind");
    break;
  case Use::K_VALUE: {
    auto It = gross::find(value_inputs(), From);
    if(It == value_inputs().end()) return false;
    auto Idx = std::distance(value_inputs().begin(), It);
    setValueInput(Idx, To);
    break;
  }
  case Use::K_CONTROL: {
    auto It = gross::find(control_inputs(), From);
    if(It == control_inputs().end()) return false;
    auto Idx = std::distance(control_inputs().begin(), It);
    setControlInput(Idx, To);
    break;
  }
  case Use::K_EFFECT: {
    auto It = gross::find(effect_inputs(), From);
    if(It == effect_inputs().end()) return false;
    auto Idx = std::distance(effect_inputs().begin(), It);
    setEffectInput(Idx, To);
    break;
  }
  }
  return true;
}

void Node::ReplaceWith(Node* Replacement, Use::Kind UseKind) {
  switch(UseKind) {
  case Use::K_NONE:
    // replace every category of uses
    ReplaceWith(Replacement, Use::K_VALUE);
    ReplaceWith(Replacement, Use::K_CONTROL);
    ReplaceWith(Replacement, Use::K_EFFECT);
    break;
  case Use::K_VALUE: {
    std::vector<Node*> Usrs(value_users().begin(), value_users().end());
    for(Node* Usr : Usrs)
      Usr->ReplaceUseOfWith(this, Replacement, UseKind);
    break;
  }
  case Use::K_CONTROL: {
    std::vector<Node*> Usrs(control_users().begin(), control_users().end());
    for(Node* Usr : Usrs)
      Usr->ReplaceUseOfWith(this, Replacement, UseKind);
    break;
  }
  case Use::K_EFFECT: {
    std::vector<Node*> Usrs(effect_users().begin(), effect_users().end());
    for(Node* Usr : Usrs)
      Usr->ReplaceUseOfWith(this, Replacement, UseKind);
    break;
  }
  }
}
