#ifndef GROSS_FRONTEND_PARSER_H
#define GROSS_FRONTEND_PARSER_H
#include "Lexer.h"
#include <stack>
#include <unordered_map>

namespace gross {
// Forward declaration
class Graph;

class Parser {
  Graph& G;
  Lexer Lex;

  inline
  Lexer::Token NextTok() {
    return Lex.getNextToken();
  }
  Lexer::Token CurTok() {
    return Lex.getToken();
  }

  inline
  const std::string& TokBuffer() {
    return Lex.getBuffer();
  }

  using SymEntriesTy
    = std::unordered_map<std::string, Node*>;
  using SymTableTy
    = std::stack<SymEntriesTy>;
  SymTableTy SymTable;

  void ParseVarDecl();

  void ParseArrayDecl();

  void ParseFuncDecl();

public:
  Parser(std::istream& IS, Graph& graph)
    : G(graph),
      Lex(IS) {}

  void Parse();
};
} // end namespace gross
#endif
