#include "gross/Graph/Reductions/ValuePromotion.h"
#include "gross/Graph/NodeUtils.h"
#include <vector>

using namespace gross;

ValuePromotion::ValuePromotion(GraphEditor::Interface* editor)
  : GraphEditor(editor),
    G(Editor->GetGraph()),
    DeadNode(NodeBuilder<IrOpcode::Dead>(&G).Build()) {}

// Append value usage only on meaningful nodes
void ValuePromotion::appendValueUsage(Node* Usr, Node* Src, Node* Val) {
  switch(Usr->getOp()) {
  case IrOpcode::End:
  case IrOpcode::Start:
  case IrOpcode::IfTrue:
  case IrOpcode::IfFalse:
  case IrOpcode::Loop:
    // these ctrl nodes will not use the defined values
    return;
  case IrOpcode::Phi: {
    // special handle for PHI
    // reserve space for value inputs
    while(Usr->getNumValueInput() < Usr->getNumEffectInput()) {
      Usr->appendValueInput(DeadNode);
    }
    size_t InputIdx = 0U;
    for(; InputIdx < Usr->getNumEffectInput(); ++InputIdx) {
      if(Src == Usr->getEffectInput(InputIdx)) break;
    }
    assert(InputIdx < Usr->getNumEffectInput());
    Usr->setValueInput(InputIdx, Val);
    break;
  }
  default:
    Usr->appendValueInput(Val);
  }
}

// Replace value generated by SrcAssignStmt with value on RHS.
GraphReduction ValuePromotion::ReduceAssignment(Node* Assign) {
  NodeProperties<IrOpcode::SrcAssignStmt> NP(Assign);
  assert(NP);
  if(NodeProperties<IrOpcode::SrcArrayAccess>(NP.dest())) {
    // array access hasn't been reduced to MemLoad
    Revisit(Assign);
    return NoChange();
  }
  if(NodeProperties<IrOpcode::MemLoad>(NP.dest()))
    return ReduceMemAssignment(Assign);

  auto* SrcVal = NP.source();
  std::vector<Node*> EffectUsrs(Assign->effect_users().begin(),
                                Assign->effect_users().end());
  for(Node* Usr : EffectUsrs) {
    appendValueUsage(Usr, Assign, SrcVal);
  }
  return Replace(DeadNode);
}

GraphReduction ValuePromotion::ReduceMemAssignment(Node* Assign) {
  NodeProperties<IrOpcode::SrcAssignStmt> NP(Assign);
  assert(NP && NP.dest()->getOp() == IrOpcode::MemLoad);

  // construction MemStore from MemLoad
  auto* MemLoadNode = NP.dest();
  NodeProperties<IrOpcode::MemLoad> LNP(MemLoadNode);
  auto* MemStoreNode = NodeBuilder<IrOpcode::MemStore>(&G)
                       .BaseAddr(LNP.BaseAddr())
                       .Offset(LNP.Offset())
                       .Src(NP.source())
                       .Build();
  // propagate side effects and control deps
  for(auto* E : MemLoadNode->effect_inputs())
    MemStoreNode->appendEffectInput(E);
  for(auto* E : Assign->effect_inputs())
    MemStoreNode->appendEffectInput(E);
  for(auto* C : MemLoadNode->control_inputs())
    MemStoreNode->appendControlInput(C);
  for(auto* C : Assign->control_inputs())
    MemStoreNode->appendControlInput(C);

  return Replace(MemStoreNode);
}

// Replace value dep on SrcVarAccess(SrcArrayAccess) with
// value dep on the source value.
GraphReduction ValuePromotion::ReduceVarAccess(Node* VarAccess) {
  if(VarAccess->getNumValueInput() > 1) {
    // one is the original decl the other is the
    // newly promoted value
    assert(VarAccess->getNumValueInput() == 2);
    auto* PromotedVal = VarAccess->getValueInput(1);
    return Replace(PromotedVal);
  } else if(VarAccess->getNumValueInput() == 1 &&
            // effect input might be reduced to value input later
            VarAccess->getNumEffectInput() == 0) {
    auto* Decl = VarAccess->getValueInput(0);
    Node* AllocaNode;
    if(Decl->getOp() == IrOpcode::Alloca) {
      AllocaNode = Decl;
    } else if(Decl->getOp() == IrOpcode::SrcVarDecl) {
      // we really need a stack slot
      auto* WordSize = NodeBuilder<IrOpcode::ConstantInt>(&G, 4)
                       .Build();
      AllocaNode = NodeBuilder<IrOpcode::Alloca>(&G)
                   .Size(WordSize)
                   .Build();
      Replace(Decl, AllocaNode);
    } else {
      return NoChange();
    }
    return Replace(AllocaNode);
  } else {
    return NoChange();
  }
}

