#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "Parser.h"
#include <unordered_set>

using namespace gross;

Node* Parser::ParseAssignment() {
  auto Tok = CurTok();
  if(Tok != Lexer::TOK_LET) {
    Log::E() << "Expecting 'let' keyword here\n";
    return nullptr;
  }
  (void) NextTok();

  auto* DesigNode = ParseDesignator();
  NodeProperties<IrOpcode::VirtSrcDesigAccess> DNP(DesigNode);
  if(!DNP) return nullptr;
  Tok = CurTok();

  if(Tok != Lexer::TOK_L_ARROW) {
    Log::E() << "Expecting '<-' here\n";
    return nullptr;
  }
  (void) NextTok();

  auto* ExprNode = ParseExpr();
  if(!ExprNode) return nullptr;

  auto* AssignNode = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                     .Dest(DesigNode).Src(ExprNode)
                     .Build();
  assert(AssignNode && "fail to build SrcAssignStmt node");
  // no effect dependency, then depend on the last control point
  // TODO: 1. consider effect dependecy beyond the last control point
  // 2. function call
  if(!DesigNode->getNumEffectInput())
    AssignNode->appendControlInput(getLastCtrlPoint());

  // update last modified map
  LastModified[DNP.decl()] = AssignNode;
  return AssignNode;
}

Node* Parser::ParseIfStmt() {
  auto Tok = CurTok();
  if(Tok != Lexer::TOK_IF) {
    Log::E() << "expecting 'if' keyword here\n";
    return nullptr;
  }
  (void) NextTok();
  auto* RelExpr = ParseRelation();
  if(!RelExpr) return nullptr;

  auto* IfNode = NodeBuilder<IrOpcode::If>(&G)
                 .Condition(RelExpr)
                 .Build();
  IfNode->appendControlInput(getLastCtrlPoint());
  setLastCtrlPoint(IfNode);

  Tok = CurTok();
  if(Tok != Lexer::TOK_THEN) {
    Log::E() << "expecting 'then' keyword here\n";
    return nullptr;
  }
  (void) NextTok();

  NewSymScope();
  LastModified.NewAffineScope();
  LastControlPoint.NewAffineScope();
  auto* TrueBranch = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                     .IfStmt(IfNode)
                     .Build();
  setLastCtrlPoint(TrueBranch);
  std::vector<Node*> Stmts;
  if(!ParseStatements(Stmts)) return nullptr;
  PopSymScope();
  //BoundBranchControl(TrueBranch, Stmts);

  Tok = CurTok();
  Node* FalseBranch = nullptr;
  if(Tok == Lexer::TOK_ELSE) {
    (void) NextTok();
    NewSymScope();
    LastModified.NewBranch();
    LastControlPoint.NewBranch();
    FalseBranch = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                  .IfStmt(IfNode)
                  .Build();
    setLastCtrlPoint(FalseBranch);
    std::vector<Node*> ElseStmts;
    if(!ParseStatements(ElseStmts)) return nullptr;
    PopSymScope();
    //BoundBranchControl(FalseBranch, ElseStmts);
    Tok = CurTok();
  }

  // add merge node to merge control deps
  using ctrl_point_type = typename decltype(LastControlPoint)::value_type;
  auto CtrlMergeCallback = [&](ctrl_point_type* MergePoint,
                               const std::vector<ctrl_point_type*>& BrPoints) {
    assert(BrPoints.size() <= 2 && BrPoints.size());
    NodeBuilder<IrOpcode::Merge> MB(&G);
    MB.AddCtrlInput(std::get<0>(*BrPoints.at(0)));
    // check whether there is false branch
    if(BrPoints.size() == 1)
      // no false branch, merge from IfStmt
      MB.AddCtrlInput(std::get<0>(*MergePoint));
    else
      MB.AddCtrlInput(std::get<0>(*BrPoints.at(1)));

    (*MergePoint)[0] = MB.Build();
  };
  LastControlPoint.CloseAffineScope<>(CtrlMergeCallback);
  auto* MergeNode = getLastCtrlPoint();
  assert(NodeProperties<IrOpcode::Merge>(MergeNode));

  if(Tok != Lexer::TOK_END_IF) {
    Log::E() << "expecting 'fi' keyword here\n";
    return nullptr;
  }
  (void) NextTok();

  using table_type = typename decltype(LastModified)::TableTy;
  auto PHINodeCallback = [&,this](table_type* JoinTable,
                                  const std::vector<table_type*>& BrTables) {
    std::unordered_map<Node*, std::vector<Node*>> Variants;
    for(auto& Pair : *JoinTable) {
      Variants[Pair.first].push_back(Pair.second);
    }
    for(auto* BrTable : BrTables)
      for(auto& Pair : *BrTable)
        Variants[Pair.first].push_back(Pair.second);

    // create PHI nodes for those whose list has more than one element
    for(auto& VP : Variants) {
      auto& Variant = VP.second;
      // Note: assume there won't be cases where a var is uninitialized
      // before IfStmt but only assigned in one of the branches
      if(Variant.size() < 2) continue;
      NodeBuilder<IrOpcode::Phi> PB(&G);
      PB.SetCtrlMerge(MergeNode);
      // only take the last two
      const auto VSize = Variant.size();
      PB.AddEffectInput(Variant.at(VSize - 2));
      PB.AddEffectInput(Variant.at(VSize - 1));
      auto* PHI = PB.Build();
      // update previous table
      (*JoinTable)[VP.first] = PHI;
    }
  };
  LastModified.CloseAffineScope<>(PHINodeCallback);
  return MergeNode;
}

bool Parser::ParseStatements(std::vector<Node*>& Stmts) {
  while(true) {
    auto Tok = CurTok();
    // TODO: funcCall, while, return
    switch(Tok) {
    case Lexer::TOK_IF: {
      auto* Stmt = ParseIfStmt();
      if(Stmt) Stmts.push_back(Stmt);
      else return false;
      break;
    }
    case Lexer::TOK_LET: {
      auto* Stmt = ParseAssignment();
      if(Stmt) Stmts.push_back(Stmt);
      else return false;
      break;
    }
    default:
      break;
    }
    Tok = CurTok();
    if(Tok != Lexer::TOK_SEMI_COLON) break;
    (void) NextTok();
  }
  return true;
}
