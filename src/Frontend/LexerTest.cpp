#include "Lexer.h"
#include "gtest/gtest.h"
#include <sstream>

using namespace gross;

TEST(LexerTest, TestBasicTokens) {
  std::stringstream SS;
  {
    // Some keywords
    SS << "var function let if fi while do od";
    Lexer L(SS);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_VAR);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_FUNCTION);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_LET);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_IF);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_END_IF);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_WHILE);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_DO);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_END_DO);
  }
  SS.clear();
  {
    // numbers and identifiers
    SS << "87 yihshyng223 0 rem88the99best00";
    Lexer L(SS);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_NUMBER);
    ASSERT_EQ(L.getBuffer(), "87");
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_IDENT);
    ASSERT_EQ(L.getBuffer(), "yihshyng223");
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_NUMBER);
    ASSERT_EQ(L.getBuffer(), "0");
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_IDENT);
    ASSERT_EQ(L.getBuffer(), "rem88the99best00");
  }
  SS.clear();
  {
    // assign and relation operators
    SS << "<- >= < <= != ==";
    Lexer L(SS);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_L_ARROW);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_BIN_OP);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_BIN_OP);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_BIN_OP);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_BIN_OP);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_BIN_OP);
  }
  SS.clear();
  {
    // keywords, identifiers, and symbols
    SS << "var foo;";
    Lexer L(SS);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_VAR);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_IDENT);
    ASSERT_EQ(L.getNextToken(), Lexer::TOK_SEMI_COLON);
  }
}
