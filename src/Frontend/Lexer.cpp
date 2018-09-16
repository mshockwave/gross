
#include "Lexer.h"
#include "gross/Support/Log.h"
#include <cctype>

using namespace gross;

Lexer::Lexer(std::istream &IS)
  : Input(IS),
    LastChar(' '),
    Buffer(),
    CurToken(TOK_NONE) {}

Lexer::Token Lexer::getNextToken() {
  Diagnostic Diag;
  Buffer.clear();
  CurToken = TOK_NONE;

  bool InComment = false;
  auto IsComment = [this]() -> bool {
    return LastChar == '#' ||
           (LastChar == '/' && Input.peek() == '/');
  };
  while(std::isspace(LastChar) ||
        IsComment() ||
        InComment) {
    if(InComment &&
       (LastChar == '\r' || LastChar == '\n'))
      InComment = false;
    if(IsComment()) InComment = true;
    LastChar = Advance();
  }

  if(std::isalpha(LastChar)) {
    while(std::isalnum(LastChar)) {
      Buffer.push_back(LastChar);
      LastChar = Advance();
    }

    if(Buffer == "let")
      CurToken = TOK_LET;
    else if(Buffer == "var")
      CurToken = TOK_VAR;
    else if(Buffer == "array")
      CurToken = TOK_ARRAY;
    else if(Buffer == "function")
      CurToken = TOK_FUNCTION;
    else if(Buffer == "procedure")
      CurToken = TOK_PROCEDURE;
    else if(Buffer == "if")
      CurToken = TOK_IF;
    else if(Buffer == "else")
      CurToken = TOK_ELSE;
    else if(Buffer == "fi")
      CurToken = TOK_END_IF;
    else if(Buffer == "while")
      CurToken = TOK_WHILE;
    else if(Buffer == "do")
      CurToken = TOK_DO;
    else if(Buffer == "od")
      CurToken = TOK_END_DO;
    else if(Buffer == "return")
      CurToken = TOK_RETURN;
    else
      CurToken = TOK_IDENT;

    LastChar = Advance();
  } else if(std::isdigit(LastChar)) {
    // Numbers
    while(std::isdigit(LastChar)) {
      Buffer.push_back(LastChar);
      LastChar = Advance();
    }
    return TOK_NUMBER;
  } else {
    Buffer.push_back(LastChar);
    switch(LastChar) {
    case ']':
    case '[':
      CurToken = TOK_BRACKET;
      break;
    case '(':
    case ')':
      CurToken = TOK_PARAN;
      break;
    case '{':
    case '}':
      CurToken = TOK_CUR_BRACKET;
      break;
    case '<': {
      if(Input.peek() == '-') {
        Buffer.push_back(Advance());
        CurToken = TOK_L_ARROW;
      } else {
        if(Input.peek() == '=')
          Buffer.push_back(Advance());
        CurToken = TOK_BIN_OP;
      }
      break;
    }
    case '>':
      if(Input.peek() == '=')
        Buffer.push_back(Advance());
      CurToken = TOK_BIN_OP;
      break;
    case '=':
    case '!':
      if(Input.peek() != '=') {
        Diag.Error() << "expecting character '=', not '"
                     << (char)Input.peek() << "'\n";
      } else {
        Buffer.push_back(Advance());
        CurToken = TOK_BIN_OP;
      }
      break;
    case ',':
      CurToken = TOK_COMMA;
      break;
    case ';':
      CurToken = TOK_SEMI_COLON;
      break;
    case '+':
    case '-':
    case '*':
    case '/':
      CurToken = TOK_BIN_OP;
      break;
    case EOF:
      CurToken = TOK_EOF;
      break;
    default:
      CurToken = TOK_NONE;
    }
    LastChar = Advance();
  }

  if(CurToken == TOK_NONE) {
    Diag.Error() << "unrecognized token '"
                 << Buffer << "'\n";
  }
  return CurToken;
}
