#include "gross/Graph/Reductions/DeadCodeElimination.h"
#include "gross/Graph/Node.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Support/STLExtras.h"

using namespace gross;

void DCEReducer::EliminateUnreachable(Graph& G) {
  // TODO: come up with a new version
}

static inline
void removeDeadControl(typename Graph::const_node_iterator& NI,
                       Graph& G, Node* DeadNode) {
  auto* N = const_cast<Node*>(Graph::GetNodeFromIt(NI));
  NodeProperties<IrOpcode::If> BrNP(N);
  if(BrNP) {
    auto* Predicate = BrNP.Condition();
    if(Predicate && Predicate->getOp() == IrOpcode::ConstantInt) {
      bool IsTrue = NodeProperties<IrOpcode::ConstantInt>(Predicate)
                    .as<int32_t>(G) != 0;
      assert(N->getNumControlInput() > 0);
      auto* PrevCtrlPoint = N->getControlInput(0);
      auto* TrueBr = BrNP.TrueBranch();
      auto MI
        = gross::find_if(TrueBr->control_users(),
                         [](Node* N) -> bool {
                           return N->getOp() == IrOpcode::Merge;
                         });
      assert(MI != TrueBr->control_users().end());
      auto* MergeNode = *MI;
      if(IsTrue) {
        // only preserve true branch
        TrueBr->ReplaceWith(PrevCtrlPoint, Use::K_CONTROL);
        TrueBr->Kill(DeadNode);
      } else {
        // only preserve false branch, or preserve nothing if
        // there is no false branch
        auto* FalseBr = BrNP.FalseBranch();
        if(FalseBr) {
          FalseBr->ReplaceWith(PrevCtrlPoint, Use::K_CONTROL);
          FalseBr->Kill(DeadNode);
        }
      }
      // remove If node
      N->Kill(DeadNode);
      // handle merge node
      std::vector<Node*> MergeCtrlUsrs(
        MergeNode->control_users().begin(),
        MergeNode->control_users().end()
      );
      for(auto* CU : MergeCtrlUsrs) {
        if(CU->getOp() != IrOpcode::Phi) continue;
        if(CU->getNumValueInput() == 2) {
          // we've go through value promotion
          auto* Taken = IsTrue? CU->getValueInput(0) :
                                CU->getValueInput(1);
          CU->ReplaceWith(Taken, Use::K_VALUE);
          CU->Kill(DeadNode);
        } else if(CU->getNumEffectInput() == 2) {
          // we haven't go through value promotion
          auto* Taken = IsTrue? CU->getEffectInput(0) :
                                CU->getEffectInput(1);
          CU->ReplaceWith(Taken, Use::K_EFFECT);
          CU->Kill(DeadNode);
        } else {
          gross_unreachable("Incorrect num of PHI inputs");
        }
      }
      MergeNode->ReplaceWith(PrevCtrlPoint, Use::K_CONTROL);
      MergeNode->Kill(DeadNode);
    }
  }
  ++NI;
}
void DCEReducer::DeadControlElimination(Graph& G) {
  for(auto I = G.node_cbegin(); I != G.node_cend();) {
    removeDeadControl(I, G, DeadNode);
  }
}

void DCEReducer::Reduce(Graph& G) {
  DeadNode = NodeBuilder<IrOpcode::Dead>(&G).Build();

  // FIXME: perform bad in nested structure
  //DeadControlElimination(G);

  EliminateUnreachable(G);
}
