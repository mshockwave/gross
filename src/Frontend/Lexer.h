#ifndef GROSS_FRONTEND_LEXER
#define GROSS_FRONTEND_LEXER

#include <iostream>
#include <string>

namespace gross {
class Lexer {
  std::istream &Input;

  char LastChar;

  // NOTE: It would be better to use std::string_view here
  // But unfortunetely we're not C++17
  std::string Buffer;

  char Advance() {
    char ch = '\0';
    Input.get(ch);
    // aware of std::istream::get() and eof()'s behavior:
    // eof() will only check eofbit in one of std::istream's field,
    // instead of actively checking whether it's EOF.
    // get() will try to fetch one char, then flip the aforementioned
    // eofbit if EOF has reached.
    //
    // So we can't check eof() BEFORE Input.get(ch). Since although at
    // that timepoint, the 'cursor' of the stream has already pointed to
    // EOF or someplace off the bound, eof() will still returns false as
    // not until the following Input.get(ch) will the eofbit been flipped!
    // So if check eof() before Input.get(ch), we will alwasy fetch one char
    // more.
    if(Input.eof()) return EOF;
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
    TOK_DOT,
    // Operations
    TOK_BIN_OP,
    TOK_REL_LE,
    TOK_REL_LT,
    TOK_REL_GE,
    TOK_REL_GT,
    TOK_REL_EQ,
    TOK_REL_NE,
    // Brackets
    TOK_L_BRACKET,
    TOK_R_BRACKET,
    TOK_L_PARAN,
    TOK_R_PARAN,
    TOK_L_CUR_BRACKET,
    TOK_R_CUR_BRACKET,
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
    TOK_RETURN,
    TOK_CALL
  };

  Lexer(std::istream &IS);

  // Step forward
  Token getNextToken();
  // Get current token
  inline Token getToken() const { return CurToken; }
  const std::string& getBuffer() const { return Buffer; }

  // Some utilities
  // binary operator
  bool isBinOpStar() const {
    return getToken() == TOK_BIN_OP &&
           getBuffer() == "*";
  }
  bool isBinOpSlash() const {
    return getToken() == TOK_BIN_OP &&
           getBuffer() == "/";
  }
  bool isBinOpPlus() const {
    return getToken() == TOK_BIN_OP &&
           getBuffer() == "+";
  }
  bool isBinOpMinus() const {
    return getToken() == TOK_BIN_OP &&
           getBuffer() == "-";
  }

private:
  Token CurToken;
};
} // end namespace gross
#endif
