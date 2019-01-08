#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "Parser.h"

using namespace gross;

bool Parser::Parse() {
  auto Tok = NextTok();
  for(; Tok != Lexer::TOK_EOF; Tok = NextTok()) {
    switch(Tok) {
    case Lexer::TOK_VAR:
      if(!ParseVarDecl())
        return false;
      else
        break;
    case Lexer::TOK_ARRAY:
      if(!ParseArrayDecl())
        return false;
      else
        break;
    case Lexer::TOK_FUNCTION:
    case Lexer::TOK_PROCEDURE:
      if(!ParseFuncDecl())
        return false;
      else
        break;
    default:
      gross_unreachable("Unimplemented");
    }
  }
  return true;
}

bool Parser::ParseVarDecl() {
  Lexer::Token Tok = NextTok();
  if(Tok == Lexer::TOK_IDENT) {
    auto SymName = TokBuffer();
    SymbolLookup Lookup(*this, SymName);
    if(Lookup.InCurrentScope())
      return false;
    NodeBuilder<IrOpcode::SrcVarDecl> VDB(&G);
    auto* VarDeclNode = VDB.SetSymbolName(SymName)
                           .Build();
    CurSymTable().insert({SymName, VarDeclNode});

    Tok = NextTok();
    for(; Tok == Lexer::TOK_COMMA; Tok = NextTok()) {
      Tok = NextTok();
      if(Tok != Lexer::TOK_IDENT) {
        Log::E() << "Expecting identifier after comma\n";
        return false;
      }
      SymName = TokBuffer();
      SymbolLookup Lookup(*this, SymName);
      if(Lookup.InCurrentScope())
        return false;
      auto* VarDeclNode = VDB.SetSymbolName(SymName)
                             .Build();
      CurSymTable().insert({SymName, VarDeclNode});
    }
    if(Tok != Lexer::TOK_SEMI_COLON){
      Log::E() << "Expecting ';' "
               << "at the end of var declaration\n";
      return false;
    }
  }else {
    Log::E() << "Expecting identifier\n";
    return false;
  }

  return true;
}

bool Parser::ParseArrayDecl() {
  return true;
}

bool Parser::ParseFuncDecl() {
  Lexer::Token Tok = NextTok();
  if(Tok != Lexer::TOK_IDENT) {
    Log::E() << "Expect identifier for function\n";
    return false;
  }
  const auto& FuncName = TokBuffer();
  // TODO: Create start node and register function
  Tok = NextTok();
  if(Tok == Lexer::TOK_PARAN) {
    // TODO: Argument list
    Tok = NextTok();
  }
  if(Tok != Lexer::TOK_SEMI_COLON) {
    Log::E() << "Expect ';' at the end\n";
    return false;
  }
  // TODO: Function body
  Tok = NextTok();
  if(Tok != Lexer::TOK_SEMI_COLON) {
    Log::E() << "Expect ';' at the end\n";
    return false;
  }
  return true;
}
