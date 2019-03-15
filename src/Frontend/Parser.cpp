#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "Parser.h"

using namespace gross;

/// Parse computation
bool Parser::Parse(bool StepLexer) {
  if(StepLexer)
    (void) NextTok();

  auto Tok = CurTok();
  if(!(Tok == Lexer::TOK_IDENT &&
       TokBuffer() == "main")) {
    Log::E() << "Expecting 'main' here\n";
    return false;
  }
  Tok = NextTok();

  // we basically treat main as special function
  NodeBuilder<IrOpcode::VirtFuncPrototype> FB(&G);
  FB.FuncName("main");

  // global variable scope
  NewSymScope();
  auto* FuncNode = FB.Build();
  CurSymTable().insert({"main", FuncNode});

  std::vector<Node*> GlobalVars;
  if(!ParseVarDeclTop(&GlobalVars)) return false;
  for(auto* GV : GlobalVars) {
    if(GV->getOp() == IrOpcode::SrcVarDecl) {
      // transform into array decl for the sake of
      // lowering to memory load/store later
      NodeProperties<IrOpcode::VirtSrcDecl> DNP(GV);
      auto* NewDecl = NodeBuilder<IrOpcode::SrcArrayDecl>(&G)
                      .SetSymbolName(DNP.ident_name(G))
                      .AddConstDim(1).Build();
      CurSymTable()[DNP.ident_name(G)] = NewDecl;
      GV->ReplaceWith(NewDecl);
      G.MarkGlobalVar(NewDecl);
    } else {
      G.MarkGlobalVar(GV);
    }
  }
  for(Tok = CurTok();
      Tok == Lexer::TOK_FUNCTION || Tok == Lexer::TOK_PROCEDURE;
      Tok = CurTok()) {
    if(!ParseFuncDecl()) return false;
  }

  Tok = CurTok();
  if(Tok != Lexer::TOK_L_CUR_BRACKET) {
    Log::E() << "Expecting '{' here\n";
    return false;
  }
  NewLastControlPoint();
  setLastCtrlPoint(FuncNode);
  (void) NextTok();
  std::vector<Node*> FuncBodyStmts;
  if(!ParseStatements(FuncBodyStmts)) return false;
  NodeBuilder<IrOpcode::End> EB(&G, FuncNode);
  EB.AddTerminator(getLastCtrlPoint());
  // must 'wait' after all side effects terminate
  // if there is no consumers
  auto MAE = LastMemAccess.end();
  for(auto& P : LastModified) {
    auto* LastMod = P.second;
    // for non-memory access, if there is no consumers,
    // well, then it's dead code
    if(LastMemAccess.find(LastMod) == MAE)
      EB.AddEffectDep(LastMod);
  }
  auto* EndNode = EB.Build();

  Tok = CurTok();
  if(Tok != Lexer::TOK_R_CUR_BRACKET) {
    Log::E() << "Expecting '}' here\n";
    return false;
  }
  Tok = NextTok();
  if(Tok != Lexer::TOK_DOT) {
    Log::E() << "Expect '.' at the end\n";
    return false;
  }
  (void) NextTok();
  PopSymScope();

  // Add function sub-graph
  SubGraph SG(EndNode);
  G.AddSubRegion(SG);
  (void) NodeBuilder<IrOpcode::FunctionStub>(&G, SG).Build();

  return true;
}

