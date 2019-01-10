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
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_VAR);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_FUNCTION);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_LET);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_IF);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_END_IF);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_WHILE);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_DO);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_END_DO);
  }
  SS.clear();
  {
    // numbers and identifiers
    SS << "87 yihshyng223 0 rem88the99best00";
    Lexer L(SS);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_NUMBER);
    EXPECT_EQ(L.getBuffer(), "87");
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_IDENT);
    EXPECT_EQ(L.getBuffer(), "yihshyng223");
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_NUMBER);
    EXPECT_EQ(L.getBuffer(), "0");
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_IDENT);
    EXPECT_EQ(L.getBuffer(), "rem88the99best00");
  }
  SS.clear();
  {
    // assign and relation operators
    SS << "<- >= < <= != ==";
    Lexer L(SS);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_L_ARROW);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_BIN_OP);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_BIN_OP);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_BIN_OP);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_BIN_OP);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_BIN_OP);
  }
}

TEST(LexerTest, TestCompleteSentence) {
  std::stringstream SS;
  {
    SS << "var foo;";
    Lexer L(SS);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_VAR);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_IDENT);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_SEMI_COLON);
  }
  SS.clear();
  {
    // with new line
    SS << "var foo;" << "\n"
       << "array[2] bar;";
    Lexer L(SS);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_VAR);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_IDENT);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_SEMI_COLON);

    EXPECT_EQ(L.getNextToken(), Lexer::TOK_ARRAY);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_L_BRACKET);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_NUMBER);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_R_BRACKET);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_IDENT);
    EXPECT_EQ(L.getNextToken(), Lexer::TOK_SEMI_COLON);
  }
  // TODO: Test (multi-line)comments
}
