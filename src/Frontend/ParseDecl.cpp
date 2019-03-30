#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Graph/NodeMarker.h"
#include "Parser.h"

using namespace gross;

template<IrOpcode::ID OC>
bool Parser::ParseTypeDecl(NodeBuilder<OC>& NB) {
  gross_unreachable("Unimplemented");
  return false;
}
namespace gross {
template<>
bool Parser::ParseTypeDecl(NodeBuilder<IrOpcode::SrcVarDecl>& NB) {
  (void) NextTok();
  return true;
}
template<>
bool Parser::ParseTypeDecl(NodeBuilder<IrOpcode::SrcArrayDecl>& NB) {
  Lexer::Token Tok;
  while(true) {
    Tok = NextTok();
    if(Tok != Lexer::TOK_L_BRACKET) break;
    (void) NextTok();
    Node* ExprNode = ParseExpr();
    NB.AddDim(ExprNode);
    Tok = CurTok();
    if(Tok != Lexer::TOK_R_BRACKET) {
      Log::E() << "Expecting close bracket\n";
      return false;
    }
  }
  return true;
}
} // end namespace gross

template<IrOpcode::ID OC>
bool Parser::ParseVarDecl(std::vector<Node*>* Results) {
  NodeBuilder<OC> NB(&G);
  if(!ParseTypeDecl(NB)) return false;

  Lexer::Token Tok = CurTok();
  std::string SymName;
  while(true) {
    if(Tok != Lexer::TOK_IDENT) {
      Log::E() << "Expecting identifier\n";
      return false;
    }
    SymName = TokBuffer();
    SymbolLookup Lookup(*this, SymName);
    if(Lookup.InCurrentScope()) {
      Log::E() << "variable already declared in this scope\n";
      return false;
    }
    auto* VarDeclNode = NB.SetSymbolName(SymName)
                          .Build();
    CurSymTable().insert({SymName, VarDeclNode});
    if(Results) Results->push_back(VarDeclNode);

    Tok = NextTok();
    if(Tok == Lexer::TOK_SEMI_COLON)
      break;
    else if(Tok != Lexer::TOK_COMMA) {
      Log::E() << "Expecting semicolomn or comma\n";
      return false;
    }
    Tok = NextTok();
  }

  assert(Tok == Lexer::TOK_SEMI_COLON &&
         "Expecting semi-colon");
  (void) NextTok();
  return true;
}

bool Parser::ParseVarDeclTop(std::vector<Node*>* Results) {
  while(true) {
    auto Tok = CurTok();
    if(Tok == Lexer::TOK_VAR) {
      if(!ParseVarDecl<IrOpcode::SrcVarDecl>(Results))
        return false;
    } else if(Tok == Lexer::TOK_ARRAY) {
      if(!ParseVarDecl<IrOpcode::SrcArrayDecl>(Results))
        return false;
    } else {
      break;
    }
  }
  return true;
}

/// placeholder function to avoid link time error
/// for un-specialized decl template functions
void Parser::__SupportedParseVarDecls() {
  (void) ParseVarDecl<IrOpcode::SrcVarDecl>();
  (void) ParseVarDecl<IrOpcode::SrcArrayDecl>();
}

Node* Parser::ParseFuncDecl() {
  auto Tok = CurTok();
  if(Tok != Lexer::TOK_FUNCTION &&
     Tok != Lexer::TOK_PROCEDURE) {
    Log::E() << "Expecting 'function' or 'procedure' here\n";
    return nullptr;
  }
  Tok = NextTok();

  if(Tok != Lexer::TOK_IDENT) {
    Log::E() << "Expect identifier for function\n";
    return nullptr;
  }
  auto FName = TokBuffer();
  NodeBuilder<IrOpcode::VirtFuncPrototype> FB(&G);
  FB.FuncName(FName);

  // for the entire function
  NodeMarker<uint16_t> NodeIdxMarker(G, 10000);
  SetNodeIdxMarker(&NodeIdxMarker);

  SymbolLookup FuncLookup(*this, FName);
  // 'CurrentScope' is global scope
  if(FuncLookup.InCurrentScope()) {
    Log::E() << "function has already declared in this scope\n";
    return nullptr;
  }
  // start of function scope
  NewSymScope();
  // start a new control point record
  NewLastControlPoint();
  NewLastModified();
  NewLastMemAccess();

  Tok = NextTok();
  if(Tok == Lexer::TOK_L_PARAN) {
    // Argument list
    Tok = NextTok();
    while(Tok != Lexer::TOK_R_PARAN) {
      if(Tok != Lexer::TOK_IDENT) {
        Log::E() << "Expecting identifier in an argument list\n";
        return nullptr;
      }
      const auto& ArgName = TokBuffer();
      SymbolLookup ArgLookup(*this, ArgName);
      if(ArgLookup.InCurrentScope()) {
        Log::E() << "argument name has already been taken in this scope\n";
        return nullptr;
      }
      auto* ArgNode = NodeBuilder<IrOpcode::Argument>(&G, ArgName)
                      .Build();
      FB.AddParameter(ArgNode);
      CurSymTable().insert({ArgName, ArgNode});

      Tok = NextTok();
      if(Tok == Lexer::TOK_COMMA)
        Tok = NextTok();
    }
    Tok = NextTok();
  }

  if(Tok != Lexer::TOK_SEMI_COLON) {
    Log::E() << "Expect ';' at the end of function header\n";
    return nullptr;
  }
  auto* FuncNode = FB.Build();
  // wind back to previous(global) scope and insert
  // function symbol
  auto ItPrevScope = CurSymTableIt();
  MoveToPrevSymTable(ItPrevScope);
  ItPrevScope->insert({FName, FuncNode});
  setLastCtrlPoint(FuncNode);

  (void) NextTok();
  if(!ParseVarDeclTop()) return nullptr;
  Tok = CurTok();
  if(Tok != Lexer::TOK_L_CUR_BRACKET) {
    Log::E() << "Expecting '{' here\n";
    return nullptr;
  }
  (void) NextTok();
  std::vector<Node*> FuncBodyStmts;
  if(!ParseStatements(FuncBodyStmts)) return nullptr;

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
    return nullptr;
  }
  Tok = NextTok();
  if(Tok != Lexer::TOK_SEMI_COLON) {
    Log::E() << "Expect ';' at the end of function decl\n";
    return nullptr;
  }
  (void) NextTok();
  PopSymScope();

  // Add function sub-graph
  SubGraph SG(EndNode);
  G.AddSubRegion(SG);
  (void) NodeBuilder<IrOpcode::FunctionStub>(&G, SG).Build();

  ClearNodeIdxMarker();

  return EndNode;
}