// Reduce to MemLoad first, then transform to MemStore if needed
GraphReduction ValuePromotion::ReduceMemAccess(Node* MemAccess) {
  NodeProperties<IrOpcode::SrcArrayAccess> NP(MemAccess);
  assert(NP);

  Node* ArrayDecl = NP.decl();
  NodeProperties<IrOpcode::SrcArrayDecl> DNP(ArrayDecl);
  assert(DNP);
  auto DimSize = NP.dim_size(),
       DeclDimSize = DNP.dim_size();
  assert(DimSize == DeclDimSize && DimSize > 0);
  std::vector<Node*> Accums;
  for(auto I = 0U, DI = 0U; I < DimSize - 1 && DI < DeclDimSize - 1;
      ++I, ++DI) {
    auto* M = NodeBuilder<IrOpcode::BinMul>(&G)
              .LHS(NP.dim(I)).RHS(DNP.dim(DI))
              .Build();
    Accums.push_back(M);
  }
  Accums.push_back(NP.dim(DimSize - 1));

  auto IA = Accums.begin();
  auto* OffsetNode = *IA;
  ++IA;
  for(auto EA = Accums.end(); IA != EA; ++IA) {
    OffsetNode = NodeBuilder<IrOpcode::BinAdd>(&G)
                 .LHS(OffsetNode).RHS(*IA)
                 .Build();
  }
  // memory operations are byte-addressing
  auto* WordSize = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
  OffsetNode = NodeBuilder<IrOpcode::BinMul>(&G)
               .LHS(OffsetNode).RHS(WordSize)
               .Build();
  auto* MemLoadNode = NodeBuilder<IrOpcode::MemLoad>(&G)
                      .BaseAddr(ArrayDecl).Offset(OffsetNode)
                      .Build();

  // propagate side effects and control deps
  for(auto* E : MemAccess->effect_inputs())
    MemLoadNode->appendEffectInput(E);
  for(auto* C : MemAccess->control_inputs())
    MemLoadNode->appendControlInput(C);
  // break the link between ArrayDecl and ArrayAccess
  MemAccess->removeValueInputAll(ArrayDecl);
  // revisit ArrayDecl later in order to reduce to Alloca
  Revisit(ArrayDecl);

  return Replace(MemLoadNode);
}

// reduce to Alloca after all users are not SrcArrayAccess anymore
GraphReduction ValuePromotion::ReduceArrayDecl(Node* ArrayDecl) {
  NodeProperties<IrOpcode::SrcArrayDecl> DNP(ArrayDecl);
  assert(DNP);
  for(auto* U : ArrayDecl->value_users())
    if(U->getOp() == IrOpcode::SrcArrayAccess) {
      return NoChange();
    }

  // calculate the total size needed
  auto DimSize = DNP.dim_size();
  auto* Accum = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
  for(auto I = 0U; I < DimSize; ++I) {
    Accum = NodeBuilder<IrOpcode::BinMul>(&G)
            .LHS(Accum).RHS(DNP.dim(I))
            .Build();
  }

  auto* WordSize = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
  Accum = NodeBuilder<IrOpcode::BinMul>(&G)
          .LHS(Accum).RHS(WordSize)
          .Build();
  auto* AllocaNode = NodeBuilder<IrOpcode::Alloca>(&G)
                     .Size(Accum).Build();
  if(G.IsGlobalVar(ArrayDecl)) {
    G.ReplaceGlobalVar(ArrayDecl, AllocaNode);
  }
  return Replace(AllocaNode);
}

// Simlinar to ReduceVarAccess, if all the effect inputs of this PHI
// has been eliminated and there are same amount of value input, replace
// all the effect user(of this node) into value usages.
GraphReduction ValuePromotion::ReducePhiNode(Node* PHI) {
  unsigned NumDead = 0;
  unsigned NumValidInput = 0;
  for(auto* EI : PHI->effect_inputs()){
    if(EI == DeadNode) ++NumDead;
  }
  for(auto* VI : PHI->value_inputs()) {
    if(VI != DeadNode) ++NumValidInput;
  }
  if(NumDead && NumDead == NumValidInput) {
    // replace all effect usages with value usages
    std::vector<Node*> EUsrs(PHI->effect_users().begin(),
                             PHI->effect_users().end());
    for(auto* EU : EUsrs) {
      appendValueUsage(EU, PHI, PHI);
      Revisit(EU);
    }
    PHI->ReplaceWith(DeadNode, Use::K_EFFECT);
  } else {
    Revisit(PHI);
  }
  return NoChange();
}

GraphReduction ValuePromotion::Reduce(Node* N) {
  switch(N->getOp()) {
  case IrOpcode::SrcAssignStmt:
    return ReduceAssignment(N);
  case IrOpcode::SrcVarAccess:
    return ReduceVarAccess(N);
  case IrOpcode::SrcArrayDecl:
    return ReduceArrayDecl(N);
  case IrOpcode::SrcArrayAccess:
    return ReduceMemAccess(N);
  case IrOpcode::Phi:
    return ReducePhiNode(N);
  default:
    return NoChange();
  }
}
