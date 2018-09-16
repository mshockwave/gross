#ifndef GROSS_FRONTEND_LEXER
#define GROSS_FRONTEND_LEXER

#include <iostream>
#include <string>

namespace gross {
class Lexer {
  std::istream &Input;

  char LastChar;

  // TODO: It would be better to use std::string_view here
  // But unfortunetely we're not C++17
  std::string Buffer;

  char Advance() {
    char ch;
    Input.get(ch);
    return ch;
  }

public:
  enum Token {
    TOK_NONE,
    TOK_EOF,
    // Characters
    //TOK_LETTER,
    //TOK_DIGIT,
    TOK_COMMA,
    TOK_SEMI_COLON,
    // Operations
    TOK_BIN_OP,
    // Brackets
    TOK_BRACKET,
    TOK_PARAN,
    TOK_CUR_BRACKET,
    // Primitives
    TOK_IDENT,
    TOK_NUMBER,
    TOK_LET,
    TOK_L_ARROW,
    TOK_VAR,
    TOK_ARRAY,
    TOK_FUNCTION,
    TOK_PROCEDURE,
    // Control flow
    TOK_IF,
    TOK_THEN,
    TOK_ELSE,
    TOK_END_IF,
    TOK_WHILE,
    TOK_DO,
    TOK_END_DO,
    TOK_RETURN
  };

  Lexer(std::istream &IS);

  // Step forward
  Token getNextToken();
  // Get current token
  inline Token getToken() { return CurToken; }
  const std::string& getBuffer() const { return Buffer; }

private:
  Token CurToken;
};
} // end namespace gross
#endif
