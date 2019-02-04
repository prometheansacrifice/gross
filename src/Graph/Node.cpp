#include "gross/Graph/Node.h"
#include "gross/Support/STLExtras.h"
#include <iterator>

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

void Node::appendNodeInput(unsigned& Size, unsigned Offset,
                           Node* NewNode) {
  auto It = Inputs.cbegin();
  std::advance(It, Size + Offset);
  Inputs.insert(It, NewNode);
  Size += 1;
  NewNode->Users.push_back(this);
}

void Node::setNodeInput(unsigned Index, unsigned Size, unsigned Offset,
                        Node* NewNode) {
  assert(Index < Size);
  Index += Offset;
  Size += Offset;
  Node* OldNode = Inputs[Index];
  auto It = std::find(OldNode->Users.cbegin(), OldNode->Users.cend(),
                      this);
  assert(It != OldNode->Users.cend());
  OldNode->Users.erase(It);
  Inputs[Index] = NewNode;
  NewNode->Users.push_back(this);
}

void Node::setValueInput(unsigned Index, Node* NewNode) {
  setNodeInput(Index, NumValueInput, 0, NewNode);
}
void Node::appendValueInput(Node* NewNode) {
  appendNodeInput(NumValueInput, 0, NewNode);
}
void Node::setControlInput(unsigned Index, Node* NewNode) {
  setNodeInput(Index, NumControlInput, NumValueInput, NewNode);
}
void Node::appendControlInput(Node* NewNode) {
  appendNodeInput(NumControlInput, NumValueInput, NewNode);
}
void Node::setEffectInput(unsigned Index, Node* NewNode) {
  setNodeInput(Index, NumEffectInput, NumValueInput + NumControlInput, NewNode);
}
void Node::appendEffectInput(Node* NewNode) {
  appendNodeInput(NumEffectInput, NumValueInput + NumControlInput,
                  NewNode);
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
    gross_unreachable("Invalid Use Kind");
    break;
  case Use::K_VALUE: {
    for(Node* Usr : value_users())
      Usr->ReplaceUseOfWith(this, Replacement, UseKind);
    break;
  }
  case Use::K_CONTROL: {
    for(Node* Usr : control_users())
      Usr->ReplaceUseOfWith(this, Replacement, UseKind);
    break;
  }
  case Use::K_EFFECT: {
    for(Node* Usr : effect_users())
      Usr->ReplaceUseOfWith(this, Replacement, UseKind);
    break;
  }
  }
}
