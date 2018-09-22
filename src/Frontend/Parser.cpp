#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "Parser.h"

using namespace gross;

void Parser::Parse() {
  auto Tok = NextTok();
  for(; Tok != Lexer::TOK_EOF; Tok = NextTok()) {
    switch(Tok) {
    case Lexer::TOK_VAR: {
      ParseVarDecl();
      break;
    }
    case Lexer::TOK_ARRAY:
      ParseArrayDecl();
      break;
    case Lexer::TOK_FUNCTION:
    case Lexer::TOK_PROCEDURE:
      ParseFuncDecl();
      break;
    default:
      gross_unreachable("Unimplemented");
    }
  }
}

void Parser::ParseVarDecl() {
  auto Tok = NextTok();
  if(Tok == Lexer::TOK_IDENT) {
    do {

      Tok = NextTok();
    } while(Tok == Lexer::TOK_COMMA);
    if(Tok != Lexer::TOK_SEMI_COLON){
      // TODO: Error
    }
  }else {
    // TODO: Error
  }
}

void Parser::ParseArrayDecl() {
}

void Parser::ParseFuncDecl() {
}
