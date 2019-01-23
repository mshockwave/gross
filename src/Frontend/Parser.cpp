#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "Parser.h"
#include <cstdlib>

using namespace gross;

/// Note: For each ParseXXX, it would expect the lexer cursor
/// starting on the first token of its semantic rule.
bool Parser::Parse() {
  auto Tok = NextTok();
  while(Tok != Lexer::TOK_EOF) {
    switch(Tok) {
    case Lexer::TOK_VAR:
      if(!ParseVarDecl<IrOpcode::SrcVarDecl>())
        return false;
      else
        break;
    case Lexer::TOK_ARRAY:
      if(!ParseVarDecl<IrOpcode::SrcArrayDecl>())
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

