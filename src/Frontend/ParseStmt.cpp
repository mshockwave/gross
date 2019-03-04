#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "Parser.h"
#include <unordered_set>
#include <unordered_map>

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
  // TODO: function call
  if(!DesigNode->getNumEffectInput()) {
    AssignNode->appendControlInput(getLastCtrlPoint());
  } else {
    // see if this node is control depending on LastControlPoint
    if(FindNearestCtrlPoint(AssignNode) != getLastCtrlPoint())
      AssignNode->appendControlInput(getLastCtrlPoint());
  }

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
  using affine_ctrl_points = decltype(LastControlPoint);
  using ctrl_point_type = typename affine_ctrl_points::value_type;
  auto CtrlMergeCallback = [&](affine_ctrl_points& ACP,
                               const std::vector<ctrl_point_type*>& BrPoints) {
    assert(BrPoints.size() <= 2 && BrPoints.size());
    auto* MergePoint = ACP.CurEntryMutable();
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

  using affine_table_type = decltype(LastModified);
  using table_type = typename affine_table_type::TableTy;
  auto PHINodeCallback = [&,this](affine_table_type& JoinTable,
                                  const std::vector<table_type*>& BrTables) {
    std::unordered_map<Node*, std::vector<Node*>> Variants;
    for(auto& Pair : JoinTable) {
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
      // only take the last two
      const auto VSize = Variant.size();
      Node *N1 = Variant.at(VSize - 2),
           *N2 = Variant.at(VSize - 1);
      if(N1 == N2) continue;
      NodeBuilder<IrOpcode::Phi> PB(&G);
      PB.SetCtrlMerge(MergeNode);
      PB.AddEffectInput(N1)
        .AddEffectInput(N2);
      auto* PHI = PB.Build();
      // update previous table
      JoinTable[VP.first] = PHI;
    }
  };
  LastModified.CloseAffineScope<>(PHINodeCallback);
  return MergeNode;
}

Node* Parser::ParseWhileStmt() {
  auto Tok = CurTok();
  if(Tok != Lexer::TOK_WHILE) {
    Log::E() << "Expecting 'while' here\n";
    return nullptr;
  }
  (void) NextTok();
  auto* RelNode = ParseRelation();
  if(!RelNode) return nullptr;

  NodeBuilder<IrOpcode::Loop> LB(&G, getLastCtrlPoint());
  LB.Condition(RelNode);
  auto* LoopNode = LB.Build();
  auto* LoopBranch = NodeProperties<IrOpcode::Loop>(LoopNode)
                     .Branch();

  Tok = CurTok();
  if(Tok != Lexer::TOK_DO) {
    Log::E() << "Expecting 'do' here\n";
    return nullptr;
  }
  (void) NextTok();

  NewSymScope();
  LastModified.NewAffineScope();
  auto* LoopTrueBr = NodeProperties<IrOpcode::If>(LoopBranch)
                     .TrueBranch();
  setLastCtrlPoint(LoopTrueBr);
  std::vector<Node*> BodyStmts;
  if(!ParseStatements(BodyStmts)) return nullptr;
  PopSymScope();
  if(getLastCtrlPoint() != LoopTrueBr) {
    // update backedge
    LoopNode->ReplaceUseOfWith(LoopTrueBr, getLastCtrlPoint(),
                               Use::K_CONTROL);
  }

  Tok = CurTok();
  if(Tok != Lexer::TOK_END_DO) {
    Log::E() << "Expecting 'od' here\n";
    return nullptr;
  }
  (void) NextTok();

  // {Original Value, PhiNode}
  std::unordered_map<Node*,Node*> FixupMap;
  using affine_table_type = decltype(LastModified);
  using table_type = typename affine_table_type::TableTy;
  auto PHINodeCallback = [&,this](affine_table_type& JoinTable,
                                  const std::vector<table_type*>& BrTables) {
    assert(BrTables.size() == 1);
    auto& LoopBack = *BrTables.front(); // loopback values
    auto& InitVals = *(JoinTable.CurEntry());
    for(auto& P : LoopBack) {
      auto& Decl = P.first;
      if(!InitVals.count(Decl)) continue;
      // ignore duplicate values
      if(InitVals[Decl] == P.second) continue;
      auto* PHI = NodeBuilder<IrOpcode::Phi>(&G)
                  .SetCtrlMerge(LoopNode)
                  // put original value on the first
                  // in order to retreive it more easily
                  // later!
                  .AddEffectInput(InitVals[Decl])
                  .AddEffectInput(P.second)
                  .Build();
      FixupMap.insert({InitVals[Decl], PHI});
      JoinTable[Decl] = PHI;
    }
  };
  LastModified.CloseAffineScope<>(PHINodeCallback);

  // fixup statements in the body
  std::vector<Node*> Worklist;
  for(auto& P : FixupMap) {
    auto* OrigVal = P.first;
    auto* PHI = P.second;
    Worklist.clear();
    for(auto* EU : OrigVal->effect_users()) {
      if(EU == PHI) continue;
      Worklist.push_back(EU);
    }
    for(auto* N : Worklist)
      N->ReplaceUseOfWith(OrigVal, PHI, Use::K_EFFECT);
  }

  setLastCtrlPoint(NodeProperties<IrOpcode::If>(LoopBranch)
                   .FalseBranch());

  return LoopNode;
}

Node* Parser::ParseReturnStmt() {
  auto Tok = CurTok();
  if(Tok != Lexer::TOK_RETURN) {
    Log::E() << "Expecting 'return' here\n";
    return nullptr;
  }
  (void) NextTok();
  auto* ExprNode = ParseExpr();
  // ExprNode can be null
  auto* RetNode = NodeBuilder<IrOpcode::Return>(&G, ExprNode)
                  .Build();
  RetNode->appendControlInput(getLastCtrlPoint());
  setLastCtrlPoint(RetNode);
  return RetNode;
}

bool Parser::ParseStatements(std::vector<Node*>& Stmts) {
  while(true) {
    auto Tok = CurTok();
    // TODO: funcCall
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
    case Lexer::TOK_RETURN: {
      auto* Stmt = ParseReturnStmt();
      if(Stmt) Stmts.push_back(Stmt);
      else return false;
      break;
    }
    case Lexer::TOK_WHILE: {
      auto* Stmt = ParseWhileStmt();
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
