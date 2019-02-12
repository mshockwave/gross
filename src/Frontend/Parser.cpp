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

  if(!ParseVarDeclTop()) return false;
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
  for(auto& P : LastModified) {
    EB.AddEffectDep(P.second);
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

