#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Graph/NodeMarker.h"
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
  if(DesigNode->getOp() == IrOpcode::SrcArrayAccess &&
     DesigNode->getNumEffectInput() == 1) {
    // remove previous record in LastMemAccess
    auto* PrevStore = DesigNode->getEffectInput(0);
    LastMemAccess[PrevStore].erase(DesigNode);
  }

  Tok = CurTok();
  if(Tok != Lexer::TOK_L_ARROW) {
    Log::E() << "Expecting '<-' here\n";
    return nullptr;
  }
  (void) NextTok();

  auto* ExprNode = ParseExpr();
  if(!ExprNode) return nullptr;

  if(DesigNode->getOp() == IrOpcode::SrcArrayAccess &&
     DesigNode->getNumEffectInput() == 1) {
    // append memory read dependency
    auto* OldPrevStore = DesigNode->getEffectInput(0);
    // since Decl might be modified in ParseExpr (by function call)
    // we need to retreive the LastModified of Decl again
    assert(LastModified.count(DNP.decl()));
    auto* NewPrevStore = LastModified.at(DNP.decl());
    auto& MemReads = LastMemAccess[NewPrevStore];
    for(auto* MemRead : MemReads) {
      DesigNode->appendEffectInput(MemRead);
    }
    if(!MemReads.empty()) {
      DesigNode->removeEffectInputAll(OldPrevStore);
    } else if(OldPrevStore != NewPrevStore) {
      DesigNode->ReplaceUseOfWith(OldPrevStore, NewPrevStore, Use::K_EFFECT);
    }
  }

  auto* AssignNode = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                     .Dest(DesigNode).Src(ExprNode)
                     .Build();
  assert(AssignNode && "fail to build SrcAssignStmt node");

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
  LastMemAccess.NewAffineScope();
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
    LastMemAccess.NewBranch();
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

  using ma_affine_table_type = decltype(LastMemAccess);
  using ma_table_type = typename ma_affine_table_type::TableTy;
  auto MemAccessMergeCallback
    = [&,this](ma_affine_table_type& JoinTable,
               const std::vector<ma_table_type*> &BrTables) {
    auto& InitEffects = *(JoinTable.CurEntry());
    for(auto* BrTable : BrTables) {
      for(auto& P : *BrTable) {
        auto& Store = P.first;
        //if(!InitEffects.count(Store)) continue;
        JoinTable[Store].insert(P.second.begin(),
                                P.second.end());
      }
    }
  };
  LastMemAccess.CloseAffineScope<>(MemAccessMergeCallback);

  using affine_table_type = decltype(LastModified);
  using table_type = typename affine_table_type::TableTy;
  auto PHINodeCallback = [&,this](affine_table_type& JoinTable,
                                  const std::vector<table_type*>& BrTables) {
    std::unordered_map<Node*, std::vector<Node*>> Variants;
    std::unordered_map<Node*, Node*> InitVals;
    for(auto& Pair : JoinTable) {
      InitVals[Pair.first] = Pair.second;
    }
    const size_t NumBranches = BrTables.size() >= 2? BrTables.size() : 2;

    size_t Idx = 0U;
    for(auto* BrTable : BrTables) {
      for(auto& Pair : *BrTable) {
        auto* Decl = Pair.first;
        if(!Variants.count(Decl)) {
          // right before first write, initialize with
          // amount of slots matches NumBranches
          if(!InitVals.count(Decl)) {
            InitVals[Decl] = getInitialValue(Decl);
          }
          auto* InitVal = InitVals.at(Decl);
          Variants[Decl].assign(NumBranches, InitVal);
        }
        assert(Variants[Decl].size() == NumBranches);
        Variants[Decl][Idx] = Pair.second;
      }
      Idx++;
    }

    // create PHI nodes for those whose list has more than one element
    for(auto& VP : Variants) {
      auto& Variant = VP.second;
      // only take the last two
      const auto VSize = Variant.size();
      assert(VSize >= 2);
      Node *N1 = Variant.at(VSize - 2),
           *N2 = Variant.at(VSize - 1);
      if(N1 == N2) continue;
      assert(N1 && N2);
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
  auto PreBodyNodeIdx = GetCurrentNodeIdx();

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
  LastMemAccess.NewAffineScope();
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

  using ma_affine_table_type = decltype(LastMemAccess);
  using ma_table_type = typename ma_affine_table_type::TableTy;
  auto MemAccessMergeCallback
    = [&,this](ma_affine_table_type& JoinTable,
               const std::vector<ma_table_type*> &BrTables) {
    assert(BrTables.size() == 1);
    auto& LoopBack = *BrTables.front(); // loopback values
    auto& InitEffects = *(JoinTable.CurEntry());
    for(auto& P : LoopBack) {
      auto& Store = P.first;
      //if(!InitEffects.count(Store)) continue;
      JoinTable[Store].insert(P.second.begin(),
                              P.second.end());
    }
  };
  LastMemAccess.CloseAffineScope<>(MemAccessMergeCallback);

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
      if(!InitVals.count(Decl)) {
        // use initial value instead
        InitVals[Decl] = getInitialValue(Decl);
      }
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

  auto LastBodyNodeIdx = GetCurrentNodeIdx();

  // fixup statements in the body
  std::vector<Node*> Worklist;
  for(auto& P : FixupMap) {
    auto* OrigVal = P.first;
    auto* PHI = P.second;
    Worklist.clear();
    for(auto* EU : OrigVal->effect_users()) {
      if(EU != PHI &&
         (GetNodeIdx(EU) > PreBodyNodeIdx &&
          GetNodeIdx(EU) <= LastBodyNodeIdx)) {
        Worklist.push_back(EU);
      }
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

Node* Parser::ParseFuncCall() {
  auto Tok = NextTok();
  if(Tok != Lexer::TOK_IDENT) {
    Log::E() << "Expecting a function identifier\n";
    return nullptr;
  }

  auto IdentName = TokBuffer();
  SymbolLookup Lookup(*this, IdentName);
  Node* Func = (*Lookup).first;
  if(!Func || Func->getOp() != IrOpcode::Start) {
    Log::E() << "Symbol \'" << IdentName << "\' not declared "
             << "or not function\n";
    return nullptr;
  }
  auto* FuncStub = NodeProperties<IrOpcode::Start>(Func).FuncStub(G);
  assert(FuncStub && "can not find associated FuncStub node");
  NodeBuilder<IrOpcode::Call> CallBuilder(&G, FuncStub);

  Tok = NextTok();
  if(Tok == Lexer::TOK_L_PARAN) {
    // has parameters
    Tok = NextTok();
    while(Tok != Lexer::TOK_R_PARAN) {
      auto* Param = ParseExpr();
      if(!Param) return nullptr;
      CallBuilder.AddParam(Param);
      Tok = CurTok();
      if(Tok == Lexer::TOK_COMMA)
        Tok = NextTok();
    }
    Tok = NextTok();
  }
  if(CallBuilder.arg_size() != Func->getNumEffectInput()) {
    Log::E() << "Mismatched amount of actual parameters\n";
    return nullptr;
  }

  auto* CallNode = CallBuilder.Build();
  NodeProperties<IrOpcode::FunctionStub> StubNP(FuncStub);
  if(StubNP.hasAttribute<Attr::HasSideEffect>(G, Func)) {
    CallNode->appendControlInput(getLastCtrlPoint());
    setLastCtrlPoint(CallNode);
  }
  if(StubNP.hasAttribute<Attr::NoMem>(G, Func)) {
    return CallNode;
  }
  if(StubNP.hasAttribute<Attr::WriteMem>(G, Func)) {
    // clobber all the global memory
    for(auto* GVDecl : G.global_vars()) {
      if(LastModified.count(GVDecl)) {
        auto* PrevStore = LastModified.at(GVDecl);
        auto& MemReads = LastMemAccess[PrevStore];
        if(!MemReads.empty()) {
          for(auto* MemRead : MemReads) {
            CallNode->appendEffectInput(MemRead);
          }
        } else {
          CallNode->appendEffectInput(PrevStore);
        }
      }
      LastModified[GVDecl] = CallNode;
    }
  }
  if(StubNP.hasAttribute<Attr::ReadMem>(G, Func)) {
    // affect all the global memory
    for(auto* GVDecl : G.global_vars()) {
      if(LastModified.count(GVDecl)) {
        auto* PrevStore = LastModified.at(GVDecl);
        if(!StubNP.hasAttribute<Attr::WriteMem>(G, Func)) {
          // effect depend only if this call only reads memory
          CallNode->appendEffectInput(PrevStore);
        }
        LastMemAccess[PrevStore].insert(CallNode);
      }
    }
  }

  return CallNode;
}

bool Parser::ParseStatements(std::vector<Node*>& Stmts) {
  while(true) {
    auto Tok = CurTok();
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
    case Lexer::TOK_CALL: {
      auto* Stmt = ParseFuncCall();
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
